#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<opusfile.h>
#include<opus.h>

using namespace hgl;
using namespace openal;

ALenum GetOpusFormat16(const int channels)
{
    switch(channels)
    {
        case 1: return(AL_FORMAT_MONO16);
        case 2: return(AL_FORMAT_STEREO16);
        case 4: return(AL_FORMAT_QUAD16);
        case 6: return(AL_FORMAT_51CHN16);
        case 7: return(AL_FORMAT_61CHN16);
        case 8: return(AL_FORMAT_71CHN16);
        default: return(0);
    }
}

ALenum GetOpusFormatFloat32(const int channels)
{
    switch(channels)
    {
        case 1: return(AL_FORMAT_MONO_FLOAT32);
        case 2: return(AL_FORMAT_STEREO_FLOAT32);
        default: return(0);
    }
}

ALvoid LoadOpus(ALbyte *memory, ALsizei memory_size,ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    OggOpusFile *of;

    int op_error;

    *format=0;
    *data=nullptr;
    *size=0;
    *freq=0;

    of=op_open_memory((const unsigned char *)memory,memory_size,&op_error);

    if(!of)
        return;

    const OpusHead *head=op_head(of,0);

    *format=GetOpusFormat16(head->channel_count);
    if(!*format)
    {
        op_free(of);
        return;
    }

    long pcm_total=op_pcm_total(of,-1)*head->channel_count;

    opus_int16 *out_buf=new opus_int16[pcm_total];
    int out_size;
    int out_total_size=0;

    while(-1)
    {
        out_size=op_read(of,out_buf+out_total_size,pcm_total-out_total_size,nullptr);

        if(out_size<=0)
            break;

        out_total_size+=out_size*head->channel_count;
    }

    *freq=head->input_sample_rate;
    *size=out_total_size*2;
    *data=out_buf;
}

void ClearOpus(ALenum,ALvoid *data,ALsizei,ALsizei)
{
    opus_int16 *pcm=(opus_int16 *)data;

    delete[] pcm;
}


ALvoid LoadOpusFloat32(ALbyte *memory, ALsizei memory_size,ALenum *format, float **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    OggOpusFile *of;

    int op_error;

    *format=0;
    *data=nullptr;
    *size=0;
    *freq=0;

    of=op_open_memory((const unsigned char *)memory,memory_size,&op_error);

    if(!of)
        return;

    const OpusHead *head=op_head(of,0);

    *format=GetOpusFormatFloat32(head->channel_count);
    if(!*format)
    {
        op_free(of);
        return;
    }

    long pcm_total=op_pcm_total(of,-1)*head->channel_count;

    float *out_buf=new float[pcm_total];
    int out_size;
    int out_total_size=0;

    while(-1)
    {
        out_size=op_read_float(of,out_buf+out_total_size,pcm_total-out_total_size,nullptr);

        if(out_size<=0)
            break;

        out_total_size+=out_size*head->channel_count;
    }

    *freq=head->input_sample_rate;
    *size=out_total_size*sizeof(float);
    *data=out_buf;
}

void ClearOpusFloat32(ALenum,float *data,ALsizei,ALsizei)
{
    delete[] data;
}

struct OpusStream
{
    OggOpusFile *of;
    const OpusHead *head;
};

void *OpenOpus(ALbyte *memory,ALsizei memory_size,ALenum *format,ALsizei *freq,double *total_time)
{
    OggOpusFile *of;

    int op_error;

    of=op_open_memory((const unsigned char *)memory,memory_size,&op_error);

    if(!of)
        return(nullptr);

    const OpusHead *head=op_head(of,0);

    *format=GetOpusFormat16(head->channel_count);
    if(!*format)
    {
        op_free(of);
        return(nullptr);
    }

    *freq=head->input_sample_rate;

    *total_time=double(op_pcm_total(of,-1))/double(head->input_sample_rate);

    OpusStream *os=new OpusStream;

    os->of=of;
    os->head=head;

    return os;
}

void CloseOpus(void *ptr)
{
    OpusStream *os=(OpusStream *)ptr;

    op_free(os->of);

    delete os;
}

uint ReadOpus(void *ptr,char *data,uint buf_max)
{
    OpusStream *os=(OpusStream *)ptr;
    int result;
    uint size=0;
    uint buf_left=buf_max/2;        //16位,所以要除2

    while(size<buf_max)
    {
        result=op_read(os->of,((opus_int16 *)data)+size,buf_left-size,nullptr);

        if(result>0)size+=result*os->head->channel_count;else
        if(result<=0)break;
    }

    return(size*2);
}

uint ReadOpusFloat32(void *ptr,float *data,uint buf_max)
{
    OpusStream *os=(OpusStream *)ptr;
    int result;
    uint size=0;
    uint buf_left=buf_max/sizeof(float);

    while(size<buf_max)
    {
        result=op_read_float(os->of,data+size,buf_left-size,nullptr);

        if(result>0)size+=result*os->head->channel_count;else
        if(result<=0)break;
    }

    return(size*sizeof(float));
}

void RestartOpus(void *ptr)
{
    OpusStream *os=(OpusStream *)ptr;

    op_pcm_seek(os->of,0);
}
//--------------------------------------------------------------------------------------------------
struct OutInterface2
{
    void (*Load)(ALbyte *,ALsizei,ALenum *,ALvoid **,ALsizei *,ALsizei *,ALboolean *);
    void (*Clear)(ALenum,ALvoid *,ALsizei,ALsizei);

