#pragma once

#include<hgl/CoreType.h>
#include<cmath>
#include<vector>
#include<cstring>

namespace hgl::audio
{
    // ========================================================================
    // Ambisonics 共享基础类型（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    // ========================================================================

    /** B-Format 通道枚举（ACN 排序，SN3D 归一化，AmbiX 格式） */
    enum class BFormatChannel
    {
        W, Y, Z, X,          // 0 阶（W）+ 1 阶（Y/Z/X）
        V, T, R, S, U,       // 2 阶
        Q, O, M, K, L, N, P, // 3 阶
        COUNT
    };

    /** 扬声器布局预设 */
    enum class SpeakersPreset
    {
        Custom=-1,
        Mono,
        Stereo,
        Surround_5_1,
        Surround_7_1,
        CubePoints,
        DodecahedronFaces,
        LebedevGridOrder26,
        COUNT
    };

    inline constexpr float Amb_DegToRad=0.017453292519943295f;   ///< 度→弧度
    inline constexpr float Amb_PI=3.14159265358979323846f;
    inline constexpr float Amb_Epsilon=1e-5f;

    #define AMB_SQUARED(x) ((x)*(x))
    #define AMB_CUBED(x)   ((x)*(x)*(x))

    /** 3D 向量（HRIR 采样方向等，避免引入 GLM 依赖） */
    struct Vec3
    {
        float x=0.0f;
        float y=0.0f;
        float z=0.0f;
    };

    /**
    * 单声道浮点缓冲（对齐通道），替代 Amplitude AudioBufferChannel
    */
    class AudioChannel
    {
    public:
        AudioChannel()=default;
        explicit AudioChannel(uint32 size){resize(size);}
        AudioChannel(uint32 size,float value){resize(size,value);}

        uint32 size()const{return (uint32)_data.size();}
        void resize(uint32 n){_data.resize(n);}
        void resize(uint32 n,float v){_data.resize(n,v);}

        float *begin(){return _data.data();}
        const float *begin()const{return _data.data();}

        float *end(){return _data.data()+_data.size();}
        const float *end()const{return _data.data()+_data.size();}

        void clear(){std::fill(_data.begin(),_data.end(),0.0f);}

        float &operator[](uint32 index){return _data[index];}
        const float &operator[](uint32 index)const{return _data[index];}

        AudioChannel &operator=(const AudioChannel &channel)
        {
            _data=channel._data;
            return *this;
        }

        AudioChannel &operator+=(const AudioChannel &channel)
        {
            const uint32 n=(uint32)_data.size();

            for(uint32 i=0;i<n;i++)
                _data[i]+=channel._data[i];

            return *this;
        }

        AudioChannel &operator-=(const AudioChannel &channel)
        {
            const uint32 n=(uint32)_data.size();

            for(uint32 i=0;i<n;i++)
                _data[i]-=channel._data[i];

            return *this;
        }

        AudioChannel &operator*=(const AudioChannel &channel)
        {
            const uint32 n=(uint32)_data.size();

            for(uint32 i=0;i<n;i++)
                _data[i]*=channel._data[i];

            return *this;
        }

        AudioChannel &operator*=(float scalar)
        {
            const uint32 n=(uint32)_data.size();

            for(uint32 i=0;i<n;i++)
                _data[i]*=scalar;

            return *this;
        }

    private:
        std::vector<float> _data;
    };

    /**
    * 多声道浮点缓冲（帧×通道），替代 Amplitude AudioBuffer
    */
    class AmbisonicBuffer
    {
    public:
        AmbisonicBuffer()=default;

        AmbisonicBuffer(uint32 frame_count,uint32 channel_count)
        {
            _channels.resize(channel_count);

            for(uint32 i=0;i<channel_count;i++)
                _channels[i].resize(frame_count);
        }

        AudioChannel &operator[](uint32 index){return _channels[index];}
        const AudioChannel &operator[](uint32 index)const{return _channels[index];}

