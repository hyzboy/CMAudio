#include<hgl/audio/AudioAssetManager.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/AudioFileType.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/type/Queue.h>
#include<hgl/thread/Thread.h>
#include<hgl/thread/Atomic.h>
#include<hgl/time/Time.h>
#include<hgl/log/Log.h>
#include"AudioDecode.h"

namespace hgl::audio
{
    using namespace io;

    AudioLoadMode SuggestAudioLoadMode(int64 file_size,int64 full_load_threshold)
    {
        // 负值或未知大小按流式处理（保守策略）
        if(file_size<0)return AudioLoadMode::Stream;

        return file_size<=full_load_threshold
                ? AudioLoadMode::Full
                : AudioLoadMode::Stream;
    }

    // ====================================================================
    // 后台解码线程（内部实现，仅在本文件可见）
    // 职责：读文件 + 插件解码（纯 CPU/IO，不碰 OpenAL），产出 DecodedAudio
    // 主线程在 Update() 中消费结果并上传到 OpenAL buffer
    // ====================================================================

    struct AudioLoadTask
    {
        OSString      filename;
        AudioFileType file_type;
    };

    struct CompletedLoad
    {
        OSString      filename;
        DecodedAudio *decoded;          // nullptr 表示解码失败
    };

    class AudioLoadThread:public Thread
    {
        Queue<AudioLoadTask *> pending;         // 待解码任务
        Queue<CompletedLoad *> done;            // 已解码（主线程消费）

        mutable ThreadMutex queue_lock;         // 保护 pending/done

        atom<int> task_count;                   // 在途任务数（含解码中+待上传），原子，避免队列空窗导致轮询提前结束

    public:

        AudioLoadThread():task_count(0){}

        ~AudioLoadThread()
        {
            AudioLoadTask *task;

            while(pending.Pop(task))delete task;

            CompletedLoad *item;

            while(done.Pop(item))
            {
                if(item->decoded)
                {
                    item->decoded->Release();
                    delete item->decoded;
                }

                delete item;
            }
        }

        bool DeletedAfterExit()const override{return false;}

        void Submit(const os_char *filename,AudioFileType file_type)
        {
            AudioLoadTask *task=new AudioLoadTask;

            task->filename=filename;
            task->file_type=file_type;

            queue_lock.Lock();
            pending.Push(task);
            queue_lock.Unlock();

            task_count.fetch_add(1);            // 在途 +1
        }

        CompletedLoad *PopDone()
        {
            CompletedLoad *item=nullptr;

            queue_lock.Lock();

            if(done.Pop(item))
                task_count.fetch_sub(1);        // 取走一个完成项，在途 -1

            queue_lock.Unlock();

            return item;
        }

        bool HasWork()const
        {
            return task_count.load()>0;
        }

        int GetTaskCount()const
        {
            return task_count.load();
        }

        bool Execute() override
        {
            AudioLoadTask *task=nullptr;

            queue_lock.Lock();
            pending.Pop(task);
            queue_lock.Unlock();

            if(!task)
            {
                SleepSecond(0.001);     // 队列空，让出 CPU；退出由 exit_lock 检测
                return true;
            }

            // 读文件 + 解码（纯 CPU/IO，不碰 OpenAL，线程安全）
            DecodedAudio *decoded=nullptr;

            OpenFileInputStream file_stream(task->filename);

            if(!file_stream)
            {
                GLogError(OS_TEXT("AudioAssetManager: 打开文件失败 ")+task->filename);
            }
            else
            {
                const int64 file_size=file_stream->Available();

                if(file_size<=0)
                {
                    GLogError(OS_TEXT("AudioAssetManager: 文件为空 ")+task->filename);
                }
                else
                {
                    char *memory=new char[file_size];

                    const int64 read_size=file_stream->Read(memory,file_size);

                    if(read_size>0)
                        decoded=DecodeAudio(task->file_type,memory,(int)read_size);

                    delete[] memory;
                }
            }

            CompletedLoad *item=new CompletedLoad;

            item->filename=task->filename;
            item->decoded=decoded;              // nullptr 表示失败

            queue_lock.Lock();
            done.Push(item);
            queue_lock.Unlock();

            delete task;

            return true;
        }
    };//class AudioLoadThread

    // ====================================================================
    // AudioAssetManager
    // ====================================================================

    AudioAssetManager::AudioAssetManager()
    {
        load_thread=nullptr;
    }

    AudioAssetManager::~AudioAssetManager()
    {
        if(load_thread)
        {
            load_thread->WaitExit();
            delete load_thread;
            load_thread=nullptr;
        }

        Clear();
    }

    AudioBuffer *AudioAssetManager::Acquire(const os_char *filename)
    {
        if(!filename||!(*filename))return nullptr;

        ThreadMutexLock lock_guard(&lock);

        const OSString key(filename);

        AudioBuffer *buffer=nullptr;

        if(assets.Get(key,buffer))          // 命中缓存
        {
            buffer->IncRef();
            return buffer;
        }

        // 未命中：同步加载（在锁内进行，保证同一文件的并发 Acquire 不会重复加载）
        buffer=new AudioBuffer(filename);

        if(!buffer->IsLoaded())
        {
            delete buffer;
            return nullptr;
        }

        buffer->IncRef();                   // 引用计数 = 1（登记到缓存）

        assets.Add(key,buffer);

        return buffer;
    }

