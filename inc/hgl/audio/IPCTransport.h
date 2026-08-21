#pragma once

#include<hgl/CoreType.h>
#include<hgl/platform/Platform.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/audio/EventTransport.h>

#if HGL_OS == HGL_OS_Windows
    #include<windows.h>
#endif

namespace hgl::audio
{
    /**
    * 跨进程事件传输（T7，独占进程模式）
    *
    * Windows 命名管道：事件通道（客户端→服务端）+ 回传通道（服务端→客户端）。
    * 消息模式（PIPE_TYPE_MESSAGE）：每条消息 = 一个定长 POD（AudioEvent 48B / AudioEventResult 16B）。
    *
    * 用法：
    *   服务端（音频进程）：
    *     IPCTransport ipc;
    *     ipc.InitServer(OS_TEXT("myapp"));       // 创建两条命名管道并等待客户端
    *     AudioEngineThread engine(&ipc);          // 引擎线程消费管道事件
    *     engine.Start();
    *
    *   客户端（游戏进程）：
    *     IPCTransport ipc;
    *     ipc.ConnectClient(OS_TEXT("myapp"));     // 连接两条管道
    *     ipc.Send(ev);                            // 发事件（写事件管道）
    *     ipc.PollResult(r);                       // 收回传（查回传管道）
    *
    * 生命周期：管道名 \\.\pipe\hgl_audio_<name>（事件）与 hgl_audio_<name>_result（回传）。
    * 客户端断开后服务端 ReadFile 返回失败 → 引擎可检测退出。
    */
    class IPCTransport:public EventTransport
    {
    #if HGL_OS == HGL_OS_Windows

        HANDLE event_pipe;           ///< 事件通道：客户端写 / 服务端读
        HANDLE result_pipe;          ///< 回传通道：服务端写 / 客户端读

        os_char pipe_name[128];      ///< 管道名（含路径）

        bool server_role;            ///< 服务端角色

        bool CreatePipes();          ///< 服务端：创建命名管道并等待客户端连接
        bool ConnectPipes();         ///< 客户端：连接服务端管道

    public:

        IPCTransport()
        {
            event_pipe=INVALID_HANDLE_VALUE;
            result_pipe=INVALID_HANDLE_VALUE;
            server_role=false;
            pipe_name[0]=0;
        }

        ~IPCTransport()override
        {
            Close();
        }

        /**
        * 服务端角色：创建命名管道并等待客户端连接（阻塞等待）
        * @param name 管道名（不含路径前缀）
        */
        bool InitServer(const os_char *name);

        /**
        * 客户端角色：连接服务端管道
        * @param name 管道名（与 InitServer 一致）
        */
        bool ConnectClient(const os_char *name);

        void Close();

        bool IsConnected()const{return event_pipe!=INVALID_HANDLE_VALUE;}

        /**
        * 对端是否已断开（PeekNamedPipe 失败 = 管道破裂/客户端退出）
        * 服务端主循环轮询此方法检测客户端退出
        */
        bool IsPeerDisconnected();

        // ---- EventTransport ----

        bool Send(const AudioEvent &ev)override;     ///< 客户端：写事件管道（消息模式）
        bool Recv(AudioEvent &ev)override;           ///< 服务端：读事件管道（消息模式）
        bool PostResult(const AudioEventResult &r)override; ///< 服务端：写回传管道
        bool PollResult(AudioEventResult &r)override;      ///< 客户端：查回传管道

        int  GetPendingCount()const override;
        int  GetResultCount()const override;
        uint64 GetDroppedCount()const override{return 0;}

    #else
        // 非 Windows：T7 暂不支持（后续 Unix domain socket）
    public:
        bool InitServer(const os_char *){return false;}
        bool ConnectClient(const os_char *){return false;}
        void Close(){}
        bool Send(const AudioEvent &)override{return false;}
        bool Recv(AudioEvent &)override{return false;}
        bool PostResult(const AudioEventResult &)override{return false;}
        bool PollResult(AudioEventResult &)override{return false;}
        int  GetPendingCount()const override{return 0;}
        int  GetResultCount()const override{return 0;}
    #endif//HGL_OS
    };//class IPCTransport
}//namespace hgl::audio
