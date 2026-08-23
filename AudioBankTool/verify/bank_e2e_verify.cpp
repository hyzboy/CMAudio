// 端到端验证：真实 WAV → AudioBankWriter 打包 → AudioBank 读回 → AudioPlayer 播放
// 编译为独立 exe（bank_e2e_verify），验证工具导出的 .bank 可被运行时加载播放
#include <hgl/audio/AudioBank.h>
#include <hgl/audio/AudioPlayer.h>
#include <hgl/audio/OpenAL.h>
#include "../../src/AudioDecode.h"
#include <hgl/io/MemoryInputStream.h>
#include <hgl/CoreType.h>
#include <hgl/type/String.h>

#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace hgl;
using hgl::audio::AudioBank;
using hgl::audio::AudioBankWriter;
using hgl::audio::AudioFileType;
using hgl::io::MemoryInputStream;

namespace
{
    int failures=0;

    void Check(bool cond,const char *name)
    {
        std::printf("  [%s] %s\n",cond?"PASS":"FAIL",name);
        std::fflush(stdout);
        if(!cond)failures++;
    }

    // 生成 1 秒 440Hz 16bit 单声道正弦 WAV 到文件
    bool GenerateSineWav(const char *path)
    {
        constexpr int sample_rate=44100;
        constexpr int seconds=1;
        constexpr int samples=sample_rate*seconds;
        constexpr int data_size=samples*2;

        FILE *f=fopen(path,"wb");
        if(!f)return false;

        // RIFF 头
        fwrite("RIFF",1,4,f);
        uint32_t riff_size=36+data_size;
        fwrite(&riff_size,4,1,f);
        fwrite("WAVE",1,4,f);
        fwrite("fmt ",1,4,f);
        uint32_t fmt_size=16;
        fwrite(&fmt_size,4,1,f);
        uint16_t audio_format=1, channels=1;
        fwrite(&audio_format,2,1,f);
        fwrite(&channels,2,1,f);
        uint32_t rate=sample_rate, byte_rate=sample_rate*2;
        fwrite(&rate,4,1,f);
        fwrite(&byte_rate,4,1,f);
        uint16_t block_align=2, bits=16;
        fwrite(&block_align,2,1,f);
        fwrite(&bits,2,1,f);
        fwrite("data",1,4,f);
        uint32_t ds=data_size;
        fwrite(&ds,4,1,f);

        // 正弦 PCM
        for(int i=0;i<samples;i++)
        {
            const double t=double(i)/sample_rate;
            const int16_t v=static_cast<int16_t>(32767*0.5*sin(2.0*3.14159265358979*440.0*t));
            fwrite(&v,2,1,f);
        }

        fclose(f);
        return true;
    }
}//namespace

int main(int,char **)
{
    std::printf("== AudioBank 端到端验证（真实 WAV → 打包 → 读回 → 播放）==\n");

    const char *wav_path="e2e_tone.wav";
    const char *bank_path="e2e_test.bank";

    // 1. 生成真实 WAV
    Check(GenerateSineWav(wav_path),"生成 440Hz 正弦 WAV");

    // 2. 读入内存
    FILE *f=fopen(wav_path,"rb");
    Check(f!=nullptr,"打开 WAV");
    if(!f)return 1;

    fseek(f,0,SEEK_END);
    const long wav_size=ftell(f);
    fseek(f,0,SEEK_SET);

    uint8_t *wav_data=new uint8_t[wav_size];
    const size_t rd=fread(wav_data,1,wav_size,f);
    fclose(f);
    Check(rd==static_cast<size_t>(wav_size),"读取 WAV 完整");

    // 3. AudioBankWriter 打包
    {
        AudioBankWriter writer;

        Check(writer.AddEntry(OS_TEXT("sine_440"),OS_TEXT("test"),AudioFileType::Wav,false,0.9f,wav_data,wav_size),
              "AddEntry 真实 WAV");

        const OSString out_path=OS_TEXT("e2e_test.bank");
        Check(writer.Write(out_path),"写入 .bank");
        Check(writer.GetEntryCount()==1,"条目数==1");
    }

    // 4. AudioBank 读回
    AudioBank bank;
    Check(bank.Load(OS_TEXT("e2e_test.bank")),"Load .bank");

    const hgl::audio::AudioBankEntry *entry=bank.FindEntry(OS_TEXT("sine_440"));
    Check(entry!=nullptr,"FindEntry sine_440");

    const void *bank_data=nullptr;
    if(entry)
    {
        Check(entry->file_type==AudioFileType::Wav,"file_type==Wav");
        Check(entry->gain>0.899f&&entry->gain<0.901f,"gain==0.9");
        Check(!entry->loop,"loop==false");

        bank_data=bank.GetEntryData(entry);
        Check(bank_data!=nullptr,"GetEntryData 非空");
        Check(entry->data_size==static_cast<uint64>(wav_size),"data_size==原始 WAV 大小");

        if(bank_data)
            Check(memcmp(bank_data,wav_data,wav_size)==0,"bank 数据与原始 WAV 字节一致");
    }

    // 5. AudioPlayer 从 bank 内存数据播放（null 后端，无需音频设备）
    {
        Check(openal::InitOpenAL(nullptr,"null",false,false),"InitOpenAL null 后端");
        std::printf("  [info] InitOpenAL 完成\n"); std::fflush(stdout);

        // 5a. 先手动测解码插件加载链路（定位 Load 崩溃点）
        {
            std::printf("  [info] alGenSources=%p alGenBuffers=%p\n",
                        (void*)openal::alGenSources,(void*)openal::alGenBuffers);
            std::fflush(stdout);

            hgl::audio::AudioPlugInInterface dec{};
            const bool got=hgl::audio::GetAudioInterface(OS_TEXT("Wav"),&dec,nullptr);
            Check(got,"GetAudioInterface 手动加载 Wav 插件");

            std::printf("  [info] dec.Open=%p Read=%p Close=%p Clear=%p Load=%p Restart=%p\n",
                        (void*)dec.Open,(void*)dec.Read,(void*)dec.Close,(void*)dec.Clear,(void*)dec.Load,(void*)dec.Restart);
            std::fflush(stdout);
        }

        hgl::audio::AudioPlayer player;
        std::printf("  [info] AudioPlayer 构造完成\n"); std::fflush(stdout);

        bool load_ok=false;
        double total_time=0;

        if(entry&&bank_data)
        {
            std::printf("  [info] 开始 Load...\n"); std::fflush(stdout);
            MemoryInputStream mis(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(bank_data)),
                                  static_cast<int64>(entry->data_size));
            load_ok=player.Load(&mis,static_cast<int>(entry->data_size),entry->file_type);
            std::printf("  [info] Load 返回\n"); std::fflush(stdout);
            total_time=player.GetTotalTime();
        }

        Check(load_ok,"AudioPlayer 从 bank 内存数据加载");
        Check(total_time>0.9&&total_time<1.1,"时长≈1 秒（解码正确）");

        if(load_ok)
        {
            player.SetLoop(false);
            player.Play(false);
            Check(player.GetPlayState()==hgl::audio::PlayState::Play||player.GetPlayState()==hgl::audio::PlayState::None,
                  "Play 调用（null 后端状态合法）");
            std::printf("  [info] 播放状态=%d 已播=%.3fs\n",
                        static_cast<int>(player.GetPlayState()),player.GetPlayTime());
        }

        openal::CloseOpenAL();
    }

    delete[] wav_data;

    std::printf("== 结果: %s (%d failures) ==\n",failures?"FAILED":"ALL PASSED",failures);
    return failures?1:0;
}