    bool AudioAssetManager::Register(const os_char *name,AudioBuffer *buffer)
    {
        if(!name||!(*name)||!buffer)return false;

        ThreadMutexLock lock_guard(&lock);

        const OSString key(name);

        if(assets.ContainsKey(key))return false;

        buffer->IncRef();                   // 引用计数 = 1（登记到缓存）

        return assets.Add(key,buffer);
    }

    void AudioAssetManager::Release(AudioBuffer *buffer)
    {
        if(!buffer)return;

        ThreadMutexLock lock_guard(&lock);

        const uint rc=buffer->GetRefCount();

        if(rc==0)return;                    // 无引用（如异步预加载后未 Acquire 的 buffer），忽略

        buffer->DecRef();                   // rc>=1 → rc-1

        if(rc>1)return;                     // 仍有其他引用，保留

        // 归零：按指针反查 key 并从缓存移除
        for(auto it=assets.begin();it!=assets.end();++it)
        {
            if(it->second==buffer)
            {
                assets.DeleteByKey(it->first);
                break;
            }
        }

        delete buffer;
    }

    void AudioAssetManager::Release(const os_char *name)
    {
        if(!name||!(*name))return;

        ThreadMutexLock lock_guard(&lock);

        const OSString key(name);

        AudioBuffer *buffer=nullptr;

        if(!assets.Get(key,buffer))return;

        const uint rc=buffer->GetRefCount();

        if(rc==0)return;                    // 无引用（如异步预加载后未 Acquire 的 buffer），忽略

        buffer->DecRef();                   // rc>=1 → rc-1

        if(rc>1)return;                     // 仍有其他引用，保留

        assets.DeleteByKey(key);

        delete buffer;
    }

    AudioBuffer *AudioAssetManager::Find(const os_char *name)const
    {
        if(!name||!(*name))return nullptr;

        lock.Lock();

        AudioBuffer *buffer=nullptr;

        assets.Get(OSString(name),buffer);

        lock.Unlock();

        return buffer;
    }

    bool AudioAssetManager::Contains(const os_char *name)const
    {
        if(!name||!(*name))return false;

        lock.Lock();

        const bool rv=assets.ContainsKey(OSString(name));

        lock.Unlock();

        return rv;
    }

    int AudioAssetManager::GetCount()const
    {
        lock.Lock();

        const int rv=assets.GetCount();

        lock.Unlock();

        return rv;
    }

    void AudioAssetManager::Clear()
    {
        ThreadMutexLock lock_guard(&lock);

        for(auto &kv : assets)
            delete kv.second;

        assets.Clear();
    }

    bool AudioAssetManager::AcquireAsync(const os_char *filename)
    {
        if(!filename||!(*filename))return false;

        {
            ThreadMutexLock lock_guard(&lock);

            if(assets.ContainsKey(OSString(filename)))return true;   // 已缓存，无需异步
        }

        const AudioFileType file_type=CheckAudioFileType(filename);

        if(!RangeCheck(file_type))return false;                     // 无法识别类型

        if(!load_thread)                                            // 懒启动后台线程
        {
            load_thread=new AudioLoadThread;
            load_thread->Start();
        }

        load_thread->Submit(filename,file_type);

        return true;
    }

    int AudioAssetManager::Update()
    {
        if(!load_thread)return 0;

        int completed=0;

        CompletedLoad *item;

        while((item=load_thread->PopDone())!=nullptr)
        {
            ++completed;

            AudioBuffer *buffer=nullptr;

            if(item->decoded)                                       // 解码成功
            {
                buffer=new AudioBuffer;                             // 空 buffer（不碰 OpenAL）

                if(!buffer->SetData(item->decoded->format,
                                    item->decoded->data,
                                    item->decoded->size,
                                    item->decoded->freq))
                {
                    delete buffer;
                    buffer=nullptr;
                }

                item->decoded->Release();                           // 释放插件解码数据
                delete item->decoded;
            }

            if(buffer)
            {
                ThreadMutexLock lock_guard(&lock);

                AudioBuffer *existing=nullptr;

                if(assets.Get(item->filename,existing))             // 同步路径已抢先加载
                {
                    delete buffer;                                  // 丢弃重复加载
                }
                else
                {
                    assets.Add(item->filename,buffer);              // ref=0（预加载，无外部引用，Acquire 时 +1）
                }
            }

            delete item;
        }

        return completed;
    }

    bool AudioAssetManager::IsLoading()const
    {
        return load_thread && load_thread->HasWork();
    }

    int AudioAssetManager::GetPendingCount()const
    {
        return load_thread?load_thread->GetTaskCount():0;
    }
}//namespace hgl::audio
