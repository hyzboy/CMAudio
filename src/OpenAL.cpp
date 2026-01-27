#include<hgl/log/Log.h>
#include<hgl/filesystem/FileSystem.h>
#include<hgl/filesystem/Filename.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/type/Pair.h>
#include<hgl/type/Stack.h>
#include<hgl/type/String.h>
#include<hgl/type/StringList.h>
#include<hgl/type/SplitString.h>
#include<hgl/math/PhysicsConstants.h>
#include<hgl/platform/ExternalModule.h>

using namespace hgl;

namespace openal
{
    static ALCchar AudioDeviceName[AL_DEVICE_NAME_MAX_LEN]={0};

    static ExternalModule *AudioEM      =nullptr;        //OpenAL动态链接库指针
    static ALCdevice *AudioDevice       =nullptr;        //OpenAL设备
    static ALCcontext *AudioContext     =nullptr;        //OpenAL上下文

    static U8StringList OpenALExt_List;
    static U8StringList OpenALContextExt_List;

    static bool AudioFloat32            =false;         //是否支持float 32数据
    static bool AudioEFX                =false;         //EFX是否可用
    static bool AudioXRAM               =false;         //X-RAM是否可用

    bool LoadALCFunc(ExternalModule *);
    bool LoadALFunc(ExternalModule *);

    bool CheckXRAM(ALCdevice_struct *);
    bool CheckEFX(ALCdevice_struct *);

    void ClearAL();
    void ClearALC();
    void ClearXRAM();
    void ClearEFX();

    void PutOpenALInfo();
    //--------------------------------------------------------------------------------------------------
    bool OnFindedOpenALDynamicLinkLibrary(const OSString &filename,void *user_data,bool exist)
    {
        if(!exist)return(true);

        AudioEM=LoadExternalModule(filename);

        if(!AudioEM)
        {
            GLogWarning(OS_TEXT("Finded a OpenAL dynamic link library, but it can't load. filename: ")+OSString(filename));
            return(true);
        }

        GLogInfo(OS_TEXT("Loading of openAL dynamic link library successfully, filename: ")+OSString(filename));

        return(false);
    }
    //--------------------------------------------------------------------------------------------------
    /**
    * 加载OpenAL
    * @return 加载OpenAL是否成功
    */
    bool LoadOpenAL(const os_char *filename)
    {
        OSStringList oalpn;
        OSStringList oalfn=
        {
#if HGL_OS == HGL_OS_Windows
            OSString(OS_TEXT("OpenAL32.dll")),
            OSString(OS_TEXT("ct_oal.dll")),
            OSString(OS_TEXT("nvOpenAL.dll")),
            OSString(OS_TEXT("soft_oal.dll")),
            OSString(OS_TEXT("wrap_oal.dll")),
#elif (HGL_OS == HGL_OS_Linux)||(HGL_OS == HGL_OS_FreeBSD)||(HGL_OS == HGL_OS_NetBSD)||(HGL_OS == HGL_OS_OpenBSD)
            OSString(OS_TEXT("libopenal.so")),
            OSString(OS_TEXT("libopenal.so.1")),
#elif HGL_OS == HGL_OS_MacOS
            OSString(OS_TEXT("libopenal.dylib")),
#endif//HGL_OS == HGL_OS_Windows
        };

        int count=0;
        os_char *final_filename=nullptr;

        {
            OSString pn;

            filesystem::GetCurrentPath(pn);
            oalpn.Add(pn);

            pn=filesystem::JoinPathWithFilename(pn,OS_TEXT("Plug-Ins"));
            if(filesystem::IsDirectory(pn))
                oalpn.Add(pn);

            filesystem::GetOSLibraryPath(pn);
            oalpn.Add(pn);
        }

        CloseOpenAL();

        if(filename)
        {
            filesystem::FindFileOnPaths(filename,oalpn,nullptr,OnFindedOpenALDynamicLinkLibrary);
        }
        else
        {
            filesystem::FindFileOnPaths(oalfn,oalpn,nullptr,OnFindedOpenALDynamicLinkLibrary);
        }

        if(AudioEM)
        {
            return true;
        }
        else
        {
            GLogError(  OS_TEXT("The OpenAL Dynamic link library was not found, perhaps the loading failed..\n")
                        OS_TEXT("you can download it on \"http://www.openal.org\" or \"https://openal-soft.org\",\n")
                        OS_TEXT("Downloading the driver driver installation from the official website of the sound card is also a good choice."));

            return false;
        }
    }

