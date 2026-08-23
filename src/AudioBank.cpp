#include<hgl/audio/AudioBank.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/io/FileOutputStream.h>
#include<hgl/io/DataInputStream.h>
#include<hgl/io/DataOutputStream.h>
#include<hgl/io/EndianDataInputStream.h>
#include<hgl/io/EndianDataOutputStream.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/utf.h>

namespace hgl::audio
{
    using hgl::io::DirectDataInputStream;
    using hgl::io::DirectDataOutputStream;
    using hgl::io::FileInputStream;
    using hgl::io::FileOutputStream;
    using hgl::io::MemoryInputStream;

    // ====================================================================
    // 格式布局（.bank v1，小端，UTF-8 名字）
    // --------------------------------------------------------------------
    // 魔数 "HGLBK" (5B)
    // uint16 版本
    // uint32 条目数 N
    // ── 条目目录 × N ──
    //   uint32  name_hash（FNV1a，与 CueNameHash 一致）
    //   uint16  name 长度（UTF-8 字节数）
    //   name UTF-8 字节
    //   uint32  group_hash
    //   uint16  group 长度
    //   group UTF-8 字节
    //   uint8   file_type（AudioFileType）
    //   uint8   loop
    //   float32 gain
    //   uint64  data_offset（相对文件头）
    //   uint64  data_size
    // ── 数据段 ──
    //   条目0 原始数据 ... 条目N-1 原始数据
    // ====================================================================

    constexpr char    AudioBankMagic[5]={'H','G','L','B','K'};

    namespace
    {
        uint32 HashName(const OSString &name)
        {
            const U8String u=ToU8String(name);

            return CueNameHash(reinterpret_cast<const char *>(u.c_str()));
        }

        bool WriteString(DirectDataOutputStream *dos,const OSString &str)
        {
            const U8String u=ToU8String(str);
            const uint32   len=static_cast<uint32>(u.Length());

            if(!dos->WriteUint16(static_cast<uint16>(len)))return(false);
            if(len>0)
            {
                if(dos->Write(u.c_str(),len)!=len)return(false);
            }
            return(true);
        }

        bool ReadString(DirectDataInputStream *dis,OSString &str)
        {
            uint16 len;

            if(!dis->ReadUint16(len))return(false);

            if(len==0)
            {
                str.Clear();
                return(true);
            }

            u8char *buf=new u8char[len];

            if(dis->Read(buf,len)!=len)
            {
                delete[] buf;
                return(false);
            }

            U8String u(buf,len);
            delete[] buf;

            str=ToOSString(u);
            return(true);
        }
    }//namespace

    // ====================================================================
    // AudioBankEntry / AudioBank（读取器）
    // ====================================================================

    AudioBank::AudioBank()
    {
        file_data=nullptr;
        file_size=0;
        entries=nullptr;
        entry_count=0;
    }

    AudioBank::~AudioBank()
    {
        Clear();
    }

    void AudioBank::Clear()
    {
        delete[] file_data;
        delete[] entries;

        file_data=nullptr;
        file_size=0;
        entries=nullptr;
        entry_count=0;
    }

    bool AudioBank::Load(const os_char *filename)
    {
        Clear();

        FileInputStream fis;

        if(!fis.Open(filename))
            return(false);

        const int64 fsize=fis.GetSize();

        if(fsize<5+2+4)
            return(false);                                              //小于最小头

        uint8 *data=new uint8[fsize];

        if(fis.ReadFully(data,fsize)!=fsize)
        {
            delete[] data;
            return(false);
        }

        MemoryInputStream mis(data,fsize);
        DirectDataInputStream dis(&mis);

        char magic[5];

        if(dis.ReadFully(magic,5)!=5
         ||memcmp(magic,AudioBankMagic,5)!=0)
        {
            delete[] data;
            return(false);                                              //魔数错误
        }

        uint16 version;
        uint32 count;

        if(!dis.ReadUint16(version)||version!=AudioBankVersion
         ||!dis.ReadUint32(count)||count>0xFFFF)                        //条目数上限保护
        {
            delete[] data;
            return(false);
        }

        AudioBankEntry *ents=new AudioBankEntry[count];

        for(uint32 i=0;i<count;i++)
        {
            AudioBankEntry &e=ents[i];

            if(!dis.ReadUint32(e.name_hash)
             ||!ReadString(&dis,e.name)
             ||!dis.ReadUint32(e.group_hash)
             ||!ReadString(&dis,e.group))
            {
                delete[] ents;
                delete[] data;
                return(false);
            }

            uint8 file_type_u8;
            uint8 loop_u8;

            if(!dis.ReadUint8(file_type_u8)
             ||!dis.ReadUint8(loop_u8)
             ||!dis.ReadFloat(e.gain)
             ||!dis.ReadUint64(e.data_offset)
             ||!dis.ReadUint64(e.data_size))
            {
                delete[] ents;
                delete[] data;
                return(false);
            }

            e.file_type=static_cast<AudioFileType>(file_type_u8);
            e.loop    =(loop_u8!=0);

            if(static_cast<uint64>(e.data_offset)+e.data_size>static_cast<uint64>(fsize))
            {
                delete[] ents;
                delete[] data;
                return(false);                                          //数据越界
            }
        }

        //成功：移交内存
        file_data=data;
        file_size=fsize;
        entries=ents;
        entry_count=static_cast<int>(count);
        return(true);
    }

