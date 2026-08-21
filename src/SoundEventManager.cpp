#include<hgl/audio/SoundEventManager.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/utf.h>
#include<hgl/type/StdString.h>      // ToOSString(std::string)

#include<fstream>
#include<sstream>
#include<string>
#include<vector>

namespace hgl::audio
{
    namespace
    {
        // OSString(u16) → FNV1a 哈希（与 CueNameHash(UTF-8) 一致：先转 UTF-8 再哈希）
        uint32 HashEventKey(const OSString &s)
        {
            const U8String u=ToU8String(s);

            return CueNameHash(reinterpret_cast<const char *>(u.c_str()));
        }
    }

    // ====================================================================
    // 简单 TOML 解析辅助（section + key-value + 字符串数组）
    // 风格与 examples/AudioMixerSceneConfig.h 一致，避免引入重型 TOML 库
    // ====================================================================
    namespace
    {
        std::string Trim(const std::string &str)
        {
            size_t first=str.find_first_not_of(" \t\r\n");
            if(first==std::string::npos)return "";
            size_t last=str.find_last_not_of(" \t\r\n");
            return str.substr(first,last-first+1);
        }

        std::string Unquote(const std::string &str)
        {
            std::string s=Trim(str);
            if(s.length()>=2&&s[0]=='\"'&&s[s.length()-1]=='\"')
                return s.substr(1,s.length()-2);
            return s;
        }

        float ParseFloat(const std::string &value)
        {
            try{return std::stof(Trim(value));}
            catch(...){return 0.0f;}
        }

        bool ParseBool(const std::string &value)
        {
            const std::string v=Trim(value);
            return v=="true"||v=="1"||v=="yes"||v=="on";
        }

        // 解析字符串数组 ["a.wav", "b.wav"] → 追加到 out
        void ParseStringArray(const std::string &value,std::vector<std::string> &out)
        {
            std::string s=Trim(value);

            if(s.length()>=2&&s[0]=='['&&s[s.length()-1]==']')
                s=s.substr(1,s.length()-2);

            std::stringstream ss(s);
            std::string item;

            while(std::getline(ss,item,','))
            {
                std::string u=Unquote(item);
                if(!u.empty())
                    out.push_back(u);
            }
        }
    }//namespace

    SoundEventManager::SoundEventManager()
    {
    }

    SoundEventManager::~SoundEventManager()
    {
        Clear();
    }

    bool SoundEventManager::AddEvent(const os_char *name,const SoundEventConfig &config)
    {
        if(!name||!(*name))return false;

        events[OSString(name)]=config;      // 覆盖语义

        return true;
    }

    bool SoundEventManager::RemoveEvent(const os_char *name)
    {
        if(!name||!(*name))return false;

        return events.DeleteByKey(OSString(name));
    }

    const SoundEventConfig *SoundEventManager::GetEvent(const os_char *name)const
    {
        if(!name||!(*name))return nullptr;

        return events.GetValuePointer(OSString(name));
    }

    const SoundEventConfig *SoundEventManager::GetEventByHash(uint32 hash)const
    {
        if(!hash)return nullptr;

        // 遍历事件表找哈希匹配（事件数通常几十，线性可接受）
        const SoundEventConfig *result=nullptr;

        events.EnumKeys([&](const OSString &key)
        {
            if(HashEventKey(key)==hash)
                result=events.GetValuePointer(key);
        });

        return result;
    }

    bool SoundEventManager::Contains(const os_char *name)const
    {
        if(!name||!(*name))return false;

        return events.ContainsKey(OSString(name));
    }

    int SoundEventManager::GetCount()const
    {
        return events.GetCount();
    }

    void SoundEventManager::Clear()
    {
        events.Clear();
        snapshots.Clear();
    }

    // ---- 混音快照（T2）----

    bool SoundEventManager::AddSnapshot(const os_char *name,const SnapshotConfig &config)
    {
        if(!name||!(*name))return false;

        snapshots[OSString(name)]=config;

        return true;
    }

