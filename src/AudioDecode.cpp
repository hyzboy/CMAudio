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

    bool GetAudioMidiInterface(const OSString &name,AudioMidiConfigInterface *amci)
    {
        PlugIn *pi=audio_plug_in.LoadPlugin(name);

        if(!pi)
            return(false);

        if(amci)
            return pi->GetInterface(4,amci);

        return false;
    }

    bool GetAudioMidiChannelInterface(const OSString &name,AudioMidiChannelInterface *amchi)
    {
        PlugIn *pi=audio_plug_in.LoadPlugin(name);

        if(!pi)
            return(false);

        if(amchi)
            return pi->GetInterface(5,amchi);

        return false;
    }

    void CloseAudioPlugIns()
    {
        audio_plug_in.Clear();
    }
}//namespace hgl
