#pragma once

#include <sys/stat.h> // 用于创建目录
#include <chrono>     // 用于时间戳
#include <sys/stat.h> // 用于创建目录
#include <atomic> // 添加atomic支持
#include <thread>
#include <mutex>
#include "maix_fs.hpp" // 添加文件系统支持
#include <queue>
#include <condition_variable>
#include <vector>
#include <utility> // 用于 std::pair

extern maix::peripheral::pwm::PWM *output1, *output2, *output3, *output4, *output;
// 舵机的初始化
#define SERVO_PERIOD 400     // 400Hz 2.5ms
#define SERVO_MIN_DUTY 20  // 20% -> 0.5ms
#define SERVO_MAX_DUTY 100  // 100% -> 2.5ms


// 涵道电机的初始化
#define MOTOR_PERIOD 200
#define MOTOR_MIN_DUTY 20 // 20% -> 0ms
#define MOTOR_MAX_DUTY 40 // 40% -> 2ms

// 比例导引参数
struct GuidanceParams {
    float k_p;       // 比例导引系数
    float max_rate;  // 输出的pitch roll yaw的delta最大绝对数值（可以理解为舵机限制的速度)
    float servo1_neutral; //舵机1中立位置
    float servo2_neutral; //舵机2中立位置
    float servo3_neutral; //舵机3中立位置
    float servo4_neutral; //舵机4中立位置
    float servo_min; //舵机最小位置限幅(往小多少)
    float servo_max; //舵机最大位置限幅（往大多少）
    float flight_pitch_compen; // 飞行pitch角度补偿
    float flight_yaw_compen;// 飞行yaw角度补偿
    float flight_roll_compen;// 飞行roll角度补偿
    float init_angle;// 攻角初始值(度)
};

#define DEVICE_YELLOW
// #define DEVICE_PINK

#ifdef DEVICE_YELLOW
// 默认导引参数
const GuidanceParams Guidance_params = { // 制导参数
    3.0f, //png的比例系数
    15.0f, // 输出的pitch roll yaw的delta最大绝对数值
    55.0f,  //舵机1中立位置
    56.0f,  //舵机2中立位置
    50.0f,  //舵机3中立位置
    48.0f,  //舵机4中立位置
    18.0f, //舵机最小位置限幅（往小多少）
    18.0f, //舵机最大位置限幅（往大多少）
    0.0f, //飞行pitch角度补偿
    -0.0f, // 飞行yaw角度补偿
    -0.0f, // 飞行roll角度补偿
    157.0f //攻角初始值(度)
};
#elif defined(DEVICE_PINK)
const GuidanceParams Guidance_params = { // 制导参数
    3.0f, //png的比例系数
    15.0f, // 输出的pitch roll yaw的delta最大绝对数值
    50.0f,  //舵机1中立位置
    50.0f,  //舵机2中立位置
    50.0f,  //舵机3中立位置
    50.0f,  //舵机4中立位置
    32.0f, //舵机最小位置限幅
    68.0f, //舵机最大位置限幅
    0.15f, //飞行pitch角度补偿
    -0.15f, // 飞行yaw角度补偿
    -0.5f, // 飞行roll角度补偿
    157.0f //攻角初始值(度)
};
#else
#error "Please define device"
#endif

// PID pitch 控制器
struct PID_Pitch {
    float Kp_angle;           // 角度的比例项系数
    float Kp_gyro;
    float Ki;           // 积分项系数
    float Kd;           // 微分项系数
    float max_integral; // 积分限幅
    float max_output;   // 输出限幅
    float alpha_comp;   // 攻角补偿系数，没用到
    float pitch_kp;    // pitch在舵面控制中的比例
};


// PID控制器参数
const PID_Pitch Pitch_PID_params = {
    0.8f,   // Kp角度
    -0.2f,//kp角速度
    0.05f,  // Ki
    0.001f,   // Kd
    50.0f,  // 积分限幅
    100.0f, // 输出限幅
    -0.2f,    // 攻角补偿系数，没用到
    0.3,
};

// roll 控制器
struct Roll_Control{
    float roll_kp; // roll在舵面控制中的比例
    float roll_gyro_gain; // roll的速度的增益
    float roll_angle_gain; // roll的角度的增益
};

