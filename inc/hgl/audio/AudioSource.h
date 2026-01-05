#pragma once

#include<hgl/audio/ConeAngle.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/al/al.h>
#include<hgl/math/Vector.h>
#include<hgl/log/Log.h>

namespace hgl
{
    using namespace math;

    class AudioListener;

    /**
    * 音频源，指的是一个发声源，要发声必须创建至少一个发声源。而这个类就是管理发声源所用的。
    */
    class AudioSource                                                                       ///音频源类
    {
        OBJECT_LOGGER

        friend class AudioPlayer;

    private:

        void InitPrivate();

        AudioBuffer *Buffer;

    protected:

        uint index;

        bool pause;

        bool loop;
        float pitch;
        float gain;
        float cone_gain;
        Vector3f position;
        Vector3f velocity;
        Vector3f direction;
        float ref_dist,max_dist;
        uint distance_model;
        float rolloff_factor;
        ConeAngle angle;

        float doppler_factor;
        float doppler_velocity;
        
        float air_absorption_factor;

    public: //属性

                        uint    GetIndex()const{return index;}                                      ///<获取当前音源索引
                        int     GetState()const;                                                    ///<获取当前音源状态

                const   bool    IsNone      ()const{return GetState()==AL_NONE; }
                const   bool    IsStopped   ()const{return GetState()==AL_STOPPED;}
                const   bool    IsPaused    ()const{return GetState()==AL_PAUSED;}
                const   bool    IsPlaying   ()const{return GetState()==AL_PLAYING;}

                        double  GetCurTime()const;                                                  ///<获取当前播放到的时间
                        void    SetCurTime(const double &);                                         ///<设置当前播放时间

                        float   GetMinGain()const;                                                  ///<获取最小增益
                        float   GetMaxGain()const;                                                  ///<获取最大增益

                const   bool    IsLoop()const{return loop;}                                         ///<是否循环播放
        virtual         void    SetLoop(bool);                                                      ///<设置是否循环播放

                const   float   GetPitch()const{return pitch;}                                      ///<获取播放频率
                        void    SetPitch(float);                                                    ///<设置播放频率

                const   float   GetGain()const{return gain;}                                        ///<获取音量增益幅度
                        void    SetGain(float);                                                     ///<设置音量增益幅度
                const   float   GetConeGain()const{return cone_gain;}                               ///<获取锥形音量增益幅度
                        void    SetConeGain(float);                                                 ///<设置锥形音量增益幅度

                const   uint    GetDistanceModel()const{return distance_model;}                     ///<获取音量距离衰减模型
                        void    SetDistanceModel(uint);                                             ///<设置音量距离衰减模型

                const   float   GetRolloffFactor()const{return rolloff_factor;}                     ///<获取音量衰减因子
                        void    SetRolloffFactor(float);                                            ///<设置音量衰减因子(>=0,默认1.0)

                        void    GetDoppler(float &factor,float &velocity)const                      ///<获取多普勒强度和速度
                        {
                            factor=doppler_factor;
                            velocity=doppler_velocity;
                        }

                        void    SetDopplerFactor(const float &);                                    ///<设置多普勒效果强度
                        void    SetDopplerVelocity(const float &);                                  ///<设置多普勒速度
                        
                const   float   GetAirAbsorptionFactor()const{return air_absorption_factor;}        ///<获取空气吸收因子
                        void    SetAirAbsorptionFactor(const float &);                              ///<设置空气吸收因子(0.0-10.0,默认0.0)

                const   void    GetDistance(float &rd,float &md)const                               ///<获取音源距离范围
                {
                    rd=ref_dist;
                    md=max_dist;
                }
                        void    SetDistance(const float &ref_distance,const float &max_distance);   ///<设置音源距离范围

                const Vector3f &GetPosition()const{return position;}                                ///<获取音源位置
                      void      SetPosition(const Vector3f &);                                      ///<设置音源位置

                const Vector3f &GetVelocity()const{return velocity;}                                ///<获取速度
                      void      SetVelocity(const Vector3f &);                                      ///<设置速度

                const Vector3f &GetDirection()const{return direction;}                              ///<获取发声方向
                      void      SetDirection(const Vector3f &);                                     ///<设置发声方向

                const ConeAngle &GetAngle()const{return angle;}                                     ///<获取发声锥形角度
                      void      SetConeAngle(const ConeAngle &);                                    ///<设置发声锥形角度

    public: //方法

        AudioSource(bool=false);                                                                    ///<本类构造函数
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
}//namespace hgl
