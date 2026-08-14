#pragma once

#include<hgl/audio/AudioBus.h>

namespace hgl::audio
{
    /**
    * 音频引擎（最小实现）：持有根总线（Master）并预建标准四子总线（Music/SFX/Ambient/UI）。
    * 后续扩展：update() 统一驱动、资源管理、事件系统、效果链等。
    */
    class AudioEngine
    {
        AudioBus master;

        AudioBus *music;
        AudioBus *sfx;
        AudioBus *ambient;
        AudioBus *ui;

    public:

        AudioEngine()
        {
            music  =master.CreateChild("Music");
            sfx    =master.CreateChild("SFX");
            ambient=master.CreateChild("Ambient");
            ui     =master.CreateChild("UI");
        }

        AudioBus *GetMaster (){return &master;}
        AudioBus *GetMusic  (){return music;}
        AudioBus *GetSFX    (){return sfx;}
        AudioBus *GetAmbient(){return ambient;}
        AudioBus *GetUI     (){return ui;}
    };//class AudioEngine
}//namespace hgl::audio
