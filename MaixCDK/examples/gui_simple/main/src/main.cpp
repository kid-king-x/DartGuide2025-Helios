/*
* 一个线程用于图像的采集和处理
* 一个线程用于飞控的处理
* 
* 
*/
#include "maix_basic.hpp"
#include "maix_camera.hpp"
#include "maix_display.hpp"
#include "maix_time.hpp"
#include "maix_adc.hpp"
#include "maix_pinmap.hpp"
#include "maix_uart.hpp"
#include "maix_pwm.hpp"
#include "main.hpp"
#include <thread>   //用于创建线程
#include <mutex>    //mutex 和 condition_variable：实现线程同步
#include <condition_variable>   
#include <memory>
#include <queue>    //图像缓冲队列
#include <atomic>   //用于线程间的安全布尔值
#include <vector>   //存储颜色阈值
#include <algorithm>
#include <cmath>

using namespace maix;
using namespace maix::fs;
using namespace maix::peripheral;
using namespace maix::peripheral::adc;

std::mutex imu_mutex;
std::mutex angle_mutex;


class JY901B {
    private:
        maix::peripheral::uart::UART *_uart;
        std::vector<uint8_t> ACCData;
        std::vector<uint8_t> GYROData;
        std::vector<uint8_t> AngleData;
        
        int FrameState;
        int Bytenum;
        uint8_t CheckSum;
    
    public:
        float a[3];    // 加速度 X/Y/Z
        float w[3];    // 角速度 X/Y/Z
        float Angle[3];// 欧拉角 Roll/Pitch/Yaw
    
        JY901B(const std::string &device = "/dev/ttyS2", int baudrate = 230400) 
            : ACCData(8, 0), GYROData(8, 0), AngleData(8, 0),
              FrameState(0), Bytenum(0), CheckSum(0) {
            _uart = new maix::peripheral::uart::UART(device, baudrate);
            _uart->open();
            _uart->set_received_callback([this](maix::peripheral::uart::UART &uart, maix::Bytes &data) {
                this->DueData(data.data, data.data_len); 
            });
        }
    
        ~JY901B() {
            delete _uart;
        }
    
    private:
        void DueData(uint8_t *inputdata, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                uint8_t data = inputdata[i];
                if (FrameState == 0) {
                    if (data == 0x55 && Bytenum == 0) {
                        CheckSum = data;
                        Bytenum = 1;
                        continue;
                    } else if (data == 0x51 && Bytenum == 1) {
                        CheckSum += data;
                        FrameState = 1;
                        Bytenum = 2;
                    } else if (data == 0x52 && Bytenum == 1) {
                        CheckSum += data;
                        FrameState = 2;
                        Bytenum = 2;
                    } else if (data == 0x53 && Bytenum == 1) {
                        CheckSum += data;
                        FrameState = 3;
                        Bytenum = 2;
                    }
                } else if (FrameState == 1) {
                    if (Bytenum < 10) {
                        ACCData[Bytenum - 2] = data;
                        CheckSum += data;
                        Bytenum += 1;
                    } else {
                        if (data == (CheckSum & 0xff)) {
                            get_acc(ACCData.data());
                        }
                        CheckSum = 0;
                        Bytenum = 0;
                        FrameState = 0;
                    }
                } else if (FrameState == 2) {
                    if (Bytenum < 10) {
                        GYROData[Bytenum - 2] = data;
                        CheckSum += data;
                        Bytenum += 1;
                    } else {
                        if (data == (CheckSum & 0xff)) {
                            get_gyro(GYROData.data());
                        }
                        CheckSum = 0;
                        Bytenum = 0;
                        FrameState = 0;
                    }
                } else if (FrameState == 3) {
                    if (Bytenum < 10) {
                        AngleData[Bytenum - 2] = data;
                        CheckSum += data;
                        Bytenum += 1;
                    } else {
                        if (data == (CheckSum & 0xff)) {
                            get_angle(AngleData.data());
                        }
                        CheckSum = 0;
                        Bytenum = 0;
                        FrameState = 0;
                    }
                }
            }
        }
    
        void get_acc(uint8_t *datahex) {
            const float k_acc = 16.0f;
            
            int16_t ax = (datahex[1] << 8) | datahex[0];
            int16_t ay = (datahex[3] << 8) | datahex[2];
            int16_t az = (datahex[5] << 8) | datahex[4];
            
            a[0] = ax / 32768.0f * k_acc;
            a[1] = ay / 32768.0f * k_acc;
            a[2] = az / 32768.0f * k_acc;
            
            if (a[0] >= k_acc) a[0] -= 2 * k_acc;
            if (a[1] >= k_acc) a[1] -= 2 * k_acc;
            if (a[2] >= k_acc) a[2] -= 2 * k_acc;
        }
    
        void get_gyro(uint8_t *datahex) {
            const float k_gyro = 2000.0f;
            
            int16_t wx = (datahex[1] << 8) | datahex[0];
            int16_t wy = (datahex[3] << 8) | datahex[2];
            int16_t wz = (datahex[5] << 8) | datahex[4];
            
            w[0] = wx / 32768.0f * k_gyro;
            w[1] = wy / 32768.0f * k_gyro;
            w[2] = wz / 32768.0f * k_gyro;
            
            if (w[0] >= k_gyro) w[0] -= 2 * k_gyro;
            if (w[1] >= k_gyro) w[1] -= 2 * k_gyro;
            if (w[2] >= k_gyro) w[2] -= 2 * k_gyro;
        }
    
        void get_angle(uint8_t *datahex) {
            const float k_angle = 180.0f;
            
            int16_t rx = (datahex[1] << 8) | datahex[0];
            int16_t ry = (datahex[3] << 8) | datahex[2];
            int16_t rz = (datahex[5] << 8) | datahex[4];
            
            Angle[0] = rx / 32768.0f * k_angle;
            Angle[1] = ry / 32768.0f * k_angle;
            Angle[2] = rz / 32768.0f * k_angle;
            
            if (Angle[0] >= k_angle) Angle[0] -= 2 * k_angle;
            if (Angle[1] >= k_angle) Angle[1] -= 2 * k_angle;
            if (Angle[2] >= k_angle) Angle[2] -= 2 * k_angle;
        }
    };


