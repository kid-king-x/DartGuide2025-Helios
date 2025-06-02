/**
 * @author neucrack@sipeed, lxowalle@sipeed
 * @copyright Sipeed Ltd 2023-
 * @license Apache 2.0
 * @update 2023.9.8: Add framework, create this file.
 */

#include "maix_pwm.hpp"
#include "maix_log.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define EXPORT_PATH "/sys/class/pwm/pwmchip%d/export"
#define UNEXPORT_PATH "/sys/class/pwm/pwmchip%d/unexport"
#define DUTYCYCLE_PATH "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle"
#define PERIOD_PATH "/sys/class/pwm/pwmchip%d/pwm%d/period"
#define ENABLE_PATH "/sys/class/pwm/pwmchip%d/pwm%d/enable"
#define freq_to_period_ns(freq) (1000 * 1000 * 1000 / (freq))

namespace maix::peripheral::pwm
{
    static int _pwm_id_to_chipid(int pwm_id, int &chip_id, int &offset)
    {
        chip_id = 0;
        offset = 0;
        if (pwm_id >= 0 && pwm_id <= 3)
        {
            chip_id = 0;
            offset = pwm_id;
        }
        else if (pwm_id >= 4 && pwm_id <= 7)
        {
            chip_id = 4;
            offset = pwm_id - 4;
        }
        else if (pwm_id >= 8 && pwm_id <= 11)
        {
            chip_id = 8;
            offset = pwm_id - 8;
        }
        else if (pwm_id >= 12 && pwm_id <= 15)
        {
            chip_id = 12;
            offset = pwm_id - 12;
        }
        else
        {
            log::error("pwm_id %d is not support\r\n", pwm_id);
            return -1;
        }
        return 0;
    }

    static err::Err _pwm_unexport(int chip_id, int offset)
    {
        int fd;
        char buf[100];
        snprintf(buf, sizeof(buf), UNEXPORT_PATH, chip_id);
        fd = ::open(buf, O_WRONLY);
        if (fd < 0)
        {
            log::error("open %s failed\r\n", buf);
            return err::ERR_IO;
        }
        snprintf(buf, sizeof(buf), "%d", offset);
        if ((int)strlen(buf) != ::write(fd, buf, strlen(buf)))
        {
            log::error("write %s > %s failed\r\n", buf, UNEXPORT_PATH);
            close(fd);
            return err::ERR_IO;
        }
        fsync(fd);
        close(fd);
        return err::ERR_NONE;
    }

    static err::Err _pwm_export(int chip_id, int offset)
    {
        int fd;
        char buf[100];
        // check if ENABLE_PATH exists, return ERR_NONE
        snprintf(buf, sizeof(buf), ENABLE_PATH, chip_id, offset);
        if (access(buf, F_OK) == 0)
        {
            log::warn("pwm %d already exported, unexport first automatically", chip_id + offset);
            _pwm_unexport(chip_id, offset);
        }
        snprintf(buf, sizeof(buf), EXPORT_PATH, chip_id);
        fd = ::open(buf, O_WRONLY);
        if (fd < 0)
        {
            log::error("open %s failed\r\n", buf);
            return err::ERR_IO;
        }
        snprintf(buf, sizeof(buf), "%d", offset);
        if ((int)strlen(buf) != ::write(fd, buf, strlen(buf)))
        {
            log::error("write %s > %s failed\r\n", buf, EXPORT_PATH);
            close(fd);
            return err::ERR_IO;
        }
        fsync(fd);
        close(fd);
        return err::ERR_NONE;
    }

    static err::Err _pwm_set_period(int chip_id, int offset, int peroid_ns)
    {
        int fd;
        char buf[100];
        snprintf(buf, sizeof(buf), PERIOD_PATH, chip_id, offset);
        fd = ::open(buf, O_RDWR);
        if (fd < 0)
        {
            log::error("open %s failed\r\n", buf);
            return err::ERR_IO;
        }
        snprintf(buf, sizeof(buf), "%d", peroid_ns);
        if ((int)strlen(buf) != ::write(fd, buf, strlen(buf)))
        {
            log::error("write peroid_ns = %s failed\r\n", buf);
            close(fd);
            return err::ERR_IO;
        }
        fsync(fd);
        close(fd);
        return err::ERR_NONE;
    }