const Roll_Control Roll_Control_params = {
    1, // roll在舵面控制中的比例
    0, // roll的角速度的增益
    0  // roll的角度的增益
};

// yaw 控制器
struct Yaw_Control{
    float yaw_kp; // yaw在舵面控制中的比例
    float yaw_gyro_gain; // yaw的速度的增益
    float yaw_angle_gain; // yaw的角度的增益
};

const Yaw_Control Yaw_Control_params = {
    1, // yaw在舵面控制中的比例
    0, // yaw的速度的增益
    0.25  // yaw的角度的增益
};

// 攻角非线性补偿
struct AlphaCompensationParams {
    float static_comp;       // 基本静态补偿系数
    float nonlinear_factor;  // 非线性补偿因子
    float rate_comp;         // 攻角速率补偿系数
    float max_comp;         // 最大补偿量限制
    float adapt_gain;       // 自适应调整增益
};

// 默认攻角补偿参数
const AlphaCompensationParams Alpha_comp_params = {
    -0.2f,   // 基本静态补偿系数
    0.01f,  // 非线性补偿因子
    0.05f,   // 攻角速率补偿系数
    20.0f,   // 最大补偿量限制
    0.01f    // 自适应调整增益
};


// 存储三个通道的控制输出量
struct ControlOutput {
    float pitch_delta; // 俯仰角变化量(度)
    float yaw_delta;   // 偏航角变化量(度)
    float roll_delta;  // 滚转角变化量(度)
};



struct ServoCommands {
    float servo1; // 左上舵机
    float servo2; // 右上舵机
    float servo3; // 左下舵机
    float servo4; // 右上舵机
};

// 修改共享数据结构
struct BlobCenter {
    int cx;
    int cy;
};

// PID控制器类
class PIDController {
    public:
        PIDController(const PID_Pitch& params, const AlphaCompensationParams& alpha_params)
                    : params(params), alpha_params(alpha_params), 
                    integral(0), prev_error(0), last_time(0),
                    last_angle(0), adapted_comp(alpha_params.static_comp) {}
    
        float update(float pitch_delta, float gyro_pitch, uint64_t current_time, float angle) {
            // 计算时间步长（秒）
            float dt = (last_time > 0) ? (current_time - last_time) / 1000.0f : 0.01f;
            dt = std::clamp(dt, 0.001f, 0.1f); // 限制在1ms-100ms之间
            
            float error = pitch_delta;
            
            // 积分项（抗饱和处理）
            integral += error * dt;
            integral = std::clamp(integral, -params.max_integral, params.max_integral);
            
            // 微分项
            float derivative = (last_time > 0) ? (error - prev_error) / dt : 0;

            // 计算攻角补偿
            float angle_rate = (angle - last_angle) / dt;
            float compensation = calculate_alpha_compensation(angle, angle_rate, error);
            
            // 记录状态
            prev_error = error;
            last_time = current_time;
            last_angle = angle;
            
            // PID输出
            float output = params.Kp_angle * error + 
                            params.Kp_gyro * gyro_pitch + 
                            params.Ki * integral + 
                            params.Kd * derivative +
                            compensation;
            
            return std::clamp(output, -params.max_output, params.max_output);
        }
    
        void reset() {
            integral = 0;
            prev_error = 0;
            last_time = 0;
            last_angle = 0;
            adapted_comp = alpha_params.static_comp;
        }
    
    private:
        PID_Pitch params;
        AlphaCompensationParams alpha_params;
        float integral;
        float prev_error;
        uint64_t last_time;
        float last_angle;
        float adapted_comp; // 自适应调整后的补偿系数
    
        // 计算攻角补偿
        float calculate_alpha_compensation(float angle, float angle_rate, float error) {
            // 静态非线性补偿
            float static_comp = angle * (adapted_comp + alpha_params.nonlinear_factor * fabs(angle));
            
            // 动态速率补偿
            float dynamic_comp = angle_rate * alpha_params.rate_comp;
            
            // 自适应调整补偿系数
            // if (fabs(error) > 5.0f) { // 当误差较大时调整
            //     adapted_comp += alpha_params.adapt_gain * (error > 0 ? -1 : 1);
            //     adapted_comp = std::clamp(adapted_comp, 
            //                             alpha_params.static_comp * 0.5f, 
            //                             alpha_params.static_comp * 1.5f);
            // }
            
            // 总和并限幅
            float total_comp = static_comp + dynamic_comp;
            return std::clamp(total_comp, -alpha_params.max_comp, alpha_params.max_comp);
        }        
};


