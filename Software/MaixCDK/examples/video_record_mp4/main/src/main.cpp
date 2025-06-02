
#include "stdio.h"
#include "main.h"
#include "maix_util.hpp"
#include "maix_image.hpp"
#include "maix_time.hpp"
#include "maix_display.hpp"
#include "maix_video.hpp"
#include "maix_camera.hpp"
#include "maix_basic.hpp"
#include "csignal"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

using namespace maix;

int _main(int argc, char* argv[])
{
    camera::Camera cam = camera::Camera(2560, 1440, image::Format::FMT_YVU420SP);
    // 创建视频录制器
    video::VideoRecorder recorder(true);  // true表示自动打开
    
    // 绑定摄像头
    recorder.bind_camera(&cam);
    
    // 设置输出路径
    recorder.config_path("output.mp4");
    
    // 开始录制
    recorder.record_start();
    
    log::info("Recording started, will stop after 5 seconds...");

    int sleep_s = 5;
    while(!app::need_exit() && sleep_s-- > 0) {
        time::sleep(1);
    }
    
    if(!app::need_exit()) {  // 只有正常计时结束才调用finish
        recorder.record_finish();
        log::info("Recording finished!");
    }

    return 0;
}

int main(int argc, char* argv[])
{
    // Catch signal and process
    sys::register_default_signal_handle();

/*CATCH_EXCEPTION_RUN_RETURN 是 Maix 框架提供的一个宏（macro），
    用于安全地执行一个函数并捕获可能发生的异常，确保程序在异常发生时能够正确地释放资源并返回指定的错误码。
    相当于：
    try {
        return _main(argc, argv);  // 正常执行 _main
    } catch (const std::exception &e) {
        log::error("Exception caught: %s", e.what());  // 打印异常信息
        return -1;  // 返回错误码 -1
    } catch (...) {
        log::error("Unknown exception caught!");  // 未知异常
        return -1;
    }
    */
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}
