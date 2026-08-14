#pragma once

#include<hgl/audio/AudioBus.h>
#include<hgl/type/UnorderedSet.h>

namespace hgl::audio
{
    class AudioAssetManager;
    class AudioBuffer;
    class SpatialAudioWorld;

    /**
    * 音频引擎：总线树 + 资源管理 + 空间音频世界 + 统一更新驱动（P1-1）
    *
    * - 持有根总线（Master）与标准四子总线（Music/SFX/Ambient/UI）
    * - 持有 AudioAssetManager（资源缓存 + 异步加载，P0-2）
    * - 注册并驱动 SpatialAudioWorld（空间音频场景，引擎不持有其生命周期）
    * - Update() 统一驱动所有需要每帧更新的子系统（资源上传 + 各世界刷新）
    */
    class AudioEngine
    {
        AudioBus master;

        AudioBus *music;
        AudioBus *sfx;
        AudioBus *ambient;
        AudioBus *ui;

        AudioAssetManager *asset_manager;               ///< 资源管理（引擎持有）

        UnorderedSet<SpatialAudioWorld *> worlds;       ///< 注册的空间音频世界（引擎不持有）

    public:

        AudioEngine();
        ~AudioEngine();

    public: //总线

        AudioBus *GetMaster (){return &master;}
        AudioBus *GetMusic  (){return music;}
        AudioBus *GetSFX    (){return sfx;}
        AudioBus *GetAmbient(){return ambient;}
        AudioBus *GetUI     (){return ui;}

    public: //资源管理（转发到 asset_manager）

        AudioAssetManager *GetAssetManager(){return asset_manager;}

        AudioBuffer *Acquire(const os_char *filename);          ///< 取得（或加载）音频缓冲区（缓存去重）
        void         Release(AudioBuffer *buffer);              ///< 释放一次引用
        void         Release(const os_char *filename);          ///< 按文件名释放一次引用
        bool         AcquireAsync(const os_char *filename);     ///< 异步预加载（后台解码）

    public: //空间音频世界注册

        void AddWorld(SpatialAudioWorld *world);                ///< 注册一个空间音频世界（不持有）
        void RemoveWorld(SpatialAudioWorld *world);             ///< 注销一个空间音频世界
        int  GetWorldCount()const;                              ///< 已注册的世界数量

    public: //统一驱动

        /**
        * 统一驱动：主线程每帧调用
        * 依次：资源管理（上传已完成解码的异步缓冲）→ 各空间音频世界刷新
        * @param ct 当前时间（秒），0 表示由各子系统自行取时间
        */
        void Update(const double &ct=0);
    };//class AudioEngine
}//namespace hgl::audio