JY901B *jy901b = nullptr;

float servo_to_duty(float percent)
{
    return (SERVO_MAX_DUTY - SERVO_MIN_DUTY) * percent / 100.0 + SERVO_MIN_DUTY;
}

pwm::PWM *output1 = nullptr;
pwm::PWM *output2 = nullptr;
pwm::PWM *output3 = nullptr;
pwm::PWM *output4 = nullptr;
pwm::PWM *output = nullptr;


// 低通滤波器结构体
struct LowPassFilter {
    float last_value = 0;
    float alpha = 0.2f; // 滤波系数(0-1), 越小滤波越强
    
    float update(float new_value) {
        last_value = alpha * new_value + (1 - alpha) * last_value;
        return last_value;
    }
};

// 比例导引历史数据结构体(新增滤波)
struct PNHistory {
    float last_los_pitch = 0;
    float last_los_yaw = 0;
    uint64_t last_time = 0;
    
    // 新增低通滤波器
    LowPassFilter pitch_rate_filter;
    LowPassFilter yaw_rate_filter;
    
    // 初始化时可以设置滤波系数
    PNHistory(float alpha = 0.3f) {
        pitch_rate_filter.alpha = alpha;
        yaw_rate_filter.alpha = alpha;
    }
};

float motor_to_duty(float percent)
{
    return (MOTOR_MAX_DUTY - MOTOR_MIN_DUTY) * percent / 100.0 + MOTOR_MIN_DUTY;
}


std::string image_save_path = "/root/images/"; // 图片保存路径
int image_counter = 0;
const int MAX_IMAGES = 100; // 最大保存图片数量

void draw_debug_info(maix::image::Image* img, float fps, const std::vector<BlobCenter>& centers) {
    if (!img) return;
    
    // 绘制帧率信息
    std::string fps_text = "FPS: " + std::to_string(fps);
    img->draw_string(10, 10, fps_text, image::COLOR_GREEN, 1.0);
    
    // 绘制检测到的绿块信息
    std::string blob_text = "Blobs: " + std::to_string(centers.size());
    img->draw_string(10, 30, blob_text, image::COLOR_GREEN, 1.0);
    
    // 绘制每个绿块的中心坐标
    for (size_t i = 0; i < centers.size(); ++i) {
        std::string center_text = "Blob " + std::to_string(i) + ": (" + 
                                 std::to_string(centers[i].cx) + ", " + 
                                 std::to_string(centers[i].cy) + ")";
        img->draw_string(10, 50 + i * 20, center_text, image::COLOR_GREEN, 1.0);
        
        // 在图像上标记中心点
        img->draw_circle(centers[i].cx, centers[i].cy, 5, image::COLOR_RED, -1);
    }
}


