#include<hgl/audio/AudioBank.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/utf.h>

#include<cstdio>
#include<cstring>
#include<cmath>

using namespace hgl;
using namespace hgl::audio;

namespace
{
    int failures=0;

    void Check(bool cond,const char *name)
    {
        std::printf("  [%s] %s\n",cond?"PASS":"FAIL",name);
        if(!cond)failures++;
    }

    // 构造一段伪 WAV 数据（合法 RIFF 头 + 短 PCM），作为"原始压缩数据"入包
    // 注意：bank 只搬运字节，不解析内容，伪数据足以验证格式层
    void MakeFakeWav(uint8 *buf,int &size)
    {
        struct WavHeader
        {
            char     riff[4];    // "RIFF"
            uint32   riff_size;
            char     wave[4];    // "WAVE"
            char     fmt[4];     // "fmt "
            uint32   fmt_size;
            uint16   audio_format;
            uint16   channels;
            uint32   sample_rate;
            uint32   byte_rate;
            uint16   block_align;
            uint16   bits;
            char     data[4];    // "data"
            uint32   data_size;
        };

        WavHeader h;

        memcpy(h.riff,"RIFF",4);
        memcpy(h.wave,"WAVE",4);
        memcpy(h.fmt,"fmt ",4);
        memcpy(h.data,"data",4);

        h.fmt_size=16;
        h.audio_format=1;               //PCM
        h.channels=1;
        h.sample_rate=44100;
        h.bits=16;
        h.block_align=uint16(h.channels*h.bits/8);
        h.byte_rate=h.sample_rate*h.block_align;
        h.data_size=512;                //256 个采样
        h.riff_size=4+8+16+h.data_size;

        memcpy(buf,&h,sizeof(h));

        for(int i=0;i<512;i++)
            buf[sizeof(h)+i]=uint8((i*7)&0xFF);     //伪 PCM 字节

        size=sizeof(h)+512;
    }

    // 伪 OGG 数据（非合法，仅字节搬运验证）
    void MakeFakeOgg(uint8 *buf,int &size)
    {
        static const char fake_ogg[]="OggS\x00\x02 fake-vorbis-data-0123456789";
        memcpy(buf,fake_ogg,sizeof(fake_ogg));
        size=sizeof(fake_ogg);
    }
}//namespace

