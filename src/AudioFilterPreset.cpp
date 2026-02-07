#include<hgl/audio/AudioFilterPreset.h>

namespace hgl
{
    AudioFilterConfig GetAudioFilterPresetConfig(AudioFilterPreset preset)
    {
        AudioFilterConfig config;
        config.enable = true;
        config.gain = 1.0f;
        config.gain_lf = 1.0f;
        config.gain_hf = 1.0f;

        switch(preset)
        {
            case AudioFilterPreset::OldTelephone:
                // 窄带通，突出中频，模拟电话机带宽
                config.type = AudioFilterType::Bandpass;
                config.gain = 1.0f;
                config.gain_lf = 0.2f;
                config.gain_hf = 0.2f;
                break;
            case AudioFilterPreset::TelephoneModern:
                // 较宽带通，听感更清晰
                config.type = AudioFilterType::Bandpass;
                config.gain = 1.0f;
                config.gain_lf = 0.35f;
                config.gain_hf = 0.4f;
                break;
            case AudioFilterPreset::Intercom:
                // 轻度带通，保留更多高频
                config.type = AudioFilterType::Bandpass;
                config.gain = 1.0f;
                config.gain_lf = 0.3f;
                config.gain_hf = 0.35f;
                break;
            case AudioFilterPreset::WalkieTalkie:
                // 更窄带通，整体更薄
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.95f;
                config.gain_lf = 0.2f;
                config.gain_hf = 0.25f;
                break;
            case AudioFilterPreset::Vinyl:
                // 低通略削高频
                config.type = AudioFilterType::Lowpass;
                config.gain = 1.0f;
                config.gain_hf = 0.6f;
                break;
            case AudioFilterPreset::Cassette:
                // 磁带机：低通更明显，整体略降
                config.type = AudioFilterType::Lowpass;
                config.gain = 0.9f;
                config.gain_hf = 0.45f;
                break;
            case AudioFilterPreset::OldRadio:
                // 带通+略降整体增益
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.9f;
                config.gain_lf = 0.4f;
                config.gain_hf = 0.5f;
                break;
            case AudioFilterPreset::AMRadio:
                // 更窄的带通
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.85f;
                config.gain_lf = 0.25f;
                config.gain_hf = 0.35f;
                break;
            case AudioFilterPreset::FMRadio:
                // 较宽带通，高频保留更多
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.9f;
                config.gain_lf = 0.45f;
                config.gain_hf = 0.6f;
                break;
            case AudioFilterPreset::Megaphone:
                // 带通+更明显的高频削减
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.9f;
                config.gain_lf = 0.3f;
                config.gain_hf = 0.3f;
                break;
            case AudioFilterPreset::PA:
                // 公共广播，带通但较宽
                config.type = AudioFilterType::Bandpass;
                config.gain = 0.9f;
                config.gain_lf = 0.45f;
                config.gain_hf = 0.55f;
                break;
            case AudioFilterPreset::Muffled:
                // 闷声/隔墙：强低通+略降整体增益
                config.type = AudioFilterType::Lowpass;
                config.gain = 0.85f;
                config.gain_hf = 0.25f;
                break;
            case AudioFilterPreset::InsideHelmet:
                // 头盔内：低通+轻微削低频
                config.type = AudioFilterType::Lowpass;
                config.gain = 0.9f;
                config.gain_hf = 0.35f;
                break;
            case AudioFilterPreset::UnderwaterFresh:
                // 淡水：高频衰减稍弱
                config.type = AudioFilterType::Lowpass;
                config.gain = 0.85f;
                config.gain_hf = 0.2f;
                break;
            case AudioFilterPreset::UnderwaterSea:
                // 海水：高频衰减更强
                config.type = AudioFilterType::Lowpass;
                config.gain = 0.75f;
                config.gain_hf = 0.12f;
                break;
            case AudioFilterPreset::None:
            default:
                // 禁用滤波
                config.type = AudioFilterType::None;
                config.enable = false;
                break;
        }

        return config;
    }
}//namespace hgl
