#include<hgl/audio/IPCTransport.h>
#include<hgl/type/String.h>
#include<cstring>

namespace hgl::audio
{
#if HGL_OS == HGL_OS_Windows

    bool IPCTransport::InitServer(const os_char *name)
    {
        if(!name||!(*name))return false;
        if(IsConnected())return false;

        const OSString base=OS_TEXT("\\\\.\\pipe\\hgl_audio_")+OSString(name);

        hgl::strcpy(pipe_name,sizeof(pipe_name)/sizeof(os_char),base);

        server_role=true;

        return CreatePipes();
    }

    bool IPCTransport::ConnectClient(const os_char *name)
    {
        if(!name||!(*name))return false;
        if(IsConnected())return false;

        const OSString base=OS_TEXT("\\\\.\\pipe\\hgl_audio_")+OSString(name);

        hgl::strcpy(pipe_name,sizeof(pipe_name)/sizeof(os_char),base);

        server_role=false;

        return ConnectPipes();
    }

    bool IPCTransport::CreatePipes()
    {
        const OSString base(pipe_name);
        const OSString event_name=base;
        const OSString result_name=base+OS_TEXT("_result");

        // 事件管道：客户端写、服务端读（服务端只读）
        event_pipe=CreateNamedPipeW(event_name.c_str(),
                                    PIPE_ACCESS_INBOUND,
                                    PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
                                    1,0,sizeof(AudioEvent)*8,0,nullptr);

        if(event_pipe==INVALID_HANDLE_VALUE)
            return false;

        // 回传管道：服务端写、客户端读（服务端只写）
        result_pipe=CreateNamedPipeW(result_name.c_str(),
                                     PIPE_ACCESS_OUTBOUND,
                                     PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
                                     1,sizeof(AudioEventResult)*8,0,0,nullptr);

        if(result_pipe==INVALID_HANDLE_VALUE)
        {
            CloseHandle(event_pipe);
            event_pipe=INVALID_HANDLE_VALUE;
            return false;
        }

        // 等待客户端连接两条管道（阻塞）
        if(!ConnectNamedPipe(event_pipe,nullptr)&&GetLastError()!=ERROR_PIPE_CONNECTED)
        {
            Close();
            return false;
        }

        if(!ConnectNamedPipe(result_pipe,nullptr)&&GetLastError()!=ERROR_PIPE_CONNECTED)
        {
            Close();
            return false;
        }

        return true;
    }

    bool IPCTransport::ConnectPipes()
    {
        const OSString base(pipe_name);
        const OSString event_name=base;
        const OSString result_name=base+OS_TEXT("_result");

        // 客户端连接：事件管道（写）+ 回传管道（读）
        event_pipe=CreateFileW(event_name.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);

        if(event_pipe==INVALID_HANDLE_VALUE)
            return false;

        result_pipe=CreateFileW(result_name.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);

        if(result_pipe==INVALID_HANDLE_VALUE)
        {
            CloseHandle(event_pipe);
            event_pipe=INVALID_HANDLE_VALUE;
            return false;
        }

        return true;
    }

    void IPCTransport::Close()
    {
        if(event_pipe!=INVALID_HANDLE_VALUE)
        {
            CloseHandle(event_pipe);
            event_pipe=INVALID_HANDLE_VALUE;
        }

        if(result_pipe!=INVALID_HANDLE_VALUE)
        {
            CloseHandle(result_pipe);
            result_pipe=INVALID_HANDLE_VALUE;
        }
    }

    bool IPCTransport::IsPeerDisconnected()
    {
        if(event_pipe==INVALID_HANDLE_VALUE)
            return true;

        DWORD avail=0;

        return !PeekNamedPipe(event_pipe,nullptr,0,nullptr,&avail,nullptr);
    }

    bool IPCTransport::Send(const AudioEvent &ev)
    {
        if(event_pipe==INVALID_HANDLE_VALUE)
            return false;

        DWORD written=0;

        if(!WriteFile(event_pipe,&ev,sizeof(AudioEvent),&written,nullptr))
            return false;

        return written==sizeof(AudioEvent);
    }

    bool IPCTransport::Recv(AudioEvent &ev)
    {
        if(event_pipe==INVALID_HANDLE_VALUE)
            return false;

        // 非阻塞：先查有无完整消息
        DWORD avail=0;

        if(!PeekNamedPipe(event_pipe,nullptr,0,nullptr,&avail,nullptr))
            return false;                       // 客户端断开

        if(avail<sizeof(AudioEvent))
            return false;                       // 暂无完整事件

        DWORD read=0;

        if(!ReadFile(event_pipe,&ev,sizeof(AudioEvent),&read,nullptr))
            return false;

        return read==sizeof(AudioEvent);
    }

    bool IPCTransport::PostResult(const AudioEventResult &r)
    {
        if(result_pipe==INVALID_HANDLE_VALUE)
            return false;

        DWORD written=0;

        if(!WriteFile(result_pipe,&r,sizeof(AudioEventResult),&written,nullptr))
            return false;

        return written==sizeof(AudioEventResult);
    }

    bool IPCTransport::PollResult(AudioEventResult &r)
    {
        if(result_pipe==INVALID_HANDLE_VALUE)
            return false;

        // 非阻塞：先查有无完整消息
        DWORD avail=0;

        if(!PeekNamedPipe(result_pipe,nullptr,0,nullptr,&avail,nullptr))
            return false;

        if(avail<sizeof(AudioEventResult))
            return false;

        DWORD read=0;

        if(!ReadFile(result_pipe,&r,sizeof(AudioEventResult),&read,nullptr))
            return false;

        return read==sizeof(AudioEventResult);
    }

    int IPCTransport::GetPendingCount()const
    {
        if(event_pipe==INVALID_HANDLE_VALUE)
            return 0;

        DWORD avail=0;

        if(!PeekNamedPipe(event_pipe,nullptr,0,nullptr,&avail,nullptr))
            return 0;

        return(int)(avail/sizeof(AudioEvent));
    }

    int IPCTransport::GetResultCount()const
    {
        if(result_pipe==INVALID_HANDLE_VALUE)
            return 0;

        DWORD avail=0;

        if(!PeekNamedPipe(result_pipe,nullptr,0,nullptr,&avail,nullptr))
            return 0;

        return(int)(avail/sizeof(AudioEventResult));
    }

#endif//HGL_OS
}//namespace hgl::audio