    bool SoundEventManager::RemoveSnapshot(const os_char *name)
    {
        if(!name||!(*name))return false;

        return snapshots.DeleteByKey(OSString(name));
    }

    const SnapshotConfig *SoundEventManager::GetSnapshot(const os_char *name)const
    {
        if(!name||!(*name))return nullptr;

        return snapshots.GetValuePointer(OSString(name));
    }

    const SnapshotConfig *SoundEventManager::GetSnapshotByHash(uint32 hash)const
    {
        if(!hash)return nullptr;

        const SnapshotConfig *result=nullptr;

        snapshots.EnumKeys([&](const OSString &key)
        {
            if(HashEventKey(key)==hash)
                result=snapshots.GetValuePointer(key);
        });

        return result;
    }

    bool SoundEventManager::ContainsSnapshot(const os_char *name)const
    {
        if(!name||!(*name))return false;

        return snapshots.ContainsKey(OSString(name));
    }

    int SoundEventManager::GetSnapshotCount()const
    {
        return snapshots.GetCount();
    }

    bool SoundEventManager::LoadFromTOML(const char *filename)
    {
        if(!filename||!(*filename))return false;

        std::ifstream file(filename);
        if(!file.is_open())return false;

        std::string line;
        std::string current_event;
        std::string current_snapshot;
        SoundEventConfig current_config;
        SnapshotConfig current_snapshot_config;
        bool in_event=false;
        bool in_snapshot=false;
        bool in_snapshot_gain=false;
        bool in_rtpc=false;
        RTPCConfig current_rtpc;

        int loaded=0;

        while(std::getline(file,line))
        {
            line=Trim(line);

            if(line.empty()||line[0]=='#')continue;

            // 段头：[[数组表]] 或 [表]
            if(line[0]=='[')
            {
                const bool is_array=(line.size()>=2&&line[1]=='[');
                std::string section;

                if(is_array)
                    section=line.substr(2,line.length()-4);     // 去掉 [[ ]]
                else
                    section=line.substr(1,line.length()-2);     // 去掉 [ ]

                // RTPC 数组表 [[event.xxx.rtpc]]：属于当前事件，不退出事件上下文
                if(is_array&&section.rfind("event.",0)==0&&section.find(".rtpc")!=std::string::npos&&in_event)
                {
                    // 保存上一个 rtpc（若已有）
                    if(!current_rtpc.param.IsEmpty())
                        current_config.rtpc.push_back(current_rtpc);

                    current_rtpc=RTPCConfig();
                    in_rtpc=true;
                    continue;
                }

                // 普通段头：保存上一个事件
                if(in_event&&!current_event.empty())
                {
                    // flush 当前 rtpc
                    if(in_rtpc&&!current_rtpc.param.IsEmpty())
                        current_config.rtpc.push_back(current_rtpc);

                    AddEvent(ToOSString(current_event).c_str(),current_config);
                    ++loaded;
                }

                // 快照 bus_gain 子段：不保存上一个快照（续写当前快照）
                const bool is_bus_gain_sub=(!is_array&&section.rfind("snapshots.",0)==0
                                            &&section.find(".bus_gain")!=std::string::npos
                                            &&!current_snapshot.empty()
                                            &&section.substr(10,section.length()-10-9)==current_snapshot);

                // 保存上一个快照（bus_gain 子段除外）
                if(!is_bus_gain_sub&&in_snapshot&&!current_snapshot.empty())
                {
                    AddSnapshot(ToOSString(current_snapshot).c_str(),current_snapshot_config);
                    ++loaded;
                }

                in_event=false;
                in_snapshot=false;
                in_snapshot_gain=false;
                in_rtpc=false;

                // 事件段 [event.xxx]
                if(section.rfind("event.",0)==0)
                {
                    current_event=section.substr(6);
                    current_config=SoundEventConfig();
                    in_event=true;
                }
                // 快照段 [snapshots.xxx] 或 [snapshots.xxx.bus_gain]
                else if(section.rfind("snapshots.",0)==0)
                {
                    std::string sub=section.substr(10);     // 去掉 "snapshots."

                    const bool is_bus_gain=(sub.size()>=9&&sub.compare(sub.size()-9,9,".bus_gain")==0);

                    if(is_bus_gain)     // 以 .bus_gain 结尾
                    {
                        current_snapshot=sub.substr(0,sub.size()-9);
                        in_snapshot=true;
                        in_snapshot_gain=true;
                    }
                    else
                    {
                        current_snapshot=sub;
                        current_snapshot_config=SnapshotConfig();
                        in_snapshot=true;
                    }
                }

                continue;
            }

            size_t eq=line.find('=');
            if(eq==std::string::npos)continue;

            const std::string key=Trim(line.substr(0,eq));
            const std::string value=Trim(line.substr(eq+1));

            // 快照总线增益行：Music = -6.0
            if(in_snapshot_gain)
            {
                current_snapshot_config.SetGain(AudioBusTypeFromString(key.c_str()),ParseFloat(value));
                continue;
            }

            // RTPC 字段
            if(in_rtpc)
            {
                if     (key=="param")     current_rtpc.param=ToOSString(Unquote(value));
                else if(key=="min")       current_rtpc.min=ParseFloat(value);
                else if(key=="max")       current_rtpc.max=ParseFloat(value);
                else if(key=="target")    current_rtpc.target=RTPCTargetFromString(Unquote(value).c_str());
                else if(key=="min_value") current_rtpc.min_value=ParseFloat(value);
                else if(key=="max_value") current_rtpc.max_value=ParseFloat(value);

                continue;
            }

            if(!in_event)continue;

            if     (key=="file")            current_config.files.Add(ToOSString(Unquote(value)));
            else if(key=="files")           { std::vector<std::string> arr; ParseStringArray(value,arr); for(auto &s:arr) current_config.files.Add(ToOSString(s)); }
            else if(key=="sequence")        { std::vector<std::string> arr; ParseStringArray(value,arr); for(auto &s:arr) current_config.sequence.Add(ToOSString(s)); }
            else if(key=="children")        { std::vector<std::string> arr; ParseStringArray(value,arr); for(auto &s:arr) current_config.children.Add(ToOSString(s)); }
            else if(key=="min_gain")        current_config.min_gain=ParseFloat(value);
            else if(key=="max_gain")        current_config.max_gain=ParseFloat(value);
            else if(key=="min_pitch")       current_config.min_pitch=ParseFloat(value);
            else if(key=="max_pitch")       current_config.max_pitch=ParseFloat(value);
            else if(key=="priority")        current_config.priority=ParseFloat(value);
            else if(key=="reference_distance") current_config.reference_distance=ParseFloat(value);
            else if(key=="max_distance")    current_config.max_distance=ParseFloat(value);
            else if(key=="rolloff_factor")  current_config.rolloff_factor=ParseFloat(value);
            else if(key=="loop")            current_config.loop=ParseBool(value);
            else if(key=="bus")             current_config.bus_type=AudioBusTypeFromString(Unquote(value).c_str());
        }

        // 保存最后一个事件
        if(in_event&&!current_event.empty())
        {
            // flush 当前 rtpc
            if(in_rtpc&&!current_rtpc.param.IsEmpty())
                current_config.rtpc.push_back(current_rtpc);

            AddEvent(ToOSString(current_event).c_str(),current_config);
            ++loaded;
        }

        // 保存最后一个快照
        if(in_snapshot&&!current_snapshot.empty())
        {
            AddSnapshot(ToOSString(current_snapshot).c_str(),current_snapshot_config);
            ++loaded;
        }

        file.close();

        return loaded>0;
    }
}//namespace hgl::audio
