#ifndef __CONFIG_READER_H__
#define __CONFIG_READER_H__

#include <map>
#include <string>
#include <sstream>
#include <fstream>

#include "src/utils/util.hpp"

class ConfigReader
{
private:
    std::map<std::string, std::string> config_;

    // 读取键值config，数据以空格隔开
    void read_kv_config(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            UtilError::error_exit("config file not exits", false);
        }

        // 读取行
        std::string line;
        int line_num = 1;
        while (std::getline(file, line))
        {
            // 读取
            std::istringstream ss(line);
            std::string key, value;
            ss >> key >> value;

            // 忽略注释和空行
            if (key.front() == '#' || key.empty())
                continue;

            // 有key没有value
            if (!key.empty() && value.empty())
                UtilError::error_exit("row " + std::to_string(line_num) + " has key but no value", false);

            // 重复键
            if (config_.find(key) != config_.end())
                UtilError::error_exit("duplicate key \"" + key + "\" in config", false);

            config_[key] = value;
            line_num++;
        }

        file.close();
    }

public:
    ConfigReader(std::string path) { read_kv_config(path); }
    std::string get(std::string key)
    {
        auto it = config_.find(key);
        if (it != config_.end())
            return it->second;
        else
        {
            UtilError::error_exit("config do not have key \"" + key + "\"", false);
            return "";
        }
    }
};

// 全局且单例的Config类，在global.h中生成，配置文件地址硬编码
class config
{
private:
    static ConfigReader &get_config()
    {
        static ConfigReader reader("./app.conf");
        return reader;
    }

public:
    static std::string get(std::string key)
    {
        return get_config().get(key);
    }
};

#endif // __CONFIG_READER_H__