#include<hgl/audio/AudioEvent.h>
#include<hgl/util/hash/FNV1a.h>
#include<cstring>

namespace hgl::audio
{
    uint32 CueNameHash(const char *name)
    {
        if(!name)
            return(0);

        // 复用 CMCoreType 的 FNV-1a 原语（标准 offset basis/prime，跨平台确定性）
        uint32 hash=hgl::hash::FNV1aInit<uint32>();
        hash=hgl::hash::FNV1aAppendBytes(hash,name,std::strlen(name));

        return(hash);
    }
}//namespace hgl::audio
