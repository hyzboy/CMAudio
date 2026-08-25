#pragma once

#include<hgl/audio/Ambisonics/AmbisonicTypes.h>

namespace hgl::audio
{
    /**
    * HRIR 球面数据接口（R2 定义接口，R3 实现数据加载与插值）
    *
    * 提供空间各方向的头相关脉冲响应（HRIR），
    * 供 AmbisonicBinauralizer 双耳渲染采样。
    * 移植自 Amplitude Audio SDK HRIRSphere（Apache 2.0）。
    */
    class HRIRSphere
    {
    public:
        virtual ~HRIRSphere()=default;

        virtual bool IsLoaded()const=0;

        /** HRIR 长度（采样数） */
        virtual uint32 GetIRLength()const=0;

        /** 数据采样率（Hz） */
        virtual uint32 GetSampleRate()const=0;

        /**
        * 采样指定方向的左右耳 HRIR
        * @param direction 方向（笛卡尔单位向量）
        * @param left_hrir 输出左耳 HRIR（长度 >= GetIRLength()）
        * @param right_hrir 输出右耳 HRIR（长度 >= GetIRLength()）
        */
        virtual void Sample(const Vec3 &direction,float *left_hrir,float *right_hrir)const=0;
    };
}//namespace hgl::audio
