#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "src/log/Log.hpp"
#include "src/config/ConfigReader.h"

class UserLog
{
private:
    static Logger &get_logger()
    {
        static Logger logger(
            config::get("user_log_dir"),
            std::vector<std::string>{"register", "message", "login", "logout"},
            true);
        return logger;
    }

public:
    static void log(const std::string logname, const std::string &msg)
    {
        get_logger().log(logname, msg);
    }
};

class ChatClientData
{
private:
    std::string username_;
public:
    void set_username(std::string username) { username_ = username; }
    std::string get_username() { return username_; }
    bool is_online_ = false;
};

// 把上述DAO变成单例全局变量
class global
{
public:
    static ChatClientData &chat_client_data()
    {
        static ChatClientData dao;
        return dao;
    }
};

#endif // __GLOBAL_H__