        AudioChannel &GetChannel(uint32 index){return _channels[index];}
        const AudioChannel &GetChannel(uint32 index)const{return _channels[index];}

        uint32 GetChannelCount()const{return (uint32)_channels.size();}
        uint32 GetFrameCount()const{return _channels.empty()?0:_channels[0].size();}

        void Clear()
        {
            for(auto &c:_channels)
                c.clear();
        }

        AmbisonicBuffer &operator*=(float scalar)
        {
            for(auto &c:_channels)
                c*=scalar;

            return *this;
        }

    private:
        std::vector<AudioChannel> _channels;
    };

    /**
    * 球坐标位置（方位角/仰角弧度，半径）
    */
    struct SphericalPosition
    {
        float azimuth=0.0f;     ///< 方位角（弧度，绕 z 轴）
        float elevation=0.0f;   ///< 仰角（弧度，绕 x 轴）
        float radius=1.0f;      ///< 半径

        SphericalPosition()=default;

        SphericalPosition(float _azimuth,float _elevation,float _radius=1.0f)
            : azimuth(_azimuth),elevation(_elevation),radius(_radius)
        {}

        static SphericalPosition FromDegrees(float azim_deg,float elev_deg,float r=1.0f)
        {
            return SphericalPosition(azim_deg*Amb_DegToRad,elev_deg*Amb_DegToRad,r);
        }

        void SetAzimuth(float v){azimuth=v;}
        void SetElevation(float v){elevation=v;}
        void SetRadius(float v){radius=v;}

        float GetAzimuth()const{return azimuth;}
        float GetElevation()const{return elevation;}
        float GetRadius()const{return radius;}

        /** 转笛卡尔坐标（HRIR 采样方向用） */
        Vec3 ToCartesian()const
        {
            const float cosElev=std::cos(elevation);
            const float sinElev=std::sin(elevation);
            const float cosAzim=std::cos(azimuth);
            const float sinAzim=std::sin(azimuth);

            return Vec3{radius*cosAzim*cosElev,radius*sinAzim*cosElev,radius*sinElev};
        }

        /** 从笛卡尔坐标构造（HRTF 方向约定：x 前 y 左 z 上） */
        static SphericalPosition ForHRTF(const Vec3 &v)
        {
            const float r=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);

            if(r<Amb_Epsilon)
                return SphericalPosition(0.0f,0.0f,1.0f);

            const float elev=std::asin(v.z/r);
            const float azim=std::atan2(v.y,v.x);

            return SphericalPosition(azim,elev,r);
        }
    };

    /**
    * 欧拉角朝向（alpha/beta/gamma，弧度）——声场旋转用
    */
    class Orientation
    {
        float _alpha=0.0f;      ///< 绕 z 轴
        float _beta=0.0f;       ///< 绕 y 轴
        float _gamma=0.0f;      ///< 绕 x 轴

    public:

        Orientation()=default;

        Orientation(float alpha,float beta,float gamma)
            : _alpha(alpha),_beta(beta),_gamma(gamma)
        {}

        void SetOrientation(float alpha,float beta,float gamma)
        {
            _alpha=alpha;
            _beta=beta;
            _gamma=gamma;
        }

        float GetAlpha()const{return _alpha;}
        float GetBeta()const{return _beta;}
        float GetGamma()const{return _gamma;}
    };

    /** 标量乘累加：result[i] += a[i]*b （长度 len） */
    inline void AmbScalarMultiplyAccumulate(const float *a,float *result,float b,uint32 len)
    {
        for(uint32 i=0;i<len;i++)
            result[i]+=a[i]*b;
    }

    /** 标量乘：result[i] = a[i]*b （长度 len） */
    inline void AmbScalarMultiply(float *result,const float *a,float b,uint32 len)
    {
        for(uint32 i=0;i<len;i++)
            result[i]=a[i]*b;
    }
}//namespace hgl::audio
