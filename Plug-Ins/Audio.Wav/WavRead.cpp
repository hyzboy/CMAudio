#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<string.h>
#include<malloc.h>
#include<hgl/al/al.h>

using namespace hgl;
using namespace openal;

typedef struct                                  /* WAV File-header */
{
    ALubyte  Id[4]            ;
    ALsizei  Size                ;
    ALubyte  Type[4]            ;
}
WAVFileHdr_Struct;

typedef struct                                  /* WAV Fmt-header */
{
    ALushort Format            ;
    ALushort Channels            ;
    ALuint   SamplesPerSec    ;
    ALuint   BytesPerSec        ;
    ALushort BlockAlign        ;
    ALushort BitsPerSample    ;
}
WAVFmtHdr_Struct;

typedef struct                                    /* WAV FmtEx-header */
{
    ALushort Size                ;
    ALushort SamplesPerBlock    ;
}
WAVFmtExHdr_Struct;

typedef struct                                  /* WAV Smpl-header */
{
    ALuint   Manufacturer        ;
    ALuint   Product            ;
    ALuint   SamplePeriod        ;
    ALuint   Note                ;
    ALuint   FineTune            ;
    ALuint   SMPTEFormat        ;
    ALuint   SMPTEOffest        ;
    ALuint   Loops            ;
    ALuint   SamplerData        ;

    struct
    {
        ALuint Identifier        ;
        ALuint Type                ;
        ALuint Start            ;
        ALuint End                ;
        ALuint Fraction            ;
        ALuint Count            ;
    }
    Loop[1]            ;
}
WAVSmplHdr_Struct;

typedef struct                                  /* WAV Chunk-header */
{
    ALubyte  Id[4]            ;
    ALuint   Size                ;
}
WAVChunkHdr_Struct;

ALenum GetWAVFormat(const ALushort channels,const ALushort bits)
{
    switch(channels)
    {
        case 1: return(bits==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16);
        case 2: return(bits==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16);
        case 4: return(bits==8?AL_FORMAT_QUAD8:AL_FORMAT_QUAD16);
        case 6: return(bits==8?AL_FORMAT_51CHN8:AL_FORMAT_51CHN16);
        case 7: return(bits==8?AL_FORMAT_61CHN8:AL_FORMAT_61CHN16);
        case 8: return(bits==8?AL_FORMAT_71CHN8:AL_FORMAT_71CHN16);
        default: return(0);
    }
}

ALvoid alutLoadWAVMemory(ALbyte *memory, ALsizei memory_size,ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    WAVChunkHdr_Struct ChunkHdr;
    WAVFmtExHdr_Struct FmtExHdr;
    WAVFileHdr_Struct FileHdr;
    WAVSmplHdr_Struct SmplHdr;
    WAVFmtHdr_Struct FmtHdr;
    ALbyte *Stream;

    *format=0;
    *data=NULL;
    *size=0;
    *freq=0;
    *loop=AL_FALSE;

    if (memory)
    {
        Stream=memory;

        if (Stream)
        {
            memcpy(&FileHdr, Stream, sizeof(WAVFileHdr_Struct));
            Stream+=sizeof(WAVFileHdr_Struct);
            FileHdr.Size=((FileHdr.Size+1)&~1)-4;
            while ((FileHdr.Size!=0)&&(memcpy(&ChunkHdr, Stream, sizeof(WAVChunkHdr_Struct))))
            {
                Stream+=sizeof(WAVChunkHdr_Struct);
                if (!memcmp(ChunkHdr.Id, "fmt ", 4))
                {
                    memcpy(&FmtHdr, Stream, sizeof(WAVFmtHdr_Struct));
                    if (FmtHdr.Format==0x0001)
                    {
                        *format=GetWAVFormat(FmtHdr.Channels,FmtHdr.BitsPerSample);
                        *freq=FmtHdr.SamplesPerSec;
                        Stream+=ChunkHdr.Size;
                    }
                    else
                    {
                        memcpy(&FmtExHdr, Stream, sizeof(WAVFmtExHdr_Struct));
                        Stream+=ChunkHdr.Size;
                    }
                }
                else if (!memcmp(ChunkHdr.Id, "data", 4))
                {
                    if (FmtHdr.Format==0x0001)
                    {
                        *size=ChunkHdr.Size;
                        *data=malloc(ChunkHdr.Size+31);
                        if (*data) memcpy(*data, Stream, ChunkHdr.Size);
                        memset(((char *)*data)+ChunkHdr.Size, 0, 31);
                        Stream+=ChunkHdr.Size;
                    }
                    else if (FmtHdr.Format==0x0011)
                    {
                        //IMA ADPCM
                    }
                    else if (FmtHdr.Format==0x0055)
                    {
                        //MP3 WAVE
                    }
                }
                else if (!memcmp(ChunkHdr.Id, "smpl", 4))
                {
                    memcpy(&SmplHdr, Stream, sizeof(WAVSmplHdr_Struct));
                    *loop = (SmplHdr.Loops ? AL_TRUE : AL_FALSE);
                    Stream+=ChunkHdr.Size;
                }
                else
                    Stream+=ChunkHdr.Size;
                Stream+=ChunkHdr.Size&1;
                FileHdr.Size-=(((ChunkHdr.Size+1)&~1)+8);
            }
        }
    }
}

ALvoid alutUnloadWAV(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        free(data);
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
    alutLoadWAVMemory,
    alutUnloadWAV,

    NULL,
    NULL,
    NULL,
    NULL
};
//--------------------------------------------------------------------------------------------------
const u16char plugin_intro[]=U16_TEXT("WAV音频文件解码(2014-04-09,代码源自ALUT)");

uint32 GetPlugInVersion()
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

static const char *plugin_extensions[]={"wav"};

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
