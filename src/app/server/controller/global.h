#ifndef __SERVER_GLOBAL_H__
#define __SERVER_GLOBAL_H__

#include <map>
#include <string>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "src/app/server/model/chat_models.h"

// 全局变量文件，用户自己编写
using namespace std;

// 属于DAO层
class ChatServerData
{
private:
    // 已经注册的用户
    map<string, string> username_password;
    mutex username_password_mutex_;

    // 在线用户
    unordered_set<string> online_user;
    mutex online_user_mutex_;

    // 发送给离线用户的消息
    queue<Protocal::Msg::MsgRecv> msg_to_offline_user;
    mutex msg_to_offline_user_mutex_;

public:
    bool user_is_registered(string user)
    {
        unique_lock<std::mutex> lock(username_password_mutex_);
        return username_password.find(user) != username_password.end();
    }

    bool add_register_user(string user, string password)
    {
        unique_lock<std::mutex> lock(username_password_mutex_);
        // 用户已存在
        if (username_password.find(user) != username_password.end())
            return false;

        username_password[user] = password;
        return true;
    }

    bool user_is_online(string user)
    {
        unique_lock<std::mutex> lock(online_user_mutex_);
        return online_user.find(user) != online_user.end();
    }

    bool verify_password(string user, string password)
    {
        unique_lock<std::mutex> lock(username_password_mutex_);
        auto it = username_password.find(user);
        if (it == username_password.end())
            return false;

        return it->second == password;
    }

    // 用户数量达到上限
    bool user_num_reached_the_max_limit()
    {
        int max_online_user = atoi(config::get("max_online_user").c_str());
        return online_user.size() == max_online_user;
    }

    bool add_online_user(string user, string password)
    {
        unique_lock<std::mutex> lock(online_user_mutex_);

        // 用户没注册
        auto it = username_password.find(user);
        if (it == username_password.end())
            return false;

        // 密码正确
        if (it->second == password)
        {
            if (online_user.find(user) == online_user.end())
                online_user.insert(user);
            return true;
        }

        // 密码错误
        return false;
    }

    bool remove_online_user(string user)
    {
        unique_lock<std::mutex> lock(online_user_mutex_);
        auto it = online_user.find(user);
        if (it != online_user.end())
        {
            online_user.erase(it);
            return true;
        }

        // 用户不在线
        return false;
    }

    // 缓存那些发送给离线用户的消息
    void add_msg(Protocal::Msg::MsgRecv msg)
    {
        unique_lock<std::mutex> lock(msg_to_offline_user_mutex_);
        msg_to_offline_user.push(msg);
    }

    // 成功返回true，内容赋值在msg中
    bool pop_msg(Protocal::Msg::MsgRecv &msg)
    {
        unique_lock<std::mutex> lock(msg_to_offline_user_mutex_);

        // 空
        if (msg_to_offline_user.empty())
            return false;

        msg = msg_to_offline_user.front();
        return true;
    }
};

// 把上述DAO变成单例全局变量
class global
{
public:
    static ChatServerData &chat_server_data()
    {
        static ChatServerData dao;
        return dao;
    }
};

#endif // __SERVER_GLOBAL_H__