    static err::Err _pwm_set_duty_cycle(int chip_id, int offset, int duty_cycle)
    {
        int fd;
        char buf[100];
        snprintf(buf, sizeof(buf), DUTYCYCLE_PATH, chip_id, offset);
        fd = ::open(buf, O_RDWR);
        if (fd < 0)
        {
            log::error("open %s failed\r\n", buf);
            return err::ERR_IO;
        }
        snprintf(buf, sizeof(buf), "%d", duty_cycle);
        if ((int)strlen(buf) != ::write(fd, buf, strlen(buf)))
        {
            log::error("write duty_cycle = %s failed\r\n", buf);
            close(fd);
            return err::ERR_IO;
        }
        fsync(fd);
        close(fd);
        return err::ERR_NONE;
    }

    static err::Err _pwm_set_enable(int chip_id, int offset, bool en)
    {
        int fd;
        char buf[100];
        snprintf(buf, sizeof(buf), ENABLE_PATH, chip_id, offset);
        fd = ::open(buf, O_RDWR);
        if (fd < 0)
        {
            log::error("open %s failed\r\n", buf);
            return err::ERR_IO;
        }
        snprintf(buf, sizeof(buf), "%d", en ? 1 : 0);
        if ((int)strlen(buf) != ::write(fd, buf, strlen(buf)))
        {
            log::error("write enable = %s failed\r\n", buf);
            close(fd);
            return err::ERR_IO;
        }
        fsync(fd);
        close(fd);
        return err::ERR_NONE;
    }

    //修改过的代码)
    PWM::PWM(int id, int freq, double duty, bool enable, int duty_val)
        : _duty_fd(-1), _enable_fd(-1)
    {
        _pwm_id = id;
        _freq = freq;
        _enable = enable;

        if (duty < 0 && duty_val < 0) {
            throw err::Exception(err::Err::ERR_ARGS, "one of duty and duty_val must be set");
        }
        if (freq <= 0) {
            throw err::Exception(err::Err::ERR_ARGS, "freq must be > 0");
        }

        _period_ns = freq_to_period_ns(freq);

        if (duty_val >= 0) {
            _duty_val = duty_val;
            _duty = duty_val * 100 / _period_ns;
        } else {
            _duty = duty;
            _duty_val = _period_ns * duty / 100;
        }

        // 获取chip_id和offset
        if (0 != _pwm_id_to_chipid(_pwm_id, _chip_id, _id_offset)) {
            throw err::Exception(err::Err::ERR_ARGS, "pwm_id is not support");
        }

        // 导出PWM
        if (_pwm_export(_chip_id, _id_offset) != err::ERR_NONE) {
            throw err::Exception(err::Err::ERR_IO, "export pwm failed");
        }

        // 预生成路径
        snprintf(_duty_path, sizeof(_duty_path), DUTYCYCLE_PATH, _chip_id, _id_offset);
        snprintf(_enable_path, sizeof(_enable_path), ENABLE_PATH, _chip_id, _id_offset);

        // 打开并保持文件描述符
        _duty_fd = ::open(_duty_path, O_RDWR);
        if (_duty_fd < 0) {
            throw err::Exception(err::Err::ERR_IO, "open duty_cycle failed");
        }

        _enable_fd = ::open(_enable_path, O_RDWR);
        if (_enable_fd < 0) {
            ::close(_duty_fd);
            throw err::Exception(err::Err::ERR_IO, "open enable failed");
        }

        // 设置周期和初始占空比
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", _period_ns);
        int fd = ::open(PERIOD_PATH, _chip_id, _id_offset), O_WRONLY);
        if (fd >= 0) {
            ::write(fd, buf, strlen(buf));
            ::close(fd);
        }

        snprintf(buf, sizeof(buf), "%d", _duty_val);
        if (::write(_duty_fd, buf, strlen(buf)) != (int)strlen(buf)) {
            ::close(_duty_fd);
            ::close(_enable_fd);
            throw err::Exception(err::Err::ERR_IO, "set initial duty failed");
        }

