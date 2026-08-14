#include<hgl/audio/SoundEventManager.h>
#include<hgl/type/StdString.h>      // ToOSString(std::string)

#include<fstream>
#include<sstream>
#include<string>
#include<vector>

namespace hgl::audio
{
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
    }

    bool SoundEventManager::LoadFromTOML(const char *filename)
    {
        if(!filename||!(*filename))return false;

        std::ifstream file(filename);
        if(!file.is_open())return false;

        std::string line;
        std::string current_event;
        SoundEventConfig current_config;
        bool in_event=false;

        int loaded=0;

        while(std::getline(file,line))
        {
            line=Trim(line);

            if(line.empty()||line[0]=='#')continue;

            // [event.xxx] section
            if(line[0]=='['&&line[line.length()-1]==']')
            {
                // 保存上一个事件
                if(in_event&&!current_event.empty())
                {
                    AddEvent(ToOSString(current_event).c_str(),current_config);
                    ++loaded;
                }

                current_event=line.substr(1,line.length()-2);

                if(current_event.rfind("event.",0)==0)     // 以 "event." 开头
                {
                    current_event=current_event.substr(6);
                    in_event=true;
                    current_config=SoundEventConfig();
                }
                else
                {
                    in_event=false;
                }

                continue;
            }

            if(!in_event)continue;

            size_t eq=line.find('=');
            if(eq==std::string::npos)continue;

            const std::string key=Trim(line.substr(0,eq));
            const std::string value=Trim(line.substr(eq+1));

            if     (key=="file")            current_config.files.Add(ToOSString(Unquote(value)));
            else if(key=="files")           { std::vector<std::string> arr; ParseStringArray(value,arr); for(auto &s:arr) current_config.files.Add(ToOSString(s)); }
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
            AddEvent(ToOSString(current_event).c_str(),current_config);
            ++loaded;
        }

        file.close();

        return loaded>0;
    }
}//namespace hgl::audio