/*
* 比例导引的基本思想：制导指令 = N × 视线角速率 × 接近速度
* 
* 代码中简化为：控制量 = k_p × 视线角速率
* 因为对于固定速度的飞行器，接近速度可以合并到比例系数中。
*/

// 计算比例导引
ControlOutput PNG_calculate(BlobCenter& target, 
                            image::Image& img,
                            float flight_pitch, // 飞镖实际pitch的角度
                            float flight_yaw,   // 飞镖实际yaw的角度
                            float flight_roll, // 飞镖实际roll的角度
                            PNHistory& history,
                            const GuidanceParams& params = Guidance_params) 
{
    ControlOutput output = {0, 0, 0};


    float h_fov = 67.38f; // 水平视场角(FOV_h)
    float v_fov = h_fov * img.height() / img.width(); //垂直视场角(FOV_v)
    
    // 焦距(f)(像素单位)
    float focal_length_x = img.width() / (2 * tan((h_fov * M_PI / 180.0f)/2));
    float focal_length_y = img.height() / (2 * tan((v_fov * M_PI / 180.0f)/2));

    // 从像素坐标到角度转换
    float image_yaw = target.cx - img.width()/2;
    float image_pitch = -(target.cy - img.height()/2);

    // 计算视线角
    float los_pitch = atan2(image_pitch, focal_length_y) * 180.0f / M_PI;
    float los_yaw = atan2(image_yaw, focal_length_x) * 180.0f / M_PI;

    // 使用低通滤波器平滑后的视线角变化率
    float los_pitch_rate = 0;
    float los_yaw_rate = 0;

    uint64_t current_time = time::ticks_ms();
    if (history.last_time > 0) {
        float dt = (current_time - history.last_time) / 1000.0f; //转换为秒
        if (dt > 0.001f && dt < 0.5f) {
            // 视线角速率计算
            float raw_pitch_rate = (los_pitch - history.last_los_pitch) / dt;
            float raw_yaw_rate = (los_yaw - history.last_los_yaw) / dt;
            // 低通滤波
            los_pitch_rate = history.pitch_rate_filter.update(raw_pitch_rate);
            los_yaw_rate = history.yaw_rate_filter.update(raw_yaw_rate);
        }
    }

    // 更新历史数据
    history.last_los_pitch = los_pitch;
    history.last_los_yaw = los_yaw;
    history.last_time = current_time;

    // 三维比例导引计算
    // 俯仰通道: 比例导引 + 飞行角度补偿
    output.pitch_delta = params.k_p * los_pitch_rate + params.flight_pitch_compen * flight_pitch;

    // 偏航通道: 与俯仰通道对称的处理方式
    output.yaw_delta = params.k_p * los_yaw_rate + params.flight_yaw_compen * flight_yaw;

    // 滚转通道: 主要用于稳定，基于当前滚转角度，使用直接阻尼控制，虽然这个数据并没有被用到
    output.roll_delta = - flight_roll;

    // 限制输出，用于控制舵机的转动速度（但是感觉可以不要）
    output.pitch_delta = std::clamp(output.pitch_delta, -params.max_rate, params.max_rate);
    output.yaw_delta = std::clamp(output.yaw_delta, -params.max_rate, params.max_rate);
    output.roll_delta = std::clamp(output.roll_delta, -params.max_rate, params.max_rate);

    return output;
}



