#include "maix_basic.hpp"
#include "maix_camera.hpp"
#include "maix_image.hpp"

using namespace maix;

int _main(int argc, char *argv[])
{
    // 初始化摄像头
    camera::Camera cam(320, 240, image::Format::FMT_RGB888);
    err::Err e = cam.open();
    if (e != err::ERR_NONE) {
        log::error("Camera open failed: %d", e);
        return -1;
    }

    log::info("Circle detection started. Press Ctrl+C to exit.");

    while (!app::need_exit())
    {
        // 捕获图像
        image::Image *img = cam.read();
        if (!img) {
            log::warn("Failed to read image");
            time::sleep_ms(10);
            continue;
        }

        auto circles = img.find_circles({0, 0, img->width(), img->height()}, 2, 2, 20, 10, 10, 10, 5, -1, 2, true);

        // 查找圆形
        // std::vector<image::Circle> circles = img->find_circles(
        //     {0, 0, img->width(), img->height()}, // ROI区域
        //     2,    // x_stride
        //     2,    // y_stride
        //     2000, // threshold
        //     10,   // x_margin
        //     10,   // y_margin
        //     10,   // r_margin
        //     5,    // r_min
        //     50,   // r_max
        //     1     // r_step
        // );

        // 打印检测结果
        if (!circles.empty()) {
            log::info("Detected %d circles:", circles.size());
            for (size_t i = 0; i < circles.size(); ++i) {
                auto& circle = circles[i];
                log::info("  Circle %d: X=%d, Y=%d, R=%d, Magnitude=%.1f", 
                         i+1, circle.x(), circle.y(), circle.r(), circle.magnitude());
            }
        } else {
            log::debug("No circles detected");
        }

        float fps = time::fps();
        log::info("fps: %.1f", fps);
        // 释放图像资源
        delete img;

        // 控制处理频率
        //time::sleep_ms(100); // 10fps
    }

    cam.close();
    log::info("Circle detection stopped");
    return 0;
}

int main(int argc, char *argv[])
{
    sys::register_default_signal_handle();
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}