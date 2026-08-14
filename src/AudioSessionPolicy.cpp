#include<hgl/audio/AudioSessionPolicy.h>

namespace hgl::audio
{
    // ===== DesktopSessionPolicy =====
    DesktopSessionPolicy::DesktopSessionPolicy()
    {
        state = AudioFocusState::HasFocus;
    }

    bool DesktopSessionPolicy::Initialize()
    {
        return true;
    }

    bool DesktopSessionPolicy::RequestFocus()
    {
        state = AudioFocusState::HasFocus;
        return true;
    }

    void DesktopSessionPolicy::AbandonFocus()
    {
        state = AudioFocusState::Lost;
    }

    void DesktopSessionPolicy::Update()
    {
    }

    AudioFocusState DesktopSessionPolicy::GetFocusState()const
    {
        return state;
    }

    bool DesktopSessionPolicy::IsSilenced()const
    {
        return false;
    }

    // ===== 平台实现 =====
#if (HGL_OS == HGL_OS_iOS) || (HGL_OS == HGL_OS_iPadOS) || (HGL_OS == HGL_OS_tvOS)
    /**
    * iOS/iPadOS/tvOS 实现（TODO：需集成 AVAudioSession）
    *   策略：游戏音频通常选择在静音开关开启时仍播放（AVAudioSessionCategoryPlayback
    *   类别的行为），IsSilenced() 通过轮询静音开关状态实现。
    *   完整实现需 Objective-C++ 桥接 AVAudioSession，此处为可编译 stub。
    */
    class iOSSessionPolicy : public AudioSessionPolicy
    {
        AudioFocusState state;

    public:
        iOSSessionPolicy(){ state = AudioFocusState::HasFocus; }
        bool Initialize() override { return true; }        // TODO: 设置 AVAudioSession 分类
        bool RequestFocus() override { return true; }      // iOS 无 Audio Focus 概念
        void AbandonFocus() override {}
        void Update() override {}                          // TODO: 轮询静音开关
        AudioFocusState GetFocusState()const override { return state; }
        bool IsSilenced()const override { return false; }  // TODO: 查询静音开关
    };
#endif

#if HGL_OS == HGL_OS_Android
    /**
    * Android 实现（TODO：需集成 AudioManager）
    *   策略：请求/放弃 Audio Focus；来电/导航时系统回调 LostTransient，
    *   恢复后重新请求焦点。
    *   完整实现需 JNI 桥接 AudioManager.requestAudioFocus()，此处为可编译 stub。
    */
    class AndroidSessionPolicy : public AudioSessionPolicy
    {
        AudioFocusState state;

    public:
        AndroidSessionPolicy(){ state = AudioFocusState::HasFocus; }
        bool Initialize() override { return true; }        // TODO: 获取 AudioManager
        bool RequestFocus() override { state = AudioFocusState::HasFocus; return true; }
        void AbandonFocus() override { state = AudioFocusState::Lost; }
        void Update() override {}
        AudioFocusState GetFocusState()const override { return state; }
        bool IsSilenced()const override { return false; }
    };
#endif

    AudioSessionPolicy *CreateSessionPolicy()
    {
#if (HGL_OS == HGL_OS_iOS) || (HGL_OS == HGL_OS_iPadOS) || (HGL_OS == HGL_OS_tvOS)
        return new iOSSessionPolicy();
#elif HGL_OS == HGL_OS_Android
        return new AndroidSessionPolicy();
#else
        return new DesktopSessionPolicy();
#endif
    }
}//namespace hgl::audio
