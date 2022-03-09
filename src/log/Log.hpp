#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <cassert>

#include <string.h>

#include "src/config/ConfigReader.h"

#include "src/utils/util.hpp"

using LogName = std::string;

class Logger
{
private:
    // 目录
    std::string log_dir_;
    // 文件列表
    std::vector<std::ofstream> file_list_;
    // 日志名列表
    std::vector<LogName> logname_list_;
    // 从日志名到列表下标的映射
    std::unordered_map<std::string, int> logname_index_map_;
    // 是否格式化日志
    bool format_;
    // 锁
    std::mutex mutex_;
    // 缓存
    std::vector<std::string> tmp_;
    // 缓存超过阈值时写入文件
    const size_t MAX_TMP_SIZE_ = 0;

    // 输出
    static void to_stdout(const std::string &s)
    {
        std::cout << s << std::flush;
    }

    // 获取当前时间，字符串形式
    static std::string get_time()
    {
        time_t now = time(0);
        char *s = ctime(&now);

        // 去除换行符PP
        int len = strlen(s);
        assert(len > 1);
        s[len - 1] = '\0';

        std::string result(s);
        return result;
    }

    // 格式化为日志格式
    static std::string format_str(const std::string &&type, const std::string &msg)
    {
        return "[" + type + "][" + get_time() + "]" + msg;
    }
    static std::string format_str(const std::string &type, const std::string &msg)
    {
        return "[" + type + "][" + get_time() + "]" + msg;
    }

public:
    Logger(const std::string &log_dir, const std::vector<LogName> &logname_list, bool format)
        : log_dir_(log_dir), logname_list_(logname_list), format_(format)
    {
        // log_dir检查是否存在
        if (!UtilFile::dir_exists(log_dir_))
        {
            // 创建目录
            bool success = UtilFile::dir_create(log_dir_);
            if (!success)
            {
                std::string msg = format_str("error", "create log dir failed, exit.");
                UtilError::error_exit(msg, true);
            }
        }

        // 空列表
        if (logname_list_.empty())
        {
            std::string msg = format_str("warn", "logname list is empty.");
            to_stdout(msg);
        }

        // 创建文件并打开
        file_list_.resize(logname_list_.size());
        for (int i = 0; i < logname_list_.size(); i++)
        {
            // 空字符串
            std::string &logname = logname_list_[i];
            if (logname.empty())
            {
                std::string msg = format_str("error", "a logname is empty.");
                UtilError::error_exit(msg, false);
            }

            // 创建并打开文件，已存在则直接打开
            std::string filename = log_dir_ + '/' + logname + ".log";
            file_list_[i].open(filename, std::fstream::app);

            // 日志名已经存在
            if (logname_index_map_.find(logname) != logname_index_map_.end())
            {
                std::string msg = format_str("error", "duplicate log name");
                UtilError::error_exit(msg, false);
            }

            // 从日志名到下标的映射
            logname_index_map_.emplace(logname, i);
        }

        // 初始化缓存
        tmp_.resize(logname_list_.size());
        for (int i = 0; i < logname_list_.size(); i++)
            tmp_[i].reserve(MAX_TMP_SIZE_);
    }

    ~Logger()
    {
        // 将所有缓存写入文件，并关闭文件
        std::unique_lock<std::mutex> lock(mutex_);
        for (int i = 0; i < file_list_.size(); i++)
        {
            file_list_[i] << tmp_[i];
            file_list_[i].close();
        }
    }

    // 写日志
    void log(const std::string &logname, const std::string &msg)
    {
        // 上锁
        std::unique_lock<std::mutex> lock(mutex_);

        // 日志名不存在
        if (logname_index_map_.find(logname) == logname_index_map_.end())
        {
            std::string err_msg = format_str("error", "attempting to log in a undefined logname");
            UtilError::error_exit(err_msg, false);
        }

        // 日志消息是否格式化
        std::string log_msg = format_ ? format_str(logname, msg) : msg;

        // 添加换行符
        if (log_msg.back() != '\n')
            log_msg += '\n';

#ifdef DEBUG
        // 标准输出
        to_stdout(log_msg);
#else
        if (logname != "debug")
            to_stdout(log_msg);
#endif

        // 下标
        int index = logname_index_map_[logname];

        // 写入缓存
        tmp_[index] += log_msg;

        // 缓存大于阈值，则写入文件
        if (tmp_[index].size() >= MAX_TMP_SIZE_)
        {
            // 写入文件
            file_list_[index] << log_msg;
            // 清空缓存
            tmp_[index].clear();
        }
    }
};

// 全局静态变量写日志
class Log
{
private:
    // meyers singleton mode
    // 局部静态变量只会在第一次被调用时实例化第一次，以后不会再实例化
    // 从而实现单例模式且可以充当全局变量，且保证调用时是已经被初始化的状态
    static Logger &get_logger()
    {
        // 运行时期的日志
        static Logger runtime_logger(config::get("log_dir"), std::vector<LogName>{"debug", "info", "warn", "error"}, true);
        return runtime_logger;
    }

public:
    static void debug(const std::string &msg)
    {
        get_logger().log("debug", msg);
    }

    static void info(const std::string &msg)
    {
        get_logger().log("info", msg);
    }

    static void warn(const std::string &msg)
    {
        get_logger().log("warn", msg);
    }

    static void error(const std::string &msg)
    {
        get_logger().log("error", msg);
    }
};

#endif // __LOG_HPP__