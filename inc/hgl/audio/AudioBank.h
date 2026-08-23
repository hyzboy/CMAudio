#pragma once

#include<hgl/CoreType.h>
#include<hgl/type/String.h>
#include<hgl/audio/AudioFileType.h>

namespace hgl::audio
{
    /**
    * 音频库条目元数据（AudioBank，P0）
    *
    * 每条目对应一个原始音频文件（wav/ogg 等，压缩数据原样入包），
    * 运行时按需解码（走 CMAudio 解码插件），bank 本身不做解码。
    *
    * 名字哈希用 FNV1a（与 CueNameHash 一致），可直接与事件系统
    * 的 Cue 名哈希对接：Play(cue_name) → 哈希 → bank 条目。
    */
    struct AudioBankEntry
    {
        uint32          name_hash;      ///< FNV1a(UTF-8 name)，与 CueNameHash 一致
        OSString        name;           ///< 条目名（UTF-8 存储，API 层 OSString）
        uint32          group_hash;     ///< FNV1a(UTF-8 group)
        OSString        group;          ///< 分组名（可为空）
        AudioFileType   file_type;      ///< 音频文件格式（决定用哪个解码插件）
        bool            loop;           ///< 是否循环播放
        float           gain;           ///< 默认增益（0.0~1.0）
        uint64          data_offset;    ///< 原始数据在 bank 文件内的偏移（字节）
        uint64          data_size;      ///< 原始数据字节数
    };//struct AudioBankEntry

    /**
    * 音频库读取器：加载 .bank 文件，按名/哈希查找条目
    *
    * - 整个文件读入内存（bank 通常不大），条目数据零拷贝访问
    * - FindEntry 按 name_hash 精确匹配（FNV1a 32 位）
    * - GetEntryData 返回原始压缩数据，配合 DecodeAudio/插件解码
    */
    class AudioBank
    {
        uint8           *file_data;         ///< 整个 bank 文件内存
        uint64           file_size;         ///< 文件大小

        AudioBankEntry  *entries;           ///< 条目目录
        int              entry_count;       ///< 条目数

        void Clear();                       ///< 释放全部资源

    public:

        AudioBank();
        ~AudioBank();

        AudioBank(const AudioBank &)=delete;
        AudioBank &operator=(const AudioBank &)=delete;

        /**
        * 从文件加载 .bank（整个读入内存）
        * @return 是否成功（文件不存在/魔数错误/版本不兼容返回 false）
        */
        bool Load(const os_char *filename);

        int  GetEntryCount()const{return entry_count;}
        const AudioBankEntry *GetEntry(int index)const;                         ///< 按索引取条目（越界返回 nullptr）

        const AudioBankEntry *FindEntry(const os_char *name)const;              ///< 按名字查找（空名字/未命中返回 nullptr）
        const AudioBankEntry *FindEntryByHash(uint32 name_hash)const;           ///< 按名字哈希查找

        /**
        * 取条目原始压缩数据（指针指向 bank 文件内存，零拷贝）
        * @param entry 条目（必须来自本 bank 的 GetEntry/FindEntry）
        * @return 原始数据指针；entry 为空返回 nullptr
        */
        const void *GetEntryData(const AudioBankEntry *entry)const;

        bool Contains(const os_char *name)const{return FindEntry(name)!=nullptr;}
        bool ContainsHash(uint32 name_hash)const{return FindEntryByHash(name_hash)!=nullptr;}
    };//class AudioBank

    /**
    * 音频库写入器（Authoring 工具用）：收集条目并序列化为 .bank
    *
    * 用法：AddEntry(...) × N → Write(filename)
    * 数据为原始压缩字节（调用方读取 wav/ogg 文件内容传入）
    */
    class AudioBankWriter
    {
        struct PendingEntry
        {
            OSString      name;
            OSString      group;
            uint32        name_hash;
            AudioFileType file_type;
            bool          loop;
            float         gain;
            uint8        *data;
            uint64        data_size;

            PendingEntry();
            ~PendingEntry();
            PendingEntry(const PendingEntry &)=delete;
            PendingEntry &operator=(const PendingEntry &)=delete;
        };//struct PendingEntry

        PendingEntry **entries;         ///< 待写入条目
        int            entry_count;
        int            entry_capacity;

        bool Reserve(int need);

    public:

        AudioBankWriter();
        ~AudioBankWriter();

        AudioBankWriter(const AudioBankWriter &)=delete;
        AudioBankWriter &operator=(const AudioBankWriter &)=delete;

        /**
        * 添加一个条目（数据会被拷贝，调用方随后可释放原指针）
        * @param name 条目名（非空；重复名返回 false）
        * @param group 分组（可空）
        * @param file_type 音频文件格式
        * @param loop 是否循环
        * @param gain 默认增益
        * @param data 原始压缩数据
        * @param data_size 数据字节数
        */
        bool AddEntry(const os_char *name,const os_char *group,AudioFileType file_type,
                      bool loop,float gain,const void *data,uint64 data_size);

        int  GetEntryCount()const{return entry_count;}

        /**
        * 序列化写入 .bank 文件（覆盖已存在文件）
        * @return 是否成功
        */
        bool Write(const os_char *filename)const;
    };//class AudioBankWriter

    /** 当前 bank 格式版本 */
    constexpr uint16 AudioBankVersion=1;
}//namespace hgl::audio