///舵面控制算法
ServoCommands servo_calculate(const ControlOutput& control, 
                            float gyro_yaw, // yaw角速度
                            float gyro_roll, // roll角速度
                            float gyro_pitch, // 当前俯仰角速度
                            PIDController& pitch_pid,
                            uint64_t current_time, // 当前时间戳
                            float angle_of_attack) { // 实际攻角
    ServoCommands cmd;

    // 1. PID计算俯仰舵面指令
    float pitch_cmd = pitch_pid.update(control.pitch_delta, gyro_pitch, current_time, angle_of_attack);

    // 俯仰控制: 前后舵面差动，计算结果是个百分比
    pitch_cmd = pitch_cmd * Pitch_PID_params.pitch_kp;
    // 将角度变化量转换为舵面偏转指令
    
    // 滚转控制: 保持稳定(使用当前角度z)
    float yaw_cmd = (control.yaw_delta * Yaw_Control_params.yaw_angle_gain + gyro_yaw * Yaw_Control_params.yaw_gyro_gain) * Yaw_Control_params.yaw_kp;
    
    float roll_cmd = (control.roll_delta * Roll_Control_params.roll_angle_gain + gyro_roll * Roll_Control_params.roll_gyro_gain) * Roll_Control_params.roll_kp;

    // X形布局混合控制指令
    // 假设舵面编号如下：
    // 舵面1: 左上(135°)
    // 舵面2: 右上(45°)
    // 舵面3: 右下(315°)
    // 舵面4: 左下(225°)

    // 各舵面贡献系数(基于45°投影)
    const float cos45 = 0.7071f; // cos(45°)
    //yaw_cmd = 0;
    //pitch_cmd = 0;
    // 混合控制公式：
    // 每个舵面对俯仰和偏航的贡献需要乘以cos45
    // 滚转控制保持不变(所有舵面同向偏转产生滚转)
    
    cmd.servo1 = Guidance_params.servo1_neutral + cos45*(pitch_cmd + yaw_cmd) - roll_cmd;
    cmd.servo2 = Guidance_params.servo2_neutral + cos45*(-pitch_cmd + yaw_cmd) - roll_cmd;
    cmd.servo3 = Guidance_params.servo3_neutral + cos45*(-pitch_cmd - yaw_cmd) - roll_cmd;
    cmd.servo4 = Guidance_params.servo4_neutral + cos45*(pitch_cmd - yaw_cmd) - roll_cmd;
    
    // 限制舵面指令范围
    
    cmd.servo1 = std::clamp(cmd.servo1, (Guidance_params.servo1_neutral - Guidance_params.servo_min), (Guidance_params.servo1_neutral + Guidance_params.servo_max));
    cmd.servo2 = std::clamp(cmd.servo2, (Guidance_params.servo2_neutral - Guidance_params.servo_min), (Guidance_params.servo2_neutral + Guidance_params.servo_max));
    cmd.servo3 = std::clamp(cmd.servo3, (Guidance_params.servo3_neutral - Guidance_params.servo_min), (Guidance_params.servo3_neutral + Guidance_params.servo_max));
    cmd.servo4 = std::clamp(cmd.servo4, (Guidance_params.servo4_neutral - Guidance_params.servo_min), (Guidance_params.servo4_neutral + Guidance_params.servo_max));
    log::info("pitch_cmd=%.1f yaw_cmd=%.1f roll_cmd=%.1f", 
        pitch_cmd, yaw_cmd, roll_cmd);
    
    return cmd;
}

ServoCommands servo_roll_yaw_calculate(const ControlOutput& control, //需要达到的控制量delta
                                    float gyro_yaw, // yaw的速度
                                    float gyro_roll,// roll角速度
                                    float gyro_pitch, //pitch的角速度
                                    float aoa //攻角
                                ) { 
    ServoCommands cmd;
    static int detect_time = 0;

    // 检测大的pitch角速度（绝对值大于某个阈值)
    const float large_gyro_threshold = 25.0f; 
    if(fabs(gyro_pitch) > large_gyro_threshold)
    {
        detect_time += 1;
    }
    float pitch_cmd = 30;

    // 滚转控制: 保持稳定(使用当前角度z)
    float yaw_cmd = (control.yaw_delta * Yaw_Control_params.yaw_angle_gain + gyro_yaw * Yaw_Control_params.yaw_gyro_gain) * Yaw_Control_params.yaw_kp;
    float roll_cmd = (control.roll_delta * Roll_Control_params.roll_angle_gain + gyro_roll * Roll_Control_params.roll_gyro_gain) * Roll_Control_params.roll_kp;

    if(detect_time < 60)
    {
        pitch_cmd = 30;
        
    }
    else{
        pitch_cmd = 0;
    }

    yaw_cmd = 0;
    roll_cmd = 0;
    
    //roll_cmd = 1.5f;//roll增加为+的时候，修正舵面需要为负数
    // roll_cmd = 0;
    // yaw_cmd = 0;
    //float roll_cmd = 0;
    // X形布局混合控制指令
    // 假设舵面编号如下：
    // 舵面1: 左上(135°)
    // 舵面2: 右上(45°)
    // 舵面3: 右下(315°)
    // 舵面4: 左下(225°)

    // 各舵面贡献系数(基于45°投影)
    const float cos45 = 0.7071f; // cos(45°)
    // 混合控制公式：
    // 每个舵面对俯仰和偏航的贡献需要乘以cos45
    // 滚转控制保持不变(所有舵面同向偏转产生滚转)
    // log::info("Current Attitude: pitch_cmd=%.1f yaw_cmd=%.1f roll_cmd=%.1f", 
    //     control.pitch_delta, yaw_cmd, roll_cmd);

    cmd.servo1 = Guidance_params.servo1_neutral + cos45*(pitch_cmd + yaw_cmd) + roll_cmd;
    cmd.servo2 = Guidance_params.servo2_neutral + cos45*(-pitch_cmd + yaw_cmd) + roll_cmd;
    cmd.servo3 = Guidance_params.servo3_neutral + cos45*(-pitch_cmd - yaw_cmd) + roll_cmd;
    cmd.servo4 = Guidance_params.servo4_neutral + cos45*(pitch_cmd - yaw_cmd) + roll_cmd;

    // 限制舵面指令范围
    cmd.servo1 = std::clamp(cmd.servo1, (Guidance_params.servo1_neutral - Guidance_params.servo_min), (Guidance_params.servo1_neutral + Guidance_params.servo_max));
    cmd.servo2 = std::clamp(cmd.servo2, (Guidance_params.servo2_neutral - Guidance_params.servo_min), (Guidance_params.servo2_neutral + Guidance_params.servo_max));
    cmd.servo3 = std::clamp(cmd.servo3, (Guidance_params.servo3_neutral - Guidance_params.servo_min), (Guidance_params.servo3_neutral + Guidance_params.servo_max));
    cmd.servo4 = std::clamp(cmd.servo4, (Guidance_params.servo4_neutral - Guidance_params.servo_min), (Guidance_params.servo4_neutral + Guidance_params.servo_max));

    return cmd;
}

