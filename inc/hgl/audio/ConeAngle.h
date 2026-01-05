#pragma once

namespace hgl
{
    class ConeAngle             ///锥形角度数据类
    {
    public:

        float inner;            //内部
        float outer;            //外部

    public:

        ConeAngle()
        {
            inner=0;
            outer=0;
        }
    };
}//namespace hgl
