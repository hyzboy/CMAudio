#include<hgl/audio/AudioEngine.h>
#include<hgl/audio/AudioAssetManager.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/SpatialAudioWorld.h>

namespace hgl::audio
{
    AudioEngine::AudioEngine()
    {
        music  =master.CreateChild("Music");
        sfx    =master.CreateChild("SFX");
        ambient=master.CreateChild("Ambient");
        ui     =master.CreateChild("UI");

        asset_manager=new AudioAssetManager;
    }

    AudioEngine::~AudioEngine()
    {
        delete asset_manager;
        asset_manager=nullptr;
    }

    AudioBuffer *AudioEngine::Acquire(const os_char *filename)
    {
        return asset_manager?asset_manager->Acquire(filename):nullptr;
    }

    void AudioEngine::Release(AudioBuffer *buffer)
    {
        if(asset_manager)asset_manager->Release(buffer);
    }

    void AudioEngine::Release(const os_char *filename)
    {
        if(asset_manager)asset_manager->Release(filename);
    }

    bool AudioEngine::AcquireAsync(const os_char *filename)
    {
        return asset_manager?asset_manager->AcquireAsync(filename):false;
    }

    void AudioEngine::AddWorld(SpatialAudioWorld *world)
    {
        if(world)worlds.Add(world);
    }

    void AudioEngine::RemoveWorld(SpatialAudioWorld *world)
    {
        if(world)worlds.Delete(world);
    }

    int AudioEngine::GetWorldCount()const
    {
        return (int)worlds.GetCount();
    }

    void AudioEngine::Update(const double &ct)
    {
        // 1. 资源管理：上传已完成解码的异步缓冲
        if(asset_manager)
            asset_manager->Update();

        // 2. 驱动所有空间音频世界
        for(SpatialAudioWorld *world : worlds)
            if(world)
                world->Update(ct);
    }
}//namespace hgl::audio
