#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 频谱门限降噪（NS，P3）
    *
    * 自研起步方案（后续可换 RNNoise）：
    * 帧级谱减——Hann 窗 + FFT → 幅度谱；静音帧（能量低于语音阈值）持续更新噪声底，
    * 语音帧对每个频点做谱减 max(|X|-α·N, β·|X|)，相位保持不变；IFFT 重叠相加还原。
    *
    * 流式帧接口：Init 后每帧 Process(in,out) 各 frame_samples 个样本。
    */
    class NoiseSuppressor
    {
        uint sample_rate;
        uint frame_samples;

        uint nfft;                      ///< FFT 长度（≥frame_samples 的 2 的幂）
        uint bin_count;                 ///< 频点数（nfft，含负频率——谱减必须覆盖全部频点）

        std::vector<float> window;      ///< Hann 窗（nfft）
        std::vector<float> real, imag;  ///< FFT 工作区
        std::vector<float> noise_floor; ///< 噪声底估计（每频点）
        std::vector<float> buf;         ///< 输入帧缓冲（nfft）

        int noise_frames;               ///< 已参与噪声底更新的帧数（预热期无条件更新）
        float noise_floor_db;           ///< 时域噪声底（dBFS 功率，与 FrameRMSDB 同量纲）
        float noise_alpha;              ///< 噪声底更新系数
        float speech_threshold_db;      ///< 语音判定阈值（相对噪声底总能量的抬升量 dB）
        float sub_alpha;                ///< 谱减系数
        float sub_floor;                ///< 谱减地板系数 β

    public:

        NoiseSuppressor();

        void Init(uint sample_rate,uint frame_samples);

        void SetNoiseAlpha(float a){noise_alpha=a;}                 ///< 噪声底更新速度（默认 0.1）
        void SetSpeechThreshold(float db){speech_threshold_db=db;}  ///< 语音判定阈值：帧能量高于噪声底+此值视为语音帧，不更新噪声底（默认 10dB）
        void SetSubtraction(float alpha,float floor){sub_alpha=alpha;sub_floor=floor;}  ///< 谱减强度（默认 1.5/0.02）

        void Reset();

        void Process(const float *in,float *out);   ///< 一帧降噪（frame_samples 样本）
    };//class NoiseSuppressor

    /**
    * 自动增益控制（AGC，P3）
    *
    * 帧 RMS 拉向目标电平：gain = target_rms/rms，attack/release 平滑防突变，
    * 输出软限幅防削波。
    */
    class AutoGainControl
    {
        float target_rms;               ///< 目标 RMS（线性）
        float attack;                   ///< 增益上升系数（0..1，大=快）
        float release;                  ///< 增益下降系数
        float cur_gain;                 ///< 当前增益（平滑状态）

    public:

        AutoGainControl();

        void Init(float target_rms_linear=0.1f);        ///< 目标 RMS（默认 -20dBFS≈0.1）

        void SetTargetRMS(float rms_linear){target_rms=rms_linear;}
        void SetAttackRelease(float a,float r){attack=a;release=r;}

        void Reset();

        void Process(const float *in,float *out,uint frame_samples);   ///< 一帧增益控制
    };//class AutoGainControl

    /**
    * 语音活动检测（VAD，P3）
    *
    * 帧 RMS（dB）与自适应噪声底比较：高于阈值判语音。
    * hangover 帧数防止语音尾部被切碎。
    */
    class VoiceActivityDetector
    {
        float noise_floor_db;           ///< 噪声底（dBFS，指数平滑）
        float threshold_db;             ///< 语音阈值（相对噪声底抬升量）
        int hangover;                   ///< hangover 帧数
        int hang_count;                 ///< 当前挂起计数
        float noise_alpha;              ///< 噪声底更新速度

    public:

        VoiceActivityDetector();

        void Init(float threshold_db=15.0f,int hangover_frames=5);  ///< 阈值=噪声底上抬 dB

        void Reset();

        bool Process(const float *in,uint frame_samples);   ///< 返回本帧是否语音
        float GetNoiseFloorDB()const{return noise_floor_db;}
    };//class VoiceActivityDetector

    /**
    * 语音预处理链（P3）：NS → AGC → VAD（侧路）
    *
    * 通话链输入端：CaptureSource 捕获帧 → VoicePreprocess →
    * （VAD 标志可挂载到编码策略：静音帧可跳过编码省带宽）→ AudioCodec::Encode
    */
    class VoicePreprocess
    {
        NoiseSuppressor     ns;
        AutoGainControl     agc;
        VoiceActivityDetector vad;

        std::vector<float> ns_out;

    public:

        void Init(uint sample_rate,uint frame_samples);

        void Reset();

        /**
        * 处理一帧（in→out 各 frame_samples 样本）
        * @return 本帧是否语音（VAD）
        */
        bool Process(const float *in,float *out,uint frame_samples);

        NoiseSuppressor &GetNS(){return ns;}
        AutoGainControl &GetAGC(){return agc;}
        VoiceActivityDetector &GetVAD(){return vad;}
    };//class VoicePreprocess
}//namespace hgl::audio
