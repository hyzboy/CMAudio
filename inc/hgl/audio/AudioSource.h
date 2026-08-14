#pragma once

#include<hgl/audio/ConeAngle.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/AudioFilter.h>
#include<hgl/audio/Gain.h>
#include<hgl/al/al.h>
#include<hgl/math/Vector.h>
#include<hgl/log/Log.h>

namespace hgl::audio
{
    using math::Vector3f;

    class AudioListener;
    class AudioBus;

    /**
    * 音频源，指的是一个发声源，要发声必须创建至少一个发声源。而这个类就是管理发声源所用的。
    */
    class AudioSource                                                                       ///音频源类
    {
        OBJECT_LOGGER

    private:

        void InitPrivate();

        void ApplyGain();                       ///< 唯一写 AL_GAIN 的出口（源增益 × 总线增益）

        AudioBuffer *buffer;

        AudioBus  *bus;                         ///< 所属总线（nullptr = 未挂载）
        float      bus_gain;                    ///< 缓存的总线有效增益（默认 1.0）

    protected:

        uint            source_id;                                              ///<音源索引

        bool            paused;                                              ///<是否暂停

        bool            loop;                                               ///<是否循环
        float           pitch;                                              ///<播放速率
        float           gain;                                               ///<基础增益
        float           cone_gain;                                          ///<锥形增益
        Vector3f        position;                                           ///<位置
        Vector3f        velocity;                                           ///<速度
        Vector3f        direction;                                          ///<朝向
        float           reference_distance,max_distance;                                  ///<参考/最大距离
        uint            distance_model;                                     ///<距离衰减模型
        float           rolloff_factor;                                     ///<距离衰减系数
        ConeAngle       cone_angle;                                              ///<锥形角度

        float           doppler_factor;                                     ///<多普勒强度
        float           doppler_velocity;                                   ///<多普勒速度

        float           air_absorption_factor;                              ///<空气吸收因子

        uint            direct_filter;                                      ///<直达声滤波器ID
        AudioFilterType filter_type;                                        ///<滤波器类型
        float           filter_gain;                                        ///<滤波增益
        float           filter_gain_lf;                                     ///<低频增益
        float           filter_gain_hf;                                     ///<高频增益

    public: //属性

                        uint    GetIndex()const{return source_id;}                                      ///<获取当前音源索引
                        int     GetState()const;                                                    ///<获取当前音源状态

                bool            IsNone      ()const{return GetState()==AL_NONE; }
                bool            IsStopped   ()const{return GetState()==AL_STOPPED;}
                bool            IsPaused    ()const{return GetState()==AL_PAUSED;}
                bool            IsPlaying   ()const{return GetState()==AL_PLAYING;}

                        double  GetPlaybackTime()const;                                                  ///<获取当前播放到的时间
                        void    SetPlaybackTime(const double &);                                         ///<设置当前播放时间

                        float   GetMinGain()const;                                                  ///<获取最小增益
                        float   GetMaxGain()const;                                                  ///<获取最大增益

                bool            IsLoop()const{return loop;}                                         ///<是否循环播放
        virtual         void    SetLoop(bool);                                                      ///<设置是否循环播放

                float           GetPitch()const{return pitch;}                                      ///<获取播放频率
                        void    SetPitch(float);                                                    ///<设置播放频率

                float           GetGain()const{return gain;}                                        ///<获取音量增益幅度
                        void    SetGain(float);                                                     ///<设置音量增益幅度
                float           GetGainDB()const{return GainToDB(gain);}                            ///<获取音量增益(dB)
                        void    SetGainDB(const float db){SetGain(DBToGain(db));}                   ///<设置音量增益(dB)
                float           GetConeGain()const{return cone_gain;}                               ///<获取锥形音量增益幅度
                        void    SetConeGain(float);                                                 ///<设置锥形音量增益幅度

                uint            GetDistanceModel()const{return distance_model;}                     ///<获取音量距离衰减模型
                        void    SetDistanceModel(uint);                                             ///<设置音量距离衰减模型

                float           GetRolloffFactor()const{return rolloff_factor;}                     ///<获取音量衰减因子
                        void    SetRolloffFactor(float);                                            ///<设置音量衰减因子(>=0,默认1.0)

                        void    GetDoppler(float &factor,float &velocity)const                      ///<获取多普勒强度和速度
                        {
                            factor=doppler_factor;
                            velocity=doppler_velocity;
                        }

                        void    SetDopplerFactor(const float &);                                    ///<设置多普勒效果强度
                        void    SetDopplerVelocity(const float &);                                  ///<设置多普勒速度

                float           GetAirAbsorptionFactor()const{return air_absorption_factor;}        ///<获取空气吸收因子
                        void    SetAirAbsorptionFactor(const float &);                              ///<设置空气吸收因子(0.0-10.0,默认0.0)

                bool            IsFilterEnabled()const{return filter_type!=AudioFilterType::None && direct_filter!=0;}
                AudioFilterType GetFilterType()const{return filter_type;}
                        bool    SetLowpassFilter(const float gain,const float gain_hf);
                        bool    SetHighpassFilter(const float gain,const float gain_lf);
                        bool    SetBandpassFilter(const float gain,const float gain_lf,const float gain_hf);
                        bool    SetFilter(const AudioFilterConfig &config);
                        void    DisableFilter();

                void            GetDistance(float &rd,float &md)const                               ///<获取音源距离范围
                {
                    rd=reference_distance;
                    md=max_distance;
                }
                        void    SetDistance(const float &ref_distance,const float &max_distance);   ///<设置音源距离范围

                const Vector3f &GetPosition()const{return position;}                                ///<获取音源位置
                      void      SetPosition(const Vector3f &);                                      ///<设置音源位置

                const Vector3f &GetVelocity()const{return velocity;}                                ///<获取速度
                      void      SetVelocity(const Vector3f &);                                      ///<设置速度

                const Vector3f &GetDirection()const{return direction;}                              ///<获取发声方向
                      void      SetDirection(const Vector3f &);                                     ///<设置发声方向

                const ConeAngle &GetAngle()const{return cone_angle;}                                     ///<获取发声锥形角度
                      void      SetConeAngle(const ConeAngle &);                                    ///<设置发声锥形角度

    public: //总线

        void        SetBus(AudioBus *);                     ///< 挂载/切换总线
        AudioBus *  GetBus()const{return bus;}              ///< 取得所属总线

        void        OnBusGainChanged(float effective_gain); ///< 总线增益变更回调（AudioBus 调用）
        void        OnBusDestroyed();                       ///< 总线析构回调（AudioBus 调用，解除关联）

    public: //方法

        AudioSource(bool=false);                                                                    ///< 本类构造函数
        AudioSource(AudioBuffer *);                                                                 ///<本类构造函数
        virtual ~AudioSource();                                                                     ///<本类析构函数

        virtual bool Play();                                                                        ///<播放当前音源
        virtual bool Play(bool);                                                                    ///<播放当前音源，并指定是否循环播放
        virtual void Pause();                                                                       ///<暂停播放
        virtual void Resume();                                                                      ///<继续播放
        virtual void Stop();                                                                        ///<停止播放
        virtual void Rewind();                                                                      ///<重绕播放

        virtual bool Create();                                                                      ///<创建音源
        virtual void Close();                                                                       ///<关闭音源

                bool Link(AudioBuffer *);                                                           ///<绑定一个音频缓冲区
                void Unlink();                                                                      ///<解除绑定
    };//class AudioSource
}//namespace hgl::audio