    void *(*Open)(ALbyte *,ALsizei,ALenum *,ALsizei *,double *);
    void  (*Close)(void *);
    uint  (*Read)(void *,char *,uint);
    void  (*Restart)(void *);
};

static OutInterface2 out_interface_2=
{
    LoadOpus,
    ClearOpus,

    OpenOpus,
    CloseOpus,
    ReadOpus,
    RestartOpus
};

struct OutInterface3
{
    void (*Load)(ALbyte *,ALsizei,ALenum *,float **,ALsizei *,ALsizei *,ALboolean *);
    void (*Clear)(ALenum,float *,ALsizei,ALsizei);

    uint (*Read)(void *,float *,uint);
};

static OutInterface3 out_interface_3
{
    LoadOpusFloat32,
    ClearOpusFloat32,

    ReadOpusFloat32
};
//--------------------------------------------------------------------------------------------------
// 编码接口（ver=5）：PCM ↔ 压缩包 流式编解码（LibOpus encoder/decoder）
// 供实时通话链使用；Opus 原生支持 PLC（丢包隐藏）：Decode 传 packet=nullptr 即走 PLC
//--------------------------------------------------------------------------------------------------
void *OpenOpusEncoder(uint sample_rate,uint channels,uint bitrate,int *error)
{
    int err=0;
    OpusEncoder *enc=opus_encoder_create(sample_rate,channels,OPUS_APPLICATION_VOIP,&err);

    if(enc&&bitrate>0)
        opus_encoder_ctl(enc,OPUS_SET_BITRATE(bitrate));

    if(error)*error=err;

    return(enc);
}

int EncodeOpus(void *enc,const float *pcm,uint frame_samples,char *packet,uint packet_cap)
{
    return(opus_encode_float((OpusEncoder *)enc,pcm,frame_samples,(unsigned char *)packet,packet_cap));
}

void CloseOpusEncoder(void *enc)
{
    opus_encoder_destroy((OpusEncoder *)enc);
}

void *OpenOpusDecoder(uint sample_rate,uint channels,int *error)
{
    int err=0;
    OpusDecoder *dec=opus_decoder_create(sample_rate,channels,&err);

    if(error)*error=err;

    return(dec);
}

int DecodeOpus(void *dec,const char *packet,int packet_size,float *pcm,uint pcm_cap)
{
    if(!packet||packet_size<=0)         //丢包：PLC 隐藏
        return(opus_decode_float((OpusDecoder *)dec,nullptr,0,pcm,pcm_cap,0));

    return(opus_decode_float((OpusDecoder *)dec,(const unsigned char *)packet,packet_size,pcm,pcm_cap,0));
}

void CloseOpusDecoder(void *dec)
{
    opus_decoder_destroy((OpusDecoder *)dec);
}

struct OutInterface5
{
    void *(*OpenEncoder)(uint,uint,uint,int *);
    int   (*Encode)(void *,const float *,uint,char *,uint);
    void  (*CloseEncoder)(void *);

    void *(*OpenDecoder)(uint,uint,int *);
    int   (*Decode)(void *,const char *,int,float *,uint);
    void  (*CloseDecoder)(void *);
};

static OutInterface5 out_interface_5
{
    OpenOpusEncoder,
    EncodeOpus,
    CloseOpusEncoder,

    OpenOpusDecoder,
    DecodeOpus,
    CloseOpusDecoder
};
//--------------------------------------------------------------------------------------------------
#if HGL_OS != HGL_OS_Windows
const u16char plugin_intro[]=U16_TEXT("Opus 音频文件解码(使用操作系统内置解码器,2016-09-16)");
#else
const u16char plugin_intro[]=U16_TEXT("Opus 音频文件解码(LibOpus 1.1.3,LibOpusFile 0.7,LibOGG 1.3.2,2016-09-16)");
#endif//

uint32 GetPlugInVersion()
{
    return(5);        //版本号
                    //根据版本号取得不同的API
}

u16char * GetPlugInIntro()
{
    return((u16char *)plugin_intro);
}

bool GetPlugInInterface(uint32 ver,void *data)
{
    if(ver==2)
    {
        memcpy(data,&out_interface_2,sizeof(OutInterface2));
    }
    else
    if(ver==3)
    {
        memcpy(data,&out_interface_3,sizeof(OutInterface3));
    }
    else
    if(ver==5)
    {
        memcpy(data,&out_interface_5,sizeof(OutInterface5));
    }
    else
        return(false);

    return(true);
}

static const char *plugin_extensions[]={"opus"};

bool GetCapability(uint32 id,void *data)
{
    if(id==uint32(PlugInCapability::FileExtensions))
    {
        PlugInFileExtensions *fe=(PlugInFileExtensions *)data;

        fe->count=1;
        fe->items=plugin_extensions;

        return(true);
    }

    return(false);
}

static PlugInInterface pii=
{
    nullptr,

    GetPlugInVersion,
    GetPlugInIntro,

    GetPlugInInterface,
    GetCapability
};

HGL_PLUGIN_FUNC PlugInInterface *InitPlugIn()
{
    return &pii;
}
//--------------------------------------------------------------------------------------------------