        // 设置使能状态
        snprintf(buf, sizeof(buf), "%d", _enable ? 1 : 0);
        if (::write(_enable_fd, buf, strlen(buf)) != (int)strlen(buf)) {
            ::close(_duty_fd);
            ::close(_enable_fd);
            throw err::Exception(err::Err::ERR_IO, "set enable failed");
        }
    }

    // 修改过的代码
    PWM::~PWM()
    {
        disable();
        if (_duty_fd >= 0) ::close(_duty_fd);
        if (_enable_fd >= 0) ::close(_enable_fd);
        _pwm_unexport(_chip_id, _id_offset);
    }

    // 修改过的代码
    double PWM::duty(double duty)
    {
        if (duty < 0) return _duty;
        if (duty > 100) duty = 100;
        
        _duty = duty;
        _duty_val = _period_ns * duty / 100;
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", _duty_val);
        if (::write(_duty_fd, buf, strlen(buf)) != (int)strlen(buf)) {
            log::error("set pwm duty_cycle failed");
            return (int)-err::ERR_IO;
        }
        fsync(_duty_fd);
        return duty;
    }

    // 修改过的代码
    int PWM::duty_val(int duty_val)
    {
        if (duty_val < 0) return _duty_val;
        
        _duty_val = duty_val;
        _duty = duty_val * 100 / _period_ns;
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", duty_val);
        if (::write(_duty_fd, buf, strlen(buf)) != (int)strlen(buf)) {
            log::error("set pwm duty_cycle failed");
            return (int)-err::ERR_IO;
        }
        fsync(_duty_fd);
        return duty_val;
    }

    int PWM::freq(int freq)
    {
        if (freq < 0)
        {
            return _freq;
        }
        _freq = freq;
        _period_ns = freq_to_period_ns(freq);
        err::Err ret = _pwm_set_period(_chip_id, _id_offset, _period_ns);
        if (ret != err::ERR_NONE)
        {
            log::error("set pwm period failed");
            return (int)-ret;
        }
        return freq;
    }

    err::Err PWM::enable()
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", 1);
        // === 修改部分 ===
        if (::write(_enable_fd, buf, strlen(buf)) != (int)strlen(buf)) {
        // === 修改部分 ===
            return err::ERR_IO;
        }
        _enable = true;
        return err::ERR_NONE;
    }

    err::Err PWM::disable()
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", 0);
        // === 修改部分 ===
        if (::write(_enable_fd, buf, strlen(buf)) != (int)strlen(buf)) {
        // === 修改部分 ===
            return err::ERR_IO;
        }
        _enable = false;
        return err::ERR_NONE;
    }

    bool PWM::is_enabled()
    {
        // === 修改部分 ===
        char buf[2] = {0};
        if (::pread(_enable_fd, buf, 1, 0) != 1) {
        // === 修改部分 ===
            log::error("read enable status failed");
            return false;
        }
        return buf[0] == '1';
    }

    err::Err PWM::update_duties(const std::vector<std::pair<PWM*, float>>& duties) {
        for (const auto& [pwm, duty] : duties) {
            if (!pwm) {
                log::error("Invalid PWM pointer in update_duties");
                return err::ERR_ARGS;
            }
            
            // 直接写入duty_cycle文件以提高效率
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(pwm->_period_ns * duty / 100.0f));
            
            if (pwm->_duty_fd < 0) {
                pwm->_duty_fd = ::open(pwm->_duty_path, O_WRONLY);
                if (pwm->_duty_fd < 0) {
                    log::error("open %s failed", pwm->_duty_path);
                    return err::ERR_IO;
                }
            }
            
            lseek(pwm->_duty_fd, 0, SEEK_SET);
            if ((int)strlen(buf) != ::write(pwm->_duty_fd, buf, strlen(buf))) {
                log::error("write duty_cycle = %s failed", buf);
                return err::ERR_IO;
            }
            fsync(pwm->_duty_fd);
            
            // 更新内部状态
            pwm->_duty = duty;
            pwm->_duty_val = pwm->_period_ns * duty / 100;
        }
        return err::ERR_NONE;
    }

}; // namespace maix

