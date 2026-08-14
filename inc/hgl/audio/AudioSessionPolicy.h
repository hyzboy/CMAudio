#pragma once

#include<hgl/CoreType.h>

namespace hgl::audio
{
    /**
    * 音频焦点状态（P3-3 移动平台策略）
    */
    enum class AudioFocusState
    {
        HasFocus,       ///< 拥有焦点，正常播放
        LostTransient,  ///< 暂时失去（来电/导航提示），恢复后自动继续
        Lost,           ///< 永久失去（用户切换到其它媒体），需显式重新请求
    };//enum class AudioFocusState

    /**
    * 音频会话策略（P3-3）
    * 封装移动平台特有的音频会话行为：
    *   - iOS/iPadOS/tvOS：硬件静音开关检测（Ring/Silent switch）
    *   - Android：音频焦点（Audio Focus）请求/放弃
    *   - 桌面/主机：无焦点竞争、无静音开关（恒 HasFocus / 非静音）
    *
    * 引擎集成：启动时 Initialize()；播放前检查 GetFocusState()/IsSilenced()；
    * 每帧 Update() 轮询（iOS 静音开关状态变化需轮询检测）。
    */
    class AudioSessionPolicy
    {
    public:
        virtual ~AudioSessionPolicy() {}

        virtual bool            Initialize() = 0;           ///< 初始化会话策略
        virtual bool            RequestFocus() = 0;         ///< 请求音频焦点（Android）
        virtual void            AbandonFocus() = 0;         ///< 放弃音频焦点
        virtual void            Update() = 0;               ///< 每帧轮询（iOS 静音开关状态）
        virtual AudioFocusState GetFocusState()const = 0;   ///< 当前焦点状态
        virtual bool            IsSilenced()const = 0;      ///< 是否被静音（iOS 静音开关）
    };//class AudioSessionPolicy

    /**
    * 桌面/主机默认实现：无焦点竞争、无静音开关
    *   AbandonFocus 后为 Lost，RequestFocus 恢复为 HasFocus（用于测试与状态机验证）
    */
    class DesktopSessionPolicy : public AudioSessionPolicy
    {
        AudioFocusState state;

    public:
        DesktopSessionPolicy();

        bool            Initialize() override;
        bool            RequestFocus() override;
        void            AbandonFocus() override;
        void            Update() override;
        AudioFocusState GetFocusState()const override;
        bool            IsSilenced()const override;
    };//class DesktopSessionPolicy

    /**
    * 创建当前平台的音频会话策略实现（调用方负责 delete）
    *   移动平台返回平台实现（iOS/Android），桌面返回 DesktopSessionPolicy
    */
    AudioSessionPolicy *CreateSessionPolicy();
}//namespace hgl::audio
