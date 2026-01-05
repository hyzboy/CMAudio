#pragma once

#include<hgl/thread/Thread.h>
#include<hgl/thread/ThreadMutex.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/math/Vector.h>
#include<hgl/time/Time.h>

using namespace openal;

namespace hgl
{
    using namespace math;

    namespace io
    {
        class InputStream;
    }//namespace io

    struct AudioPlugInInterface;

    enum class PlayState        //播放器状态
    {
        None=0,
        Play,
        Pause,
        Exit
    };

    /**
    * 使用AudioPlayer创建的音频播放器类，一般用于背景音乐等独占的音频处理。
    * 这个音频播放器将使用一个单独的线程，在播放器被删除时线程也会被关闭。
    */
    class AudioPlayer:public Thread                                                                 ///音频播放器基类
    {
        OBJECT_LOGGER

        ThreadMutex lock;

    protected:

        ALbyte *audio_data;
        int audio_data_size;

        void *audio_ptr;                                                                                ///<音频数据指针

        char *audio_buffer;
        int audio_buffer_size;
        uint audio_buffer_count;                                                                        ///<播放数据计数

        AudioPlugInInterface *decode;

        ALenum format;                                                                                  ///<音频数据格式
        ALsizei rate;                                                                                   ///<音频数据采样率

        struct
        {
            bool open;
            uint64 time;
            float gap;

            struct
            {
                float gain;
                double time;
            }start,end;
        }auto_gain;                                                                                     ///<自动增益

        bool ReadData(ALuint);
        bool UpdateBuffer();
        void ClearBuffer();

        bool Playback();

        bool DeletedAfterExit()const override{return false;}    
        bool Execute() override;

        void InitPrivate();
        bool Load(AudioFileType);

    protected:

        volatile bool loop;
        volatile PlayState ps;

        AudioSource audiosource;
        ALuint source;
        ALuint buffer[3];
        PreciseTime total_time;
        PreciseTime wait_time;

        double gain;

        PreciseTime start_time;

        PreciseTime fade_in_time;
        PreciseTime fade_out_time;

    public: //属性

                            uint        GetIndex()const{return audiosource.index;}                      ///<获取音源索引

                            double      GetTotalTime()const{return total_time;}                         ///<获取音频总时长

                            PlayState   GetPlayState()const{return ps;}                                 ///<获取播放器状态

                            int         GetSourceState()const{return audiosource.GetState();}           ///<获取音源索引

                            bool        IsLoop();                                                       ///<是否循环播放
                            void        SetLoop(bool);                                                  ///<设置循环播放

                            float       GetGain()const{return audiosource.gain;}                        ///<获取音量增益
                            float       GetMinGain()const{return audiosource.GetMinGain();}             ///<获取音量最小增益
                            float       GetMaxGain()const{return audiosource.GetMaxGain();}             ///<获取音量最大增益
                            float       GetConeGain()const{return audiosource.cone_gain;}               ///<获取音量锥形增益

                            void        SetGain(float val){audiosource.SetGain(val);}                   ///<设置音量增益
                            void        SetConeGain(float val){audiosource.SetConeGain(val);}           ///<设置音量锥形增益

                            float       GetPitch()const{return audiosource.pitch;}                      ///<获取播放频率
                            void        SetPitch(float val){audiosource.SetPitch(val);}                 ///<设置播放频率

                            float       GetRolloffFactor()const{return audiosource.rolloff_factor;}     ///<获取Rolloff因子
                            void        SetRolloffFactor(float f){audiosource.SetRolloffFactor(f);}     ///<设置Rolloff因子

    public: //属性方法

        const Vector3f &        GetPosition(){return audiosource.position;} const
        void                    SetPosition(const Vector3f &pos){audiosource.SetPosition(pos);}

        const Vector3f &        GetVelocity(){return audiosource.velocity;} const
        void                    SetVelocity(const Vector3f &vel){audiosource.SetVelocity(vel);}

        const Vector3f &        GetDirection(){return audiosource.direction;} const
        void                    SetDirection(const Vector3f &dir){audiosource.SetDirection(dir);}

        const void              GetDistance(float &ref_distance, float &max_distance)const{audiosource.GetDistance(ref_distance,max_distance);} const
        void                    SetDistance(const float &ref_distance,const float &max_distance){audiosource.SetDistance(ref_distance,max_distance);}

        const ConeAngle &       GetConeAngle(){return audiosource.angle;} const
        void                    SetConeAngle(const ConeAngle &ca){audiosource.SetConeAngle(ca);}

    public: //方法

        AudioPlayer();
        AudioPlayer(io::InputStream *,int,AudioFileType);
        AudioPlayer(const os_char *,AudioFileType=AudioFileType::None);
//      AudioPlayer(HAC *,const os_char *,AudioFileType=AudioFileType::None);
        virtual ~AudioPlayer();

        virtual bool Load(io::InputStream *,int,AudioFileType);                                     ///<从流中加载一个音频文件
        virtual bool Load(const os_char *,AudioFileType=AudioFileType::None);                       ///<加载一个音频文件
//      virtual bool Load(HAC *,const os_char *,AudioFileType=AudioFileType::None);                 ///<从HAC包中加载一个音频文件

        virtual void Play(bool=true);                                                               ///<播放音频
        virtual void Stop();                                                                        ///<停止播放
        virtual void Pause();                                                                       ///<暂停播放
        virtual void Resume();                                                                      ///<继续播放
        virtual void Clear();                                                                       ///<清除音频数据

        virtual PreciseTime GetPlayTime();                                                               ///<取得已播放时间(单位秒)
        virtual void SetFadeTime(PreciseTime,PreciseTime);                                                    ///<设置淡入淡出时间

        virtual void AutoGain(float,PreciseTime,const PreciseTime cur_time);                                  ///<自动音量
    };//class AudioPlayer
}//namespace hgl