struct DebugLogEntry {
    uint64_t timestamp;
    float flight_roll;
    float gyro_roll;
    float flight_yaw;
    float gyro_yaw;
    float flight_pitch;
    float gyro_pitch;
    float angle_of_attack;
    float target_pitch;
    float target_yaw;
    float blob_cx;
    float blob_cy;
    float cmd_servo1;
    float cmd_servo2;
    float cmd_servo3;
    float cmd_servo4;
};

class AsyncLogger {
    private:
        std::queue<DebugLogEntry> log_queue;
        std::mutex queue_mutex;
        std::condition_variable queue_cv;
        std::atomic<bool> running{false};
        std::thread log_thread;
        std::string filepath;
        maix::fs::File* log_file;
        size_t buffer_size;
        std::string buffer;
        std::atomic<bool> needs_flush{false};
        std::chrono::steady_clock::time_point last_flush_time;
        const std::chrono::milliseconds flush_interval{500}; // 每500ms强制刷新一次
        
        // 从路径中提取文件名（不含目录）
        std::string get_filename(const std::string& path) {
            size_t pos = path.find_last_of("/\\");
            if(pos == std::string::npos) {
                return path;
            }
            return path.substr(pos + 1);
        }
        
        // 从文件名中提取扩展名
        std::string get_extension(const std::string& filename) {
            size_t pos = filename.find_last_of(".");
            if(pos == std::string::npos) {
                return "";
            }
            return filename.substr(pos);
        }
        
        // 从文件名中去除扩展名
        std::string remove_extension(const std::string& filename) {
            size_t pos = filename.find_last_of(".");
            if(pos == std::string::npos) {
                return filename;
            }
            return filename.substr(0, pos);
        }
        
        // 生成不重复的文件名
        std::string generate_unique_filename(const std::string& path) {
            std::string filename = get_filename(path);
            std::string dir = path.substr(0, path.size() - filename.size());
            std::string ext = get_extension(filename);
            std::string name_no_ext = remove_extension(filename);
            
            int counter = 0;
            std::string new_filename;
            
            do {
                if(counter == 0) {
                    new_filename = dir + name_no_ext + ext;
                } else {
                    new_filename = dir + name_no_ext + "_" + std::to_string(counter) + ext;
                }
                counter++;
            } while(maix::fs::exists(new_filename.c_str()));
            
            return new_filename;
        }
        
        // 日志线程函数
        void log_thread_func() {
            while(running) {
                std::unique_lock<std::mutex> lock(queue_mutex);
                
                // 等待条件：有数据、需要刷新、程序退出或达到刷新间隔
                auto now = std::chrono::steady_clock::now();
                bool timeout = (now - last_flush_time) >= flush_interval;
                
                queue_cv.wait_for(lock, flush_interval, [this, timeout]() { 
                    return !buffer.empty() || !running || needs_flush || timeout; 
                });
                
                // 检查是否需要立即刷新
                bool do_flush = needs_flush || timeout;
                needs_flush = false;
                if(timeout) last_flush_time = now;
                
                // 交换缓冲区内容
                std::string temp_buffer;
                temp_buffer.swap(buffer);
                
                lock.unlock();
                
                // 写入文件
                if((!temp_buffer.empty() || do_flush) && log_file) {
                    if(!temp_buffer.empty()) {
                        size_t written = log_file->write(temp_buffer.c_str(), temp_buffer.size());
                        if(written != temp_buffer.size()) {
                            maix::log::error("Failed to write log data: wrote %zu/%zu bytes", 
                                           written, temp_buffer.size());
                        }
                    }
                }
            }
        }
        
        // 强制刷新缓冲区
        void flush_buffer() {
            std::string temp_buffer;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                temp_buffer.swap(buffer);
            }
            
