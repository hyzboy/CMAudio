#pragma once

#include<hgl/type/String.h>
#include<hgl/type/UnorderedMap.h>
#include<hgl/thread/ThreadMutex.h>
#include<hgl/CoreType.h>

namespace hgl::audio
{
    class AudioBuffer;
    class AudioLoadThread;                                  ///< 后台解码线程（实现于 .cpp，懒启动）

    /**
    * 音频资源加载模式
    */
    enum class AudioLoadMode
    {
        Full=0,      ///<全量加载（AudioBuffer，适合短音效）
        Stream       ///<流式加载（AudioPlayer，适合长音乐/BGM）
    };

    /**
    * 根据文件大小给出建议的加载模式
    * 启发式：小文件全量常驻（音效），大文件流式（BGM）
    * @param file_size 文件字节数
    * @param full_load_threshold 全量加载的字节数上限（默认 1MB）
    * @return 建议的加载模式
    */
    AudioLoadMode SuggestAudioLoadMode(int64 file_size,int64 full_load_threshold=1*1024*1024);

    /**
    * 音频资源管理器：缓存去重 + 引用计数（P0-2）
    *
    * - 同一文件（按 filename 字符串去重）只加载一次，多次 Acquire 共享同一 AudioBuffer
    * - 引用计数归零时自动卸载；外部可通过 Clear() 强制清空
    * - 线程安全：所有缓存操作在内部互斥锁保护下进行
    */
    class AudioAssetManager
    {
        UnorderedMap<OSString, AudioBuffer *> assets;       ///< 文件名 → 缓冲区缓存
        mutable ThreadMutex lock;

        AudioLoadThread *load_thread;                       ///< 后台解码线程（懒启动，nullptr=未创建）

    public:

        AudioAssetManager();
        ~AudioAssetManager();

        /**
        * 取得（或加载）一个音频缓冲区
        * 命中缓存则引用计数+1；未命中则同步加载并登记（引用计数=1）
        * @param filename 音频文件名（作为缓存键，需保持一致才能命中）
        * @return 缓冲区指针，加载失败返回 nullptr
        */
        AudioBuffer *Acquire(const os_char *filename);

        /**
        * 手动登记一个已加载的缓冲区到缓存（引用计数=1）
        * 用于测试注入、内存资源或自定义加载路径
        * @param name 缓存键
        * @param buffer 缓冲区（非空，且应已加载成功）
        * @return 是否登记成功（键已存在则失败）
        */
        bool Register(const os_char *name,AudioBuffer *buffer);

        /**
        * 释放一次引用，引用计数归零时卸载并从缓存移除
        */
        void Release(AudioBuffer *buffer);
        void Release(const os_char *name);

        /**
        * 只读查找（不改变引用计数）
        * @return 未命中返回 nullptr
        */
        AudioBuffer *Find(const os_char *name)const;

        bool Contains(const os_char *name)const;            ///< 是否已缓存

        int  GetCount()const;                               ///< 缓存条目数
        void Clear();                                       ///< 清空全部缓存（无视引用计数）

    public: //异步加载（P0-2）

        /**
        * 异步加载：提交后台解码任务，解码完成后在 Update() 中上传并登记缓存
        * 调用方之后用 Acquire() 命中缓存取得缓冲区（零成本）
        * @param filename 音频文件名
        * @return 是否已提交（已缓存命中或无法识别类型返回 false）
        */
        bool AcquireAsync(const os_char *filename);

        /**
        * 主线程每帧调用：上传已完成解码的缓冲到 OpenAL 并登记缓存
        * @return 本次完成上传/处理的任务数
        */
        int  Update();

        bool IsLoading()const;                              ///< 是否有后台任务在途
        int  GetPendingCount()const;                        ///< 在途任务数（待解码 + 待上传）
    };//class AudioAssetManager
}//namespace hgl::audio