// 全局变量
float acc_x, acc_y, acc_z;
float gyro_x, gyro_y, gyro_z;
float angle_x, angle_y, angle_z;

std::mutex center_mutex;

std::vector<BlobCenter> shared_centers;
std::condition_variable queue_cv;   //条件变量，用于线程间通知
std::atomic<bool> running(true);    //程序是否在运行状态（线程退出的标志）

std::vector<std::vector<int>> green_thresholds = {
    {0, 80, -120, -10, 0, 30}
};
// -----------------------------
// 采集图像和处理图像线程
// -----------------------------
void capture_thread_process_func(camera::Camera *cam)
{
    // 创建保存图片的目录
    // mkdir(image_save_path.c_str(), 0777);


    while (running) {
        uint64_t t_read_start = time::ticks_ms();
        image::Image *img = cam->read(); 
        uint64_t t_read_end = time::ticks_ms();

        if (!img) {
            log::error("Failed to read image");
            time::sleep_ms(10);
            continue;
        }

        // 保存图片
        // if (image_counter < MAX_IMAGES) {
        //     // 计算帧率
        //     static uint64_t last_time = time::ticks_ms();
        //     uint64_t current_time = time::ticks_ms();
        //     float fps = 1000.0f / (current_time - last_time);
        //     last_time = current_time;
            
        //     // 获取当前检测到的绿块信息
        //     std::vector<BlobCenter> current_centers;
        //     {
        //         std::lock_guard<std::mutex> lock(center_mutex);
        //         current_centers = shared_centers;
        //     }
            
        //     // 在图像上绘制调试信息
        //     draw_debug_info(img, fps, current_centers);
            
        //     std::string filename = image_save_path + "frame_" + std::to_string(image_counter) + ".jpg";
        //     if (img->save(filename.c_str(), 95) == err::ERR_NONE){
        //         log::info("Saved image: %s", filename.c_str());
        //         image_counter++;
        //     } else {
        //         log::error("Failed to save image: %s", filename.c_str());
        //     }
        // }

        uint64_t t_process_start = time::ticks_ms();

        /*
        * std::vector 是 C++ 标准库（Standard Template Library, STL）提供的动态数组容器。
        * <maix::image::Blob>, MaixCAM SDK 提供的一个类（结构体），代表的是一个图像中的色块（Blob）
        * std::vector<maix::image::Blob>类型是 maix::image::Blob, 容器是 std::vector
        */
       std::vector<maix::image::Blob> blobs = img->find_blobs(
                        green_thresholds, 
                        false,
                        {0, 0, img->width(), img->height()},
                        2, 
                        2, 
                        10, 
                        5, 
                        true, 
                        10,
                        16,
                        16);



        // 提取质心坐标
        std::vector<BlobCenter> centers;
        for (auto &blob : blobs) {
            centers.push_back({blob.cx(), blob.cy()});
            //log::info("blob: [cx]%d [cy]%d [ch]%.1f", blob.cx(), blob.cy(), blob.pixels());
        }
        // 线程安全更新共享数据
        {
            std::lock_guard<std::mutex> lock(center_mutex);
            shared_centers = std::move(centers);
        }

        // 释放图像资源
        delete img;

        uint64_t t_process_end = time::ticks_ms();
        float fps = time::fps();
        // log::info("Timing: [read]%llums [process]%llums [total]%llums | fps: %.1f",
        // t_read_end - t_read_start,
        // t_process_end - t_process_start,
        // t_process_end - t_read_start,   
        // fps);

    }
}

