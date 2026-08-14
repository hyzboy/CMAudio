#include"AudioDecode.h"
#include<hgl/plugin/PlugInManager.h>
#include<hgl/plugin/PlugInInterface.h>
#include<hgl/filesystem/FileSystem.h>
#include<hgl/type/UnorderedMap.h>

using namespace openal;
namespace hgl::audio
{
    namespace
    {
        PlugInManager audio_plug_in(OS_TEXT("Audio"));

        UnorderedMap<AnsiString,OSString> audio_ext_map;       ///<扩展名(小写)→插件名 动态映射缓存
        bool audio_ext_map_built=false;

        /**
         * 通过扫描并探测音频插件,依据插件上报的 FileExtensions 能力建立"扩展名→插件名"映射。
         * 仅在首次需要时构建,之后直接复用缓存。
         */
        void BuildAudioExtensionMap()
        {
            if(audio_ext_map_built)return;
            audio_ext_map_built=true;

            ManagedArray<PlugInInfo> infos;
            audio_plug_in.ScanAndProbe(infos);

            const int count=infos.GetCount();
            for(int i=0;i<count;i++)
            {
                const PlugInInfo *info=infos[i];
                if(!info)continue;

                const int ec=info->extensions.GetCount();
                for(int e=0;e<ec;e++)
                {
                    const AnsiString &ext=info->extensions[e];
                    if(ext.IsEmpty())continue;

                    audio_ext_map.Add(ext.ToLowerCase(),info->name);
                }
            }
        }
    }

    const OSString *GetAudioPluginNameByExtension(const char *ext_name)
    {
        if(!ext_name||!(*ext_name))return nullptr;

        BuildAudioExtensionMap();

        AnsiString ext(ext_name);
        ext.LowerCase();

        return audio_ext_map.GetValuePointer(ext);
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
}//namespace hgl::audio