int main(int,char **)
{
    std::printf("== AudioBank 读写测试 ==\n");

    const OSString filename=OS_TEXT("audio_bank_test.bank");

    // ---- 写入 ----
    {
        AudioBankWriter writer;

        uint8 wav_data[1024];
        uint8 ogg_data[128];
        int   wav_size,ogg_size;

        MakeFakeWav(wav_data,wav_size);
        MakeFakeOgg(ogg_data,ogg_size);

        Check(writer.AddEntry(OS_TEXT("ui_click"),OS_TEXT("ui"),AudioFileType::Wav,false,0.8f,wav_data,wav_size),"AddEntry ui_click");
        Check(writer.AddEntry(OS_TEXT("bgm_main"),OS_TEXT("music"),AudioFileType::Vorbis,true,1.0f,ogg_data,ogg_size),"AddEntry bgm_main");
        Check(writer.GetEntryCount()==2,"GetEntryCount==2");

        // 重名拒绝
        Check(!writer.AddEntry(OS_TEXT("ui_click"),OS_TEXT("x"),AudioFileType::Wav,false,1.0f,wav_data,wav_size),"重名 AddEntry 拒绝");

        // 非法参数拒绝
        Check(!writer.AddEntry(nullptr,OS_TEXT("x"),AudioFileType::Wav,false,1.0f,wav_data,wav_size),"null name 拒绝");
        Check(!writer.AddEntry(OS_TEXT("empty"),OS_TEXT("x"),AudioFileType::Wav,false,1.0f,nullptr,100),"null data 拒绝");
        Check(!writer.AddEntry(OS_TEXT("no_type"),OS_TEXT("x"),AudioFileType::None,false,1.0f,wav_data,wav_size),"None 类型拒绝");

        Check(writer.Write(filename),"Write .bank");
    }

    // ---- 读取 ----
    AudioBank bank;

    Check(bank.Load(filename),"Load .bank");
    Check(bank.GetEntryCount()==2,"读取条目数==2");

    if(bank.GetEntryCount()==2)
    {
        const AudioBankEntry *click=bank.FindEntry(OS_TEXT("ui_click"));
        const AudioBankEntry *bgm  =bank.FindEntry(OS_TEXT("bgm_main"));

        Check(click!=nullptr,"FindEntry ui_click");
        Check(bgm!=nullptr,"FindEntry bgm_main");

        if(click)
        {
            Check(click->group==OS_TEXT("ui"),"ui_click group==ui");
            Check(click->file_type==AudioFileType::Wav,"ui_click file_type==Wav");
            Check(!click->loop,"ui_click loop==false");
            Check(fabsf(click->gain-0.8f)<1e-6f,"ui_click gain==0.8");
            Check(click->name_hash==CueNameHash("ui_click"),"ui_click name_hash==CueNameHash");

            // 数据一致性：写什么读什么
            uint8 expect[1024];
            int   expect_size;
            MakeFakeWav(expect,expect_size);

            const void *data=bank.GetEntryData(click);

            Check(data!=nullptr,"GetEntryData ui_click");
            if(data)
            {
                Check(memcmp(data,expect,expect_size)==0,"ui_click 数据字节一致");
                Check(click->data_size==static_cast<uint64>(expect_size),"ui_click data_size 一致");
            }
        }

        if(bgm)
        {
            Check(bgm->group==OS_TEXT("music"),"bgm_main group==music");
            Check(bgm->file_type==AudioFileType::Vorbis,"bgm_main file_type==Vorbis");
            Check(bgm->loop,"bgm_main loop==true");

            // 哈希查找
            Check(bank.FindEntryByHash(bgm->name_hash)==bgm,"FindEntryByHash bgm_main");
        }

        // 不存在的条目
        Check(bank.FindEntry(OS_TEXT("not_exist"))==nullptr,"FindEntry 不存在→nullptr");
        Check(bank.FindEntryByHash(0xDEADBEEF)==nullptr,"FindEntryByHash 不存在→nullptr");
        Check(bank.GetEntry(2)==nullptr,"GetEntry 越界→nullptr");
        Check(!bank.Contains(OS_TEXT("not_exist")),"Contains 不存在→false");
        Check(bank.Contains(OS_TEXT("ui_click")),"Contains ui_click→true");
    }

    // ---- 哈希兼容性（事件系统对接）----
    {
        const AudioBankEntry *click=bank.FindEntry(OS_TEXT("ui_click"));
        Check(click&&click->name_hash==CueNameHash("ui_click"),
              "name_hash 与 CueNameHash 兼容（事件系统可直接按名对接）");
    }

    // ---- 损坏文件容错 ----
    {
        // 魔数错误
        {
            AudioBank bad;
            FILE *f=fopen("bad_magic.bin","wb");
            if(f){fwrite("XXXXX",1,5,f);fclose(f);}

            Check(!bad.Load(OS_TEXT("bad_magic.bin")),"魔数错误→Load false");
        }

        // 版本错误
        {
            AudioBank bad;
            FILE *f=fopen("bad_ver.bin","wb");
            if(f)
            {
                fwrite("HGLBK",1,5,f);
                uint16 v=99; fwrite(&v,2,1,f);
                uint32 n=0;  fwrite(&n,4,1,f);
                fclose(f);
            }

            Check(!bad.Load(OS_TEXT("bad_ver.bin")),"版本错误→Load false");
        }

        // 不存在的文件
        {
            AudioBank bad;
            Check(!bad.Load(OS_TEXT("no_such_file.bank")),"不存在文件→Load false");
        }
    }

    std::printf("== 结果: %s (%d failures) ==\n",failures?"FAILED":"ALL PASSED",failures);
    return failures?1:0;
}
