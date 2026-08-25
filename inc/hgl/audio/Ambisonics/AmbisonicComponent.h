#pragma once

#include<hgl/audio/Ambisonics/AmbisonicTypes.h>

namespace hgl::audio
{
    /**
    * Ambisonics 组件基类（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    */
    class AmbisonicComponent
    {
    public:
        AmbisonicComponent()=default;
        virtual ~AmbisonicComponent()=default;

        uint32 GetOrder()const{return order;}
        bool Is3D()const{return is3D;}
        uint32 GetChannelCount()const{return channelCount;}

        /** 配置：阶数（1-3）+ 是否 3D（含高度） */
        virtual bool Configure(uint32 _order,bool _is3D)
        {
            if(_order<1||_order>3)
                return false;

            order=_order;
            is3D=_is3D;

            // ACN 通道数：(order+1)^2（3D）或 2*order+1（2D）
            channelCount=is3D?(order+1)*(order+1):order*2+1;

            return true;
        }

        virtual void Reset()=0;
        virtual void Refresh()=0;

    protected:
        uint32 order=0;
        bool is3D=true;
        uint32 channelCount=0;
    };
}//namespace hgl::audio
