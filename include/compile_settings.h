#pragma once

#define MQ_HOST "192.168.31.6"
#define MQ_PORT 5672
#define MQ_USER "qingniao"
#define MQ_PASSWORD "123456"
#define FILE_ROOT_PATH "/tmp/judge/"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

enum class TaskStatus
{
    COMPILING = 2,
    EXECUTING = 1,
    AC = 0,
    WA = -1,
    UKE = -2,
    RE = -3,
    CE = -4,
    TLE = -5,
    MLE = -6,
    OLE = -7
};

// 自定义编译异常
class compile_error : public std::exception
{
private:
    std::string msg;

public:
    compile_error(const std::string &msg) : msg(msg) {}
    const char *what() const noexcept override
    {
        return msg.c_str();
    }
};

// 生成当前时间的格式化字符串
std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    time_t now_time = std::chrono::system_clock::to_time_t(now);
    tm tm_struct;
    localtime_r(&now_time, &tm_struct); // 使用线程安全的localtime_r

    std::stringstream ss;
    ss << "[" << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S") << "]:";
    return ss.str();
}