﻿#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<vorbis/vorbisfile.h>

using namespace hgl;
using namespace openal;

struct ogg_fileread
{
    unsigned char *data;
    size_t pos;
    size_t size;
};

size_t VorbisRead(void *buffer,size_t size,size_t count,void *ptr)
{
    ogg_fileread *obj=(ogg_fileread *)ptr;

    size*=count;

    if(obj->pos+size>obj->size)
        size=obj->size-obj->pos;

    if(size)
    {
        memcpy(buffer,obj->data+obj->pos,size);

        obj->pos+=size;
    }

    return(size);
}

int VorbisSeek(void *ptr,ogg_int64_t offset,int whence)
{
    ogg_fileread *obj=(ogg_fileread *)ptr;

    if(whence==SEEK_SET)
    {
        obj->pos=offset;
    }
    else
    if(whence==SEEK_CUR)
    {
        obj->pos+=offset;
    }else
    if(whence==SEEK_END)
    {
        obj->pos=obj->size+offset;
    }

    if(obj->pos>obj->size)obj->pos=obj->size;

    return(0);
}

int VorbisClose(void *)
{
    return(0);
}

long VorbisTell(void *ptr)
{
    return ((ogg_fileread *)ptr)->pos;
}

ALvoid LoadOGG(ALbyte *memory, ALsizei memory_size,ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    int             result;
    char *            ptr=nullptr;

    ov_callbacks    func;
    OggVorbis_File  ogg_stream;
    vorbis_info *   info;
    int             section;

    ogg_fileread    ogg_memory;

    func.read_func  =VorbisRead;
    func.close_func =VorbisClose;
    func.seek_func  =VorbisSeek;
    func.tell_func  =VorbisTell;

    ogg_memory.data=(unsigned char *)memory;
    ogg_memory.pos=0;
    ogg_memory.size=memory_size;

    *size=0;

    if(ov_open_callbacks(&ogg_memory,&ogg_stream,NULL,0,func)!=0)
        return;

    info    =ov_info(&ogg_stream,-1);

    if(info->channels==1)*format=AL_FORMAT_MONO16;
                    else *format=AL_FORMAT_STEREO16;

    const int pcm_total = ov_pcm_total(&ogg_stream,0)*info->channels*2;

    int out_size = 0;

    ptr = new char[pcm_total];

    while(true)
    {
        result=ov_read(&ogg_stream,ptr+out_size,pcm_total-out_size,0,2,1,&section);

        if(result<=0)break;

        out_size += result;
    }

    *freq=info->rate;
    *size=out_size;
    *data=ptr;

    ov_clear(&ogg_stream);
}

void ClearOGG(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (char *)data;
}
//--------------------------------------------------------------------------------------------------
struct OggStream
{
    ov_callbacks    func;

    ogg_fileread    ogg_memory;
    OggVorbis_File    ogg_stream;
};

void *OpenOGG(ALbyte *memory,ALsizei memory_size,ALenum *format,ALsizei *rate,double *total_time)
{
    vorbis_info *info;

    OggStream *ptr=new OggStream;

    ptr->func.read_func  =VorbisRead;
    ptr->func.close_func =VorbisClose;
    ptr->func.seek_func  =VorbisSeek;
    ptr->func.tell_func  =VorbisTell;

    ptr->ogg_memory.data=(unsigned char *)memory;
    ptr->ogg_memory.pos=0;
    ptr->ogg_memory.size=memory_size;

    if(ov_open_callbacks(&(ptr->ogg_memory),&(ptr->ogg_stream),NULL,0,ptr->func)!=0)
        return(nullptr);

    info    =ov_info(&(ptr->ogg_stream),-1);

    if(info->channels==1)*format=AL_FORMAT_MONO16;
                    else *format=AL_FORMAT_STEREO16;

    *rate=info->rate;

    *total_time=ov_time_total(&(ptr->ogg_stream),-1);

    return(ptr);
}

void CloseOGG(void *ptr)
{
    OggStream *obj=(OggStream *)ptr;

    ov_clear(&(obj->ogg_stream));

    delete obj;
}

uint ReadOGG(void *ptr,char *data,uint buf_max)
{
    OggStream *obj=(OggStream *)ptr;
    int section;
    int result;
    uint size=0;

    while(size<buf_max)
    {
        result=ov_read(&(obj->ogg_stream),data+size,buf_max-size,0,2,1,&section);

        if(result>0)size+=result;else
        if(result<0)return(0);else break;
    }

    if(size==0)return(0);

    return(size);
}

void RestartOGG(void *ptr)
{
    OggStream *obj=(OggStream *)ptr;

    ov_pcm_seek(&(obj->ogg_stream),0);
}
//--------------------------------------------------------------------------------------------------
struct OutInterface
{
    void (*Load)(ALbyte *,ALsizei,ALenum *,ALvoid **,ALsizei *,ALsizei *,ALboolean *);
    void (*Clear)(ALenum,ALvoid *,ALsizei,ALsizei);

    void *(*Open)(ALbyte *,ALsizei,ALenum *,ALsizei *,double *);
    void  (*Close)(void *);
    uint  (*Read)(void *,char *,uint);
    void  (*Restart)(void *);
};

static OutInterface out_interface=
{
    LoadOGG,
    ClearOGG,

    OpenOGG,
    CloseOGG,
    ReadOGG,
    RestartOGG
};
//--------------------------------------------------------------------------------------------------
#if HGL_OS != HGL_OS_Windows
const u16char plugin_intro[]=U16_TEXT("Vorbis OGG 音频文件解码(使用操作系统内置解码器,2016-09-16)");
#else
const u16char plugin_intro[]=U16_TEXT("Vorbis OGG 音频文件解码(LibOGG 1.3.2,LibVorbis 1.3.5,MMX,2016-09-16)");
#endif//

HGL_PLUGIN_FUNC uint32 GetPlugInVersion()
{
    return(2);        //版本号
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
        memcpy(data,&out_interface,sizeof(OutInterface));
    }
    else
        return(false);

    return(true);
}

static PlugInInterface pii=
{
    nullptr,

    GetPlugInVersion,
    GetPlugInIntro,

    GetPlugInInterface,
    nullptr,

    nullptr,
    nullptr
};

HGL_PLUGIN_FUNC PlugInInterface *InitPlugIn()
{
    return &pii;
}
//--------------------------------------------------------------------------------------------------