    const char *alcGetDeviceNameList()
    {
        if(!alcIsExtensionPresent)return(nullptr);

        if(alcIsExtensionPresent(nullptr,"ALC_ENUMERATE_ALL_EXT"))
            return (char *)alcGetString(nullptr,ALC_ALL_DEVICES_SPECIFIER);
        else
        if(alcIsExtensionPresent(nullptr,"ALC_ENUMERATION_EXT"))
            return (char *)alcGetString(nullptr,ALC_DEVICE_SPECIFIER);
        else
            return(nullptr);
    }

    bool alcGetDefaultDeviceName(char *name)
    {
        if(alcIsExtensionPresent)
        {
            const char *result=nullptr;

            if(alcIsExtensionPresent(nullptr,"ALC_ENUMERATE_ALL_EXT"))
                result=(char *)alcGetString(nullptr,ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
            else
            if(alcIsExtensionPresent(nullptr,"ALC_ENUMERATION_EXT"))
                result=(char *)alcGetString(nullptr,ALC_DEFAULT_DEVICE_SPECIFIER);

            if(result&&(*result))
            {
                hgl::strcpy(name,AL_DEVICE_NAME_MAX_LEN,result);
                return(true);
            }
        }

        *name=0;
        return(false);
    }

    bool alcGetDeviceInfo(OpenALDevice &dev,const char *device_name)
    {
        ALCdevice *device=alcOpenDevice(device_name);

        if(!device)
            return(false);

        hgl::strcpy(dev.name,       AL_DEVICE_NAME_MAX_LEN,device_name);
        hgl::strcpy(dev.specifier,  AL_DEVICE_NAME_MAX_LEN,alcGetString(device,ALC_DEVICE_SPECIFIER));

        alcGetIntegerv(device,ALC_MAJOR_VERSION,sizeof(int),&dev.major);

        if(dev.major)
        {
            alcGetIntegerv(device,ALC_MINOR_VERSION,sizeof(int),&dev.minor);
        }
        else
        {
            dev.major=1;
            dev.minor=0;
        }

        alcCloseDevice(device);
        return(true);
    }

    bool alcGetDefaultDevice(OpenALDevice &dev)
    {
        if(!alcGetDefaultDeviceName(dev.name))
            return(false);

        return alcGetDeviceInfo(dev,dev.name);
    }

    int alcGetDeviceList(ValueArray<OpenALDevice> &device_list)
    {
        const char *devices=alcGetDeviceNameList();

        if(!devices)
        {
            GLogInfo(OS_TEXT("Can't get list of OpenAL Devices."));
            return(-1);
        }

        while(*devices)
        {
            OpenALDevice dev;

            if(alcGetDeviceInfo(dev,devices))
                device_list.Add(dev);

            devices+=::strlen(devices)+1;
        }

        return device_list.GetCount();
    }

    void EnumOpenALDevice()
    {
        const char *devices=alcGetDeviceNameList();
        const char *actual_devicename;

        if(!devices)
        {
            GLogInfo(OS_TEXT("can't get list of OpenAL Devices."));
            return;
        }

        while(*devices)
        {
            ALCdevice *device=alcOpenDevice(devices);

            if(device)
            {
                actual_devicename=alcGetString(device,ALC_DEVICE_SPECIFIER);

                int major=0,minor=0;

                alcGetIntegerv(device,ALC_MAJOR_VERSION,sizeof(int),&major);

                if(major)
                {
                    alcGetIntegerv(device,ALC_MINOR_VERSION,sizeof(int),&minor);

                    GLogInfo(u8"OpenAL device: "+U8String((u8char *)devices)+u8" Specifier:"+U8String((u8char *)actual_devicename)+u8", version: "+U8String::numberOf(major)+u8"."+U8String::numberOf(minor));
                }
                else
                {
                    GLogInfo(u8"OpenAL device: "+U8String((u8char *)devices)+u8" Specifier:"+U8String((u8char *)actual_devicename)+u8",can't get version, used OpenAL 1.0.");
                }

                alcCloseDevice(device);
            }

            devices+=::strlen(devices)+1;
        }
    }

    void alcSetDefaultContext()
    {
        alcMakeContextCurrent(AudioContext);
    }

    void GetOpenALExt(U8StringList &ext_list,const char *ext_str)
    {
        int len=hgl::strlen(ext_str);

        SplitToStringListBySpace<u8char>(ext_list,(const u8char *)ext_str,len);        //space指所有不可打印字符
    }

    void InitOpenALExt()
    {
        GetOpenALExt(OpenALExt_List,alGetString(AL_EXTENSIONS));
        GetOpenALExt(OpenALContextExt_List,alcGetString(AudioDevice,ALC_EXTENSIONS));

        if(OpenALExt_List.Contains(u8"AL_EXT_FLOAT32"))
            AudioFloat32=true;
    }

    /**
    * 初始化OpenAL驱动
    * @param driver_name 驱动名称,如果不写则自动查找
    * @return 是否初始化成功
    */
    bool InitOpenALDriver(const os_char *driver_name)
    {
        if (!AudioEM)
            if (!LoadOpenAL(driver_name))RETURN_FALSE;

        RETURN_BOOL(LoadALCFunc(AudioEM));
    }

    bool InitOpenALDeviceByName(const char *device_name)
    {
        bool default_device=false;

        if(device_name)
            hgl::strcpy(AudioDeviceName,AL_DEVICE_NAME_MAX_LEN,device_name);
        else
        {
            alcGetDefaultDeviceName(AudioDeviceName);

            if(*AudioDeviceName)
            {
                GLogInfo(u8"Select the default OpenAL device: "+U8String((u8char *)AudioDeviceName));
            }
            else
            {
                GLogInfo(OS_TEXT("Select the default OpenAL device."));
            }

            default_device=true;
        }

        AudioDevice=alcOpenDevice(AudioDeviceName);

        if(AudioDevice==nullptr)
        {
            GLogError(u8"Failed to OpenAL Device: "+U8String((u8char *)AudioDeviceName));

            if(default_device)
            {
                CloseOpenAL();
                return(AL_FALSE);
            }

            //使用缺省设备
            {
                alcGetDefaultDeviceName(AudioDeviceName);
                AudioDevice=alcOpenDevice(AudioDeviceName);

                if(!AudioDevice)
                {
                    GLogError(OS_TEXT("The OpenAL device cannot be initialized properly using the specified device and the default device!"));
                    CloseOpenAL();
                    return(AL_FALSE);
                }

                GLogInfo(OS_TEXT("Select the default OpenAL device because the specified device is failing."));
            }
        }

        GLogInfo(U8_TEXT("Opened OpenAL Device")+U8String((u8char *)AudioDeviceName));

        AudioContext=alcCreateContext(AudioDevice,0);
        if(AudioContext==nullptr)
        {
            GLogError(OS_TEXT("Create OpenAL Context OK."));
            CloseOpenAL();
            return (AL_FALSE);
        }
        #ifdef _DEBUG
        else
        {
            GLogInfo(OS_TEXT("Failed to Create OpenAL Context."));
        }
        #endif//_DEBUG

        if(!alcMakeContextCurrent(AudioContext))
        {
            GLogError(OS_TEXT("Failed to Make OpenAL Context"));
            CloseOpenAL();
            RETURN_FALSE;
        }
        #ifdef _DEBUG
        else
        {
            GLogInfo(OS_TEXT("Make OpenAL Context OK."));
        }
        #endif//_DEBUG

        hgl::strcpy(AudioDeviceName,AL_DEVICE_NAME_MAX_LEN,alcGetString(AudioDevice,ALC_DEVICE_SPECIFIER));

        GLogInfo(u8"Initing OpenAL Device: "+U8String((u8char *)AudioDeviceName));

        if (!LoadALFunc(AudioEM))
        {
            GLogInfo(OS_TEXT("Failed to load OpenAL functions"));
            CloseOpenAL();
            RETURN_FALSE;
        }

        GLogInfo(OS_TEXT("Loaded OpenAL functions."));

        if(alcIsExtensionPresent)
        {
            AudioXRAM=CheckXRAM(AudioDevice);
            AudioEFX=CheckEFX(AudioDevice);
        }
        else
        {
            AudioXRAM=false;
            AudioEFX=false;
        }

        alGetError();

        InitOpenALExt();

        GLogInfo(OS_TEXT("Inited OpenAL."));
        return(AL_TRUE);
    }

    bool InitOpenALDevice(const OpenALDevice *device)
    {
        if(device)
            return InitOpenALDeviceByName(device->name);
        else
            return InitOpenALDeviceByName(nullptr);
    }

    //--------------------------------------------------------------------------------------------------
    /**
    * 关闭OpenAL
    */
    void CloseOpenAL()
    {
        if(AudioEM)
        {
            alcMakeContextCurrent(0);

            if(AudioContext) alcDestroyContext(AudioContext);
            if(AudioDevice)    alcCloseDevice(AudioDevice);
            SAFE_CLEAR(AudioEM);

            GLogInfo(OS_TEXT("Close OpenAL."));
        }

        ClearAL();
        ClearALC();
        ClearXRAM();
        ClearEFX();
    }

    /**
    * 设置距离衰减模型(默认钳位倒数距离模型)
    */
    bool SetDistanceModel(ALenum distance_model)
    {
        if (!AudioEM)return(false);
        if (!alDistanceModel)return(false);

        if (distance_model<AL_INVERSE_DISTANCE
          ||distance_model>AL_EXPONENT_DISTANCE_CLAMPED)
            return(false);

        alDistanceModel(distance_model);
        return(true);
    }

    bool SetSpeedOfSound(const float ss)
    {
        if (!AudioEM)return(false);
        if (!alSpeedOfSound)return(false);

        alSpeedOfSound(ss);
        return(true);
    }

    double SetSpeedOfSound(const double height,const double temperature,const double humidity)
    {
        if (!AudioEM)return(0);
        if (!alSpeedOfSound)return(0);

        double v=HGL_SPEED_OF_SOUND;                        //海拔0米，0摄氏度

        v*=sqrt(1.0f+temperature/(-HGL_ABSOLUTE_ZERO));     //先计算温度

        ////11000米高度，温度-56.5，音速295m/s。需根据大气密度来计算

        //另需计算温度，影响在0.1-0.6之间。

        alSpeedOfSound(v/HGL_SPEED_OF_SOUND);
        return(v);
    }

    bool SetDopplerFactor(const float df)
    {
        if (!AudioEM)return(false);
        if (!alDopplerFactor)return(false);

        alDopplerFactor(df);
        return(true);
    }

    bool SetDopplerVelocity(const float dv)
    {
        if (!AudioEM)return(false);
        if (!alDopplerVelocity)return(false);

        alDopplerVelocity(dv);
        return(true);
    }
    //--------------------------------------------------------------------------------------------------
//    void *alcGetCurrentDevice() { return (AudioDevice); }
//    char *alcGetCurrentDeviceName() { return(AudioDeviceName); }

    const struct ALFormatBytes
    {
        ALenum format;
        uint bytes;
    }
    al_format_bytes[]=
    {
        {AL_FORMAT_MONO8,           1},
        {AL_FORMAT_MONO16,          2},
        {AL_FORMAT_STEREO8,         2},
        {AL_FORMAT_STEREO16,        4},

//         {AL_FORMAT_QUAD16,            8},
//         {AL_FORMAT_51CHN16,            12},
//         {AL_FORMAT_61CHN16,            14},
//         {AL_FORMAT_71CHN16,            16},
//
        {AL_FORMAT_MONO_FLOAT32,    4},
        {AL_FORMAT_STEREO_FLOAT32,  8},
//
//         {AL_FORMAT_QUAD8,            4},
//         {AL_FORMAT_QUAD16,            8},
//         {AL_FORMAT_QUAD32,            16},
//         {AL_FORMAT_REAR8,            4},
//         {AL_FORMAT_REAR16,            8},
//         {AL_FORMAT_REAR32,            16},
//         {AL_FORMAT_51CHN8,            6},
//         {AL_FORMAT_51CHN16,            12},
//         {AL_FORMAT_51CHN32,            24},
//         {AL_FORMAT_61CHN8,            7},
//         {AL_FORMAT_61CHN16,            14},
//         {AL_FORMAT_61CHN32,            28},
//         {AL_FORMAT_71CHN8,            8},
//         {AL_FORMAT_71CHN16,            16},
//         {AL_FORMAT_71CHN32,            32},

        {0,0}
    };

    const int al_get_format_byte(ALenum format)
    {
        const ALFormatBytes *p=al_format_bytes;

        while(p->format)
        {
            if(p->format==format)
                return p->bytes;

            ++p;
        }

        return(0);
    }

    /**
    * 计算音频数据可播放的时间
    * @param size 数据字节长度
    * @param format 数据格式
    * @param freq 数据采样率
    * @return 时间(秒)
    */
    double AudioDataTime(ALuint size,ALenum format,ALsizei freq)
    {
        if(size==0||freq==0)return(0);

        const uint byte=al_get_format_byte(format);

        if(byte==0)
            return(0);

        return (double(size)/double(freq)/double(byte));
    }

    /**
    * 计算一秒钟音频数据所需要的字节数
    * @param format 数据格式
    * @param freq 数据采样率
    * @return 时间(秒)
    */
    unsigned int AudioTime(ALenum format,ALsizei freq)
    {
        const uint byte=al_get_format_byte(format);

        if(byte==0)
            return(0);

        return byte*freq;
    }

    unsigned int GetMaxNumSources()
    {
        ALuint source;
        Stack<ALuint> source_stack;

        // Clear AL Error Code
        alGetError();

        for(;;)
        {
            alGenSources(1, &source);

            if (alGetError() != AL_NO_ERROR)
                break;

            source_stack.Push(source);
        }

        const int count=source_stack.GetCount();
        const ALuint *p=source_stack.GetData();

        alDeleteSources(count,p);

        if (alGetError() != AL_NO_ERROR)
        {
            for(int i=0;i<count;i++)
                alDeleteSources(1, p++);
        }

        return count;
    }

    /**
     * 是否支持浮点音频数据
     */
    bool IsSupportFloatAudioData()
    {
        return AudioFloat32;
    }
    //--------------------------------------------------------------------------------------------------
    const u8char *alGetErrorInfo(const os_char *filename,const int line)
    {
        if(!alGetError)return(U8_TEXT("OpenALEE/OpenAL 未初始化!"));

        const u8char *result=nullptr;

        const u8char al_error_invalid             []=U8_TEXT("invalid");
        const u8char al_error_invalid_name        []=U8_TEXT("invalid name");
        const u8char al_error_invalid_enum        []=U8_TEXT("invalid enum");
        const u8char al_error_invalid_value       []=U8_TEXT("invalid value");
        const u8char al_error_invalid_operation   []=U8_TEXT("invalid operation");
        const u8char al_error_out_of_memory       []=U8_TEXT("out of memory");
        const u8char al_error_unknown_error       []=U8_TEXT("unknown error");

        const ALenum error=alGetError();

        if(error==AL_NO_ERROR           )result=nullptr;else
        if(error==AL_INVALID            )result=al_error_invalid;else
        if(error==AL_INVALID_NAME       )result=al_error_invalid_name;else
        if(error==AL_INVALID_ENUM       )result=al_error_invalid_enum;else
        if(error==AL_INVALID_VALUE      )result=al_error_invalid_value;else
        if(error==AL_INVALID_OPERATION  )result=al_error_invalid_operation;else
        if(error==AL_OUT_OF_MEMORY      )result=al_error_out_of_memory;else
                                         result=al_error_unknown_error;

        if(result)
        {
            GLogError(OS_TEXT("OpenAL error,source file:\"")+OSString(filename)+OS_TEXT("\",line:")+OSString::numberOf(line));
            GLogError(U8_TEXT("OpenAL ErrorNo:")+U8String(result));
        }

        return(result);
    }

    void PutOpenALInfo()
    {
        if(!alGetString)return;

        GLogInfo(U8String(    u8"          OpenAL Vendor: ")+(u8char *)alGetString(AL_VENDOR    ));
        GLogInfo(U8String(    u8"         OpenAL Version: ")+(u8char *)alGetString(AL_VERSION   ));
        GLogInfo(U8String(    u8"        OpenAL Renderer: ")+(u8char *)alGetString(AL_RENDERER  ));
        GLogInfo(U8String(    u8"      Max audio sources: ")+U8String::numberOf(GetMaxNumSources()));
        GLogInfo(AudioFloat32?u8"   Supported float data: Yes"
                             :u8"   Supported float data: No");

        GLogInfo(AudioEFX?    u8"                    EFX: Supported"
                         :    u8"                    EFX: No");

        if(AudioXRAM)
        {
            int size;

            alcGetIntegerv(AudioDevice,eXRAMSize,sizeof(int),&size);

            GLogInfo(         u8"                  X-RAM: Supported");
            GLogInfo(         u8"           X-RAM Volume: "+U8String::numberOf(size));
        }
        else
        {
            GLogInfo(         u8"                  X-RAM: No");
        }

        for(int i=0;i<OpenALExt_List.GetCount();i++)
            if(i==0)
            {
                GLogInfo(U8String(u8"      OpenAL Extentsion: ")+OpenALExt_List[i]);
            }
            else
            {
                GLogInfo(U8String(u8"                         ")+OpenALExt_List[i]);
            }

        for(int i=0;i<OpenALContextExt_List.GetCount();i++)
            if(i==0)
            {
                GLogInfo(U8String(u8"OpenAL Device Extension: ")+OpenALContextExt_List[i]);
            }
            else
            {
                GLogInfo(U8String(u8"                         ")+OpenALContextExt_List[i]);
            }


        #ifdef _DEBUG
        GLogInfo(OS_TEXT("OpenAL Infomation finished."));
        #endif//
    }

    /**
    * 初始化OpenAL
    * @param driver_name 驱动名称,如果不写则自动查找
    * @param device_name 设备名称,如果不写则自动选择缺省设备
    * @param out_info   是否输出当前OpenAL设备信息
    * @param enum_device 是否枚举所有的设备信息输出
    * @return 是否初始化成功
    */
    bool InitOpenAL(const os_char *driver_name,const char *device_name,bool out_info,bool enum_device)
    {
        if (!InitOpenALDriver(driver_name))
            return(false);

        if (enum_device)
            EnumOpenALDevice();

        if (!InitOpenALDeviceByName(device_name))
        {
            CloseOpenAL();
            return(false);
        }

        InitOpenALExt();

        if(out_info)
            PutOpenALInfo();

        return(true);
    }

    //--------------------------------------------------------------------------------------------------
    // HRTF Support Functions
    //--------------------------------------------------------------------------------------------------

    /**
     * 检查是否支持HRTF
     * @return 是否支持HRTF扩展
     */
    bool IsHRTFSupported()
    {
        if(!AudioDevice || !alcIsExtensionPresent)
            return false;

        return alcIsExtensionPresent(AudioDevice, "ALC_SOFT_HRTF") == ALC_TRUE;
    }

    /**
     * 加载HRTF扩展函数（内部辅助函数）
     * @return 是否成功加载所有必需的HRTF函数
     */
    static bool LoadHRTFFunctions()
    {
        if(!AudioDevice || !alcGetProcAddress)
            return false;

        if(!alcResetDeviceSOFT)
        {
            alcResetDeviceSOFT = (alcResetDeviceSOFTPROC)alcGetProcAddress(AudioDevice, "alcResetDeviceSOFT");
            if(!alcResetDeviceSOFT)
            {
                GLogError(OS_TEXT("Failed to load alcResetDeviceSOFT function"));
                return false;
            }
        }

        if(!alcGetStringiSOFT)
        {
            alcGetStringiSOFT = (alcGetStringiSOFTPROC)alcGetProcAddress(AudioDevice, "alcGetStringiSOFT");
            if(!alcGetStringiSOFT)
            {
                GLogWarning(OS_TEXT("Failed to load alcGetStringiSOFT function (profile names unavailable)"));
                // 不返回false，因为这个函数只用于获取配置文件名称，不是必需的
            }
        }

        return true;
    }

    /**
     * 启用/禁用HRTF
     * @param enable 是否启用HRTF
     * @return 是否成功
     */
    bool EnableHRTF(bool enable)
    {
        if(!AudioDevice || !AudioContext)
        {
            GLogError(OS_TEXT("OpenAL device or context not initialized"));
            return false;
        }

        if(!IsHRTFSupported())
        {
            GLogWarning(OS_TEXT("HRTF is not supported by current OpenAL implementation"));
            return false;
        }

        // 加载HRTF扩展函数
        if(!LoadHRTFFunctions())
        {
            return false;
        }

        // 设置HRTF属性
        ALCint attrs[] = {
            ALC_HRTF_SOFT, enable ? ALC_TRUE : ALC_FALSE,
            0  // 终止符
        };

        // 重置设备以应用HRTF设置
        if(!alcResetDeviceSOFT(AudioDevice, attrs))
        {
            ALCenum error = alcGetError(AudioDevice);
            GLogError(OS_TEXT("Failed to reset OpenAL device for HRTF. Error code: ")+OSString::numberOf(error));
            return false;
        }

        // 验证HRTF状态
        int hrtf_status = GetHRTFStatus();

        if(enable)
        {
            if(hrtf_status == ALC_HRTF_ENABLED_SOFT)
            {
                GLogInfo(OS_TEXT("HRTF enabled successfully"));
                return true;
            }
            else if(hrtf_status == ALC_HRTF_DENIED_SOFT)
            {
                GLogWarning(OS_TEXT("HRTF was denied (possibly incompatible output format)"));
                return false;
            }
            else if(hrtf_status == ALC_HRTF_REQUIRED_SOFT)
            {
                GLogInfo(OS_TEXT("HRTF is required by device"));
                return true;
            }
            else if(hrtf_status == ALC_HRTF_HEADPHONES_DETECTED_SOFT)
            {
                GLogInfo(OS_TEXT("HRTF enabled (headphones detected)"));
                return true;
            }
            else if(hrtf_status == ALC_HRTF_UNSUPPORTED_FORMAT_SOFT)
            {
                GLogWarning(OS_TEXT("HRTF unsupported with current audio format"));
                return false;
            }
            else
            {
                GLogWarning(OS_TEXT("HRTF enable status unknown: ")+OSString::numberOf(hrtf_status));
                return false;
            }
        }
        else
        {
            if(hrtf_status == ALC_HRTF_DISABLED_SOFT)
            {
                GLogInfo(OS_TEXT("HRTF disabled successfully"));
                return true;
            }
            else
            {
                GLogWarning(OS_TEXT("HRTF disable may not have taken effect. Status: ")+OSString::numberOf(hrtf_status));
                return false;
            }
        }
    }

    /**
     * 获取HRTF状态
     * @return HRTF状态常量
     */
    int GetHRTFStatus()
    {
        if(!AudioDevice || !alcGetIntegerv)
            return ALC_HRTF_DISABLED_SOFT;

        if(!IsHRTFSupported())
            return ALC_HRTF_DISABLED_SOFT;

        ALCint hrtf_status = ALC_HRTF_DISABLED_SOFT;
        alcGetIntegerv(AudioDevice, ALC_HRTF_STATUS_SOFT, 1, &hrtf_status);

        return hrtf_status;
    }

    /**
     * 获取可用的HRTF配置数量
     * @return HRTF配置数量
     */
    int GetNumHRTFSpecifiers()
    {
        if(!AudioDevice || !alcGetIntegerv)
            return 0;

        if(!IsHRTFSupported())
            return 0;

        ALCint num_hrtf = 0;
        alcGetIntegerv(AudioDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);

        return num_hrtf;
    }

    /**
     * 获取指定索引的HRTF配置名称
     * @param index HRTF配置索引
     * @return HRTF配置名称
     */
    const char* GetHRTFSpecifierName(int index)
    {
        if(!AudioDevice)
            return nullptr;

        if(!IsHRTFSupported())
            return nullptr;

        // 加载HRTF扩展函数
        if(!LoadHRTFFunctions())
            return nullptr;

        if(!alcGetStringiSOFT)
            return nullptr;

        return alcGetStringiSOFT(AudioDevice, ALC_HRTF_SPECIFIER_SOFT, index);
    }
}//namespace openal
