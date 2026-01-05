#include"AudioDecode.h"
#include<hgl/plugin/PlugInManage.h>
#include<hgl/filesystem/FileSystem.h>

using namespace openal;
namespace hgl
{
    namespace
    {
        PlugInManage audio_plug_in(OS_TEXT("Audio"));
    }

    bool GetAudioInterface(const OSString &name,AudioPlugInInterface *api,AudioFloatPlugInInterface *afpi)
    {
        PlugIn *pi=audio_plug_in.LoadPlugin(name);

        if(!pi)
            return(false);

        uint result=0;

        if(api)
            if(pi->GetInterface(2,api))++result;

        if(afpi)
            if(pi->GetInterface(3,afpi))++result;

        return result>0;
    }

    void CloseAudioPlugIns()
    {
        audio_plug_in.Clear();
    }
}//namespace hgl