            if((!temp_buffer.empty() || needs_flush) && log_file) {
                if(!temp_buffer.empty()) {
                    size_t written = log_file->write(temp_buffer.c_str(), temp_buffer.size());
                    if(written != temp_buffer.size()) {
                        maix::log::error("Failed to write log data during flush: wrote %zu/%zu bytes",
                                       written, temp_buffer.size());
                    }
                }
                if(!log_file->flush()) {
                    maix::log::error("Failed to flush log data during flush");
                }
            }
        }
        
    public:
        AsyncLogger(const std::string& path, size_t buf_size = 16384) 
        : buffer_size(buf_size), last_flush_time(std::chrono::steady_clock::now()) {
            // 生成唯一文件名
            filepath = generate_unique_filename(path);
            
            // 确保目录存在
            std::string dir = filepath.substr(0, filepath.find_last_of("/\\"));
            if(!dir.empty()) {
                if(!maix::fs::mkdir(dir.c_str(), true)) {
                    maix::log::error("Failed to create log directory: %s", dir.c_str());
                }
            }
            
            // 以写入模式打开文件（覆盖模式）
            log_file = maix::fs::open(filepath.c_str(), "w");
            if(log_file) {
                // 写入CSV头部
                std::string header = "timestamp,flight_roll,gyro_roll,flight_yaw,gyro_yaw,flight_pitch,gyro_pitch,angle_of_attack,target_pitch, target_yaw, blob_cx, blob_cy, cmd_servo1,cmd_servo2,cmd_servo3,cmd_servo4\n";
                if(log_file->write(header.c_str(), header.size()) != header.size()) {
                    maix::log::error("Failed to write header to log file");
                }
                if(!log_file->flush()) {
                    maix::log::error("Failed to flush header to log file");
                }
                maix::log::info("Logging to file: %s", filepath.c_str());
            } else {
                maix::log::error("Failed to open log file in write mode: %s", filepath.c_str());
            }
            
            // 启动日志线程
            running = true;
            log_thread = std::thread(&AsyncLogger::log_thread_func, this);
        }
        
        ~AsyncLogger() {
            running = false;
            queue_cv.notify_all();
            
            if(log_thread.joinable()) {
                log_thread.join();
            }
            
            // 确保所有数据写入
            flush_buffer();
            
            if(log_file) {
                // 额外刷新确保数据写入
                for(int i = 0; i < 3; ++i) { // 尝试最多3次
                    if(log_file->flush()) break;
                    maix::log::warn("Retrying flush... (%d/3)", i+1);
                    maix::time::sleep_ms(10);
                }
                log_file->close();
                delete log_file;
            }
        }
        
        void log(const DebugLogEntry& entry) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            
            // 构造日志行
            std::string line = 
                std::to_string(entry.timestamp) + "," +
                std::to_string(entry.flight_roll) + "," +
                std::to_string(entry.gyro_roll) + "," +
                std::to_string(entry.flight_yaw) + "," +
                std::to_string(entry.gyro_yaw) + "," +
                std::to_string(entry.flight_pitch) + "," +
                std::to_string(entry.gyro_pitch) + "," +
                std::to_string(entry.angle_of_attack) + "," +
                std::to_string(entry.target_pitch) + "," +
                std::to_string(entry.target_yaw) + "," +
                std::to_string(entry.blob_cx) + "," +
                std::to_string(entry.blob_cy) + "," +
                std::to_string(entry.cmd_servo1) + "," +
                std::to_string(entry.cmd_servo2) + "," +
                std::to_string(entry.cmd_servo3) + "," +
                std::to_string(entry.cmd_servo4) + "\n";
            
            // 添加到缓冲区
            buffer += line;
            
            // 如果缓冲区达到阈值或需要立即刷新，通知日志线程
            if(buffer.size() >= buffer_size || needs_flush) {
                queue_cv.notify_one();
            }
        }
        
        void flush() {
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                needs_flush = true;
            }
            queue_cv.notify_one();
        }
    };

// 在main.hpp末尾添加
void draw_debug_info(maix::image::Image* img, float fps, const std::vector<BlobCenter>& centers);