// -----------------------------
// 飞控线程
// -----------------------------
void process_control_func(ADC &adc, camera::Camera *cam) {
    GuidanceParams guidance_params = Guidance_params;

    static PNHistory pn_history; // 初始化滤波器，使用默认参数

    static PIDController pitch_pid(Pitch_PID_params, Alpha_comp_params); // 使用main.hpp中的默认参数
    pitch_pid.reset();

    // 初始yaw角记录
    static bool initial_yaw_recorded = false;
    static float initial_yaw = 0.0f;

    // 实际飞行的方向
    float flight_yaw = 0.0f;
    float flight_pitch = 0.0f;
    float flight_roll = 0.0f;

    uint64_t process_control_time = time::ticks_ms();

    float yaw_location = 0.0f;
    float roll_location = 0.0f;

    AsyncLogger debug_logger("/root/logs/debug_log.csv");

    while (running) {
        uint64_t current_time = time::ticks_ms();
        // 获取共享数据
        std::vector<BlobCenter> local_centers;
        float local_angle_x, local_gyro_x, local_acc_x, local_angle_y, local_gyro_y, local_angle_z, local_gyro_z;
        {
            std::lock_guard<std::mutex> lock(imu_mutex);
            local_angle_x = jy901b->Angle[0];  // Pitch
            local_angle_y = jy901b->Angle[1];   // Roll
            local_angle_z = jy901b->Angle[2];   // Yaw
            local_gyro_x = jy901b->w[0];        // Pitch角速度
            local_gyro_y = jy901b->w[1];        // Roll角速度
            local_gyro_z = jy901b->w[2];        // Yaw角速度
            local_acc_x = jy901b->a[0];         // Pitch 加速度
        }
        local_centers = shared_centers;
        
        // 读取并记录ADC值，其中angle为风标角度， 让风标被风吹得朝镖体上方时候为负
        float vol = adc.read_vol(); 
        float angle_of_attack = vol * 360 / 3.37 - 157.0f;//攻角，让风标水平为0


        
        std::lock_guard<std::mutex> lock(angle_mutex);
        flight_pitch = local_angle_x ;// 镖体pitch实际飞行方向，向上为正
        float gyro_pitch = local_gyro_x;
        float acc_pitch = local_acc_x;
        // 计算初始yaw角
        if (!initial_yaw_recorded) {
            initial_yaw = local_angle_z;
            initial_yaw_recorded = true;
            // log::info("Initial yaw recorded: %.1f degrees", initial_yaw);
        }

        // 计算yaw的实际偏转
        flight_yaw = initial_yaw - local_angle_z;// 让镖头向右滚转为正
        if(flight_yaw > 180)
        {
            flight_yaw = flight_yaw - 360;
        }
        else if(flight_yaw < -180)
        {
            flight_yaw = flight_yaw + 360;
        }
        float gyro_yaw = -local_gyro_z;

        flight_roll = local_angle_y; //让镖头roll向右滚转为正
        float gyro_roll = local_gyro_y;
        // log::info("Current Attitude: pitch=%.1f angle_of_attack=%.1f flight_pitch=%.1f roll=%.1f yaw=%.1f", 
        //     local_angle_x, angle_of_attack, flight_pitch, local_angle_y, flight_yaw);
        ServoCommands cmd;
        DebugLogEntry entry;

        // 检测到目标，执行制导
        if (!local_centers.empty()) {
            // 获取当前图像尺寸
            int img_width = cam->width();
            int img_height = cam->height();
            
            // 使用第一个检测到的目标
            BlobCenter target = local_centers[0];

            ControlOutput control = {0, 0, 0};
            
            // 执行比例导引计算
            image::Image *img = cam->read();
            if (img) {
                control = PNG_calculate(
                    target, 
                    *img,
                    flight_pitch, // 实际pitch角度
                    flight_yaw,  // 实际yaw角度
                    flight_roll, // 实际roll的角度
                    pn_history,
                    guidance_params);
                delete img;
            }
            
            // 计算舵面指令
            cmd = servo_calculate(
                control, 
                gyro_yaw, // yaw的角速度
                gyro_roll, // roll的角速度
                gyro_pitch, // 当前俯仰角速度
                pitch_pid,
                current_time,    // 当前时间戳
                angle_of_attack     // 实际攻角
            );
            
            // 替换原有的单个舵机更新代码：
            std::vector<std::pair<pwm::PWM*, float>> duties = {
                {output1, servo_to_duty(cmd.servo1)},
                {output2, servo_to_duty(cmd.servo2)},
                {output3, servo_to_duty(cmd.servo3)},
                {output4, servo_to_duty(cmd.servo4)},
            };
            pwm::PWM::update_duties(duties);
            // 记录调试信息到文件
            entry.timestamp = time::ticks_ms();
            entry.flight_roll = flight_roll;
            entry.gyro_roll = gyro_roll;
            entry.flight_yaw = flight_yaw;
            entry.gyro_yaw = gyro_yaw;
            entry.flight_pitch = flight_pitch;
            entry.gyro_pitch = gyro_pitch;
            entry.angle_of_attack = angle_of_attack;
            entry.target_pitch = flight_pitch + control.pitch_delta;
            entry.target_yaw = flight_yaw + control.yaw_delta;
            entry.blob_cx = target.cx;
            entry.blob_cy = target.cy;
            entry.cmd_servo1 = cmd.servo1;
            entry.cmd_servo2 = cmd.servo2;
            entry.cmd_servo3 = cmd.servo3;
            entry.cmd_servo4 = cmd.servo4;
            
            debug_logger.log(entry);
            uint64_t data_time1 = time::ticks_ms();
            
            // log::info("time=%dms flight_roll=%.1f gyro_roll=%.1f flight_yaw=%.1f gyro_yaw=%.1f flight_pitch=%.1f gyro_pitch=%.1f angle_of_attack=%.1f cmd.servo1=%.1f cmd.servo2=%.1f cmd.servo3=%.1f cmd.servo4=%.1f", 
            //     data_time1 - current_time, flight_roll, gyro_roll, flight_yaw, gyro_yaw, flight_pitch, gyro_pitch, angle_of_attack, cmd.servo1, cmd.servo2, cmd.servo3, cmd.servo4);


        } else {
            uint64_t control_time = time::ticks_ms();

            ControlOutput control = {0, 0, 0};

            if(control_time - process_control_time >= 2000)
            {
                yaw_location = -yaw_location;
                roll_location = -roll_location;
                process_control_time = time::ticks_ms();
            }
                
            control = {
                0,  // pitch_delta
                flight_yaw + yaw_location,  // yaw_delta
                flight_roll + roll_location // roll_delta
            };
            // 计算舵面指令 
            cmd = servo_roll_yaw_calculate(
                control, 
                gyro_yaw, // yaw的速度
                gyro_roll, // roll的速度
                gyro_pitch, //pitch的角速度
                angle_of_attack
            );
            
            uint64_t data_time = time::ticks_ms();
            
            // 替换原有的单个舵机更新代码：
            std::vector<std::pair<pwm::PWM*, float>> duties = {
                {output1, servo_to_duty(cmd.servo1)},
                {output2, servo_to_duty(cmd.servo2)},
                {output3, servo_to_duty(cmd.servo3)},
                {output4, servo_to_duty(cmd.servo4)},
            };
            pwm::PWM::update_duties(duties);

            // 记录调试信息到文件
            entry.timestamp = time::ticks_ms();
            entry.flight_roll = flight_roll;
            entry.gyro_roll = gyro_roll;
            entry.flight_yaw = flight_yaw;
            entry.gyro_yaw = gyro_yaw;
            entry.flight_pitch = flight_pitch;
            entry.gyro_pitch = gyro_pitch;
            entry.angle_of_attack = angle_of_attack;
            entry.target_pitch = flight_pitch;
            entry.target_yaw = 0;
            entry.blob_cx = 400.0f;
            entry.blob_cy = 400.0f;
            entry.cmd_servo1 = cmd.servo1;
            entry.cmd_servo2 = cmd.servo2;
            entry.cmd_servo3 = cmd.servo3;
            entry.cmd_servo4 = cmd.servo4;
            
            debug_logger.log(entry);
            uint64_t data_time1 = time::ticks_ms();
            
            // log::info("time=%dms flight_roll=%.1f gyro_roll=%.1f flight_yaw=%.1f gyro_yaw=%.1f flight_pitch=%.1f gyro_pitch=%.1f angle_of_attack=%.1f cmd.servo1=%.1f cmd.servo2=%.1f cmd.servo3=%.1f cmd.servo4=%.1f", 
            //     data_time1 - current_time, flight_roll, gyro_roll, flight_yaw, gyro_yaw, flight_pitch, gyro_pitch, angle_of_attack, cmd.servo1, cmd.servo2, cmd.servo3, cmd.servo4);

        }
        
        //time::sleep_ms(10); // 控制循环频率
    }
}




// -----------------------------
// 主函数
// -----------------------------
int _main(int argc, char *argv[])
{
    // 舵机的初始化
    int pwm_id1 = 7;
    int pwm_id2 = 6;
    int pwm_id3 = 5;
    int pwm_id4 = 4;
    int pwm_id = 8;
    pinmap::set_pin_function("A19", "PWM7");
    pinmap::set_pin_function("A18", "PWM6");
    pinmap::set_pin_function("A17", "PWM5");
    pinmap::set_pin_function("A16", "PWM4");
    pinmap::set_pin_function("P22", "PWM8");

    output1 = new pwm::PWM(pwm_id1, SERVO_PERIOD, servo_to_duty(50), true);
    output2 = new pwm::PWM(pwm_id2, SERVO_PERIOD, servo_to_duty(50), true);
    output3 = new pwm::PWM(pwm_id3, SERVO_PERIOD, servo_to_duty(50), true);
    output4 = new pwm::PWM(pwm_id4, SERVO_PERIOD, servo_to_duty(50), true);
    output = new pwm::PWM(pwm_id, MOTOR_PERIOD, motor_to_duty(0), true);

    fs::mkdir("/root/logs", true);
    //初始化相机
    camera::Camera cam(320, 240, image::Format::FMT_RGB888, nullptr, 90, 3, true, false);
    err::Err e = cam.open();
    err::check_raise(e, "camera open failed");
    
    cam.exp_mode(1);// 设置为手动曝光模式
    cam.exposure(20);// 设置固定的曝光时间，单位us
    cam.gain(4096);// 设置固定的增益数值
    
    // // 同时设置 vflip 和 hmirror（相当于旋转 180 度）
    cam.vflip(1);  // 垂直翻转
    cam.hmirror(1); // 水平镜像

    // 初始化adc-as5600磁编码器
    ADC adc(0, RES_BIT_12);

    // 设置引脚功能
    pinmap::set_pin_function("A28", "UART2_TX");
    pinmap::set_pin_function("A29", "UART2_RX");

    jy901b = new JY901B("/dev/ttyS2", 230400);
    //启动涵道电机
    output->duty(motor_to_duty(50));
    time::sleep_ms(2000); 
    output->duty(motor_to_duty(0));
    time::sleep_ms(2000); 
    output->duty(motor_to_duty(100));
    time::sleep_ms(200); 
    // 设置接收回调
    
    // 启动线程时传递相机对象
    std::thread capture_thread(capture_thread_process_func, &cam);
    std::thread process_thread1(process_control_func, std::ref(adc), &cam); 

    log::info("Threads started. Press Ctrl+C to exit.");

    while (!app::need_exit())
    {
        /*让主线程不做任何有意义的事情，只是等待其他线程完成任务。如果不加这个等待，主线程会很快退出，导致程序提前终止*/
        time::sleep_ms(100); // 主线程等待
    }

    // 通知退出
    running = false;
    queue_cv.notify_all();

    // 等待线程退出
    capture_thread.join();
    process_thread1.join();
    // 在程序退出前的清理部分：
    if(output1) { delete output1; output1 = nullptr; }
    if(output2) { delete output2; output2 = nullptr; }
    if(output3) { delete output3; output3 = nullptr; }
    if(output4) { delete output4; output4 = nullptr; }
    if(output) { delete output; output = nullptr; }

    log::info("Program exited cleanly.");
    return 0;
}

int main(int argc, char *argv[])
{
    sys::register_default_signal_handle();
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}

