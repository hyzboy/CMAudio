// Audio Server (T7: 独占进程模式)
// 独立进程运行音频引擎，通过命名管道接收客户端事件指令。
// 用法：audio_server <pipe_name> <toml_config>
//   服务端阻塞等待客户端连接 → 启动引擎线程 → 处理事件直到客户端断开
#include <iostream>
#include <hgl/audio/IPCTransport.h>
#include <hgl/audio/AudioEngineThread.h>
#include <hgl/utf.h>
#include <hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

int main(int argc,char **argv)
{
    const char *pipe_name=argc>1?argv[1]:"default";
    const char *config_path=argc>2?argv[2]:nullptr;

    std::cout << "[AudioServer] pipe=" << pipe_name
              << " config=" << (config_path?config_path:"(none)") << std::endl;

    IPCTransport ipc;

    std::cout << "[AudioServer] 等待客户端连接..." << std::endl;

    if(!ipc.InitServer(ToOSString(pipe_name).c_str()))
    {
        std::cout << "[AudioServer] 创建管道失败" << std::endl;
        return 1;
    }

    std::cout << "[AudioServer] 客户端已连接，启动引擎" << std::endl;

    AudioEngineThread engine(&ipc);

    // 加载 Cue 配置（可选）
    if(config_path)
        engine.GetCues().LoadFromTOML(config_path);

    if(!engine.Start())
    {
        std::cout << "[AudioServer] 引擎启动失败" << std::endl;
        return 1;
    }

    // 主循环：运行直到客户端断开
    while(!ipc.IsPeerDisconnected())
        hgl::SleepSecond(0.1);

    std::cout << "[AudioServer] 客户端断开，停止引擎" << std::endl;

    engine.WaitExit(2.0);

    ipc.Close();

    std::cout << "[AudioServer] 退出" << std::endl;
    return 0;
}