    const AudioBankEntry *AudioBank::GetEntry(int index)const
    {
        if(index<0||index>=entry_count)return(nullptr);
        return(entries+index);
    }

    const AudioBankEntry *AudioBank::FindEntry(const os_char *name)const
    {
        if(!name||!name[0])return(nullptr);

        return FindEntryByHash(HashName(name));
    }

    const AudioBankEntry *AudioBank::FindEntryByHash(uint32 name_hash)const
    {
        for(int i=0;i<entry_count;i++)
        {
            if(entries[i].name_hash==name_hash)
                return(entries+i);
        }
        return(nullptr);
    }

    const void *AudioBank::GetEntryData(const AudioBankEntry *entry)const
    {
        if(!entry)return(nullptr);

        //校验条目属于本 bank（地址区间检查）
        if(entry<entries||entry>=entries+entry_count)
            return(nullptr);

        return(file_data+entry->data_offset);
    }

    // ====================================================================
    // AudioBankWriter（写入器）
    // ====================================================================

    AudioBankWriter::PendingEntry::PendingEntry()
    {
        data=nullptr;
        data_size=0;
        file_type=AudioFileType::None;
        loop=false;
        gain=1.0f;
    }

    AudioBankWriter::PendingEntry::~PendingEntry()
    {
        delete[] data;
    }

    AudioBankWriter::AudioBankWriter()
    {
        entries=nullptr;
        entry_count=0;
        entry_capacity=0;
    }

    AudioBankWriter::~AudioBankWriter()
    {
        for(int i=0;i<entry_count;i++)
            delete entries[i];

        delete[] entries;
    }

    bool AudioBankWriter::Reserve(int need)
    {
        if(need<=entry_capacity)return(true);

        const int new_cap=entry_capacity?entry_capacity*2:16;

        PendingEntry **new_arr=new PendingEntry *[new_cap];

        for(int i=0;i<entry_count;i++)
            new_arr[i]=entries[i];

        delete[] entries;
        entries=new_arr;
        entry_capacity=new_cap;

        return Reserve(need);
    }

    bool AudioBankWriter::AddEntry(const os_char *name,const os_char *group,AudioFileType file_type,
                                   bool loop,float gain,const void *data,uint64 data_size)
    {
        if(!name||!name[0])return(false);
        if(!data||data_size==0)return(false);
        if(file_type==AudioFileType::None)return(false);

        //重名检查
        const uint32 name_hash=HashName(name);

        for(int i=0;i<entry_count;i++)
        {
            if(entries[i]->name_hash==name_hash)
                return(false);
        }

        if(!Reserve(entry_count+1))return(false);

        PendingEntry *pe=new PendingEntry();

        pe->name=name;
        pe->name_hash=name_hash;

        if(group)
            pe->group=group;

        pe->file_type=file_type;
        pe->loop=loop;
        pe->gain=gain;
        pe->data_size=data_size;
        pe->data=new uint8[data_size];
        memcpy(pe->data,data,data_size);

        entries[entry_count++]=pe;
        return(true);
    }

    bool AudioBankWriter::Write(const os_char *filename)const
    {
        FileOutputStream fos;

        if(!fos.CreateTrunc(filename))
            return(false);

        DirectDataOutputStream dos(&fos);

        //魔数 + 版本 + 条目数
        if(dos.Write(AudioBankMagic,5)!=5
         ||!dos.WriteUint16(AudioBankVersion)
         ||!dos.WriteUint32(static_cast<uint32>(entry_count)))
            return(false);

        //计算数据段起点：头 11B + 每条目录 (4+2+name+4+2+group+1+1+4+8+8)
        uint64 data_offset=5+2+4;

        for(int i=0;i<entry_count;i++)
        {
            const PendingEntry &e=*entries[i];

            const U8String u_name =ToU8String(e.name);
            const U8String u_group=ToU8String(e.group);

            data_offset+=4+2+u_name.Length()+4+2+u_group.Length()+1+1+4+8+8;
        }

        //条目目录
        for(int i=0;i<entry_count;i++)
        {
            const PendingEntry &e=*entries[i];

            const U8String u_name =ToU8String(e.name);
            const U8String u_group=ToU8String(e.group);

            if(!dos.WriteUint32(HashName(e.name))
             ||!WriteString(&dos,e.name)
             ||!dos.WriteUint32(HashName(e.group))
             ||!WriteString(&dos,e.group))
                return(false);

            if(!dos.WriteUint8(static_cast<uint8>(e.file_type))
             ||!dos.WriteUint8(e.loop?1:0)
             ||!dos.WriteFloat(e.gain)
             ||!dos.WriteUint64(data_offset)
             ||!dos.WriteUint64(e.data_size))
                return(false);

            data_offset+=e.data_size;
        }

        //数据段
        for(int i=0;i<entry_count;i++)
        {
            const PendingEntry &e=*entries[i];

            if(dos.Write(e.data,e.data_size)!=static_cast<int64>(e.data_size))
                return(false);
        }

        fos.Close();
        return(true);
    }
}//namespace hgl::audio
