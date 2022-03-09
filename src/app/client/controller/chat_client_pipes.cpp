#include "chat_client_pipes.h"

using namespace std;

// 注册管道用于写
RegPipe::RegPipe() : WriteOnlyFIFO<Protocal::Reg::RegRecv>(config::get("reg_fifo_path")) {}

// 登录管道用于写
LoginPipe::LoginPipe() : WriteOnlyFIFO<Protocal::Login::LoginRecv>(config::get("login_fifo_path")) {}

// 发送消息管道用于写
MsgPipe::MsgPipe() : WriteOnlyFIFO<Protocal::Msg::MsgRecv>(config::get("msg_fifo_path")) {}

// 下线管道用于写
LogoutPipe::LogoutPipe() : WriteOnlyFIFO<Protocal::Logout::LogoutRecv>(config::get("logout_fifo_path")) {}

UserRecvPipe::UserRecvPipe(string username)
    : ReadOnlyFIFO<int>(config::get("user_fifo_path") + "/" + username) {}

// 重新定义回调，显示接收到的消息
void UserRecvPipe::recv_callback()
{
    // 先读前面部分，得知协议类型
    Protocal::ProtocalType type;
    if (!readfile(&type, sizeof(Protocal::ProtocalType)))
        return;

    // 再读后面枚举值，根据model中的设置显示响应内容
    switch (type)
    {
    case Protocal::ProtocalType::LoginRet:
    {
        Protocal::Login::LoginStatus status;
        // 读到EOF时返回
        if (!readfile(&status, sizeof(Protocal::Login::LoginStatus)))
            return;
        UserLog::log("login", Protocal::Login::get_string_by_status(status));
        if (status == Protocal::Login::login_success)
            global::chat_client_data().is_online_ = true;
        break;
    }
    case Protocal::ProtocalType::LogoutRet:
    {
        Protocal::Logout::LogoutStatus status;
        if (!readfile(&status, sizeof(Protocal::Logout::LogoutStatus)))
            return;
        UserLog::log("logout", Protocal::Logout::get_string_by_status(status));
        break;
    }
    case Protocal::ProtocalType::MsgRet:
    {
        Protocal::Msg::MsgStatus status;
        if (!readfile(&status, sizeof(Protocal::Msg::MsgStatus)))
            return;
        UserLog::log("message", Protocal::Msg::get_string_by_status(status));
        break;
    }
    case Protocal::ProtocalType::RegRet:
    {
        Protocal::Reg::RegStatus status;
        if (!readfile(&status, sizeof(Protocal::Reg::RegStatus)))
            return;
        UserLog::log("register", Protocal::Reg::get_string_by_status(status));
        break;
    }
    // 其他用户发消息来
    case Protocal::ProtocalType::MsgRecv:
    {
        char from[64], to[64], msg[64];
        if (!readfile(from, 64) || !readfile(to, 64) || !readfile(msg, 64))
            return;
        UserLog::log("message", "a message from " + string(from) + " to " + string(to) + " :" + string(msg));
        break;
    }
    default:
        UtilError::error_exit("invalid protocal type " + to_string(type), false);
        break;
    }
}

// 帮助信息
void UserInput::print_help()
{
    cout << "Usage: \n"
         << "register [password]\n"
         << "login [password]\n"
         << "send [user] [message]\n"
         << "logout\n"
         << endl;
}

// 重新定义回调，用于处理用户输入
void UserInput::recv_callback()
{
    // 读一行
    string line = readline();
    stringstream ss(line);

    // 读取参数
    string type;
    ss >> type;

    if (type == "login")
    {
        string password;
        ss >> password;

        if (password.empty())
        {
            UserLog::log("login", "login password can not be empty");
            print_help();
            return;
        }
        else if (password.size() > 64)
        {
            UserLog::log("login", "login password can not be longger than 64");
            print_help();
            return;
        }

        // 构造消息
        Protocal::Login::LoginRecv msg;
        strcpy(msg.username, global::chat_client_data().get_username().c_str());
        strcpy(msg.password, password.c_str());

        // 发送
        LoginPipe pipe;
        pipe.openfile();
        pipe.send_msg(msg);
        pipe.closefile();
    }
    else if (type == "register")
    {
        string password;
        ss >> password;

        if (password.empty())
        {
            UserLog::log("register", "register can not be empty");
            print_help();
            return;
        }
        else if (password.size() > 64)
        {
            UserLog::log("register", "register password can not be longger than 64");
            print_help();
            return;
        }

        // 构造消息
        Protocal::Reg::RegRecv msg;
        strcpy(msg.username, global::chat_client_data().get_username().c_str());
        strcpy(msg.password, password.c_str());

        // 发送
        RegPipe pipe;
        pipe.openfile();
        pipe.send_msg(msg);
        pipe.closefile();
    }
    else if (type == "logout")
    {
        // 构造消息
        Protocal::Logout::LogoutRecv msg;
        strcpy(msg.username, global::chat_client_data().get_username().c_str());

        // 发送
        LogoutPipe pipe;
        pipe.openfile();
        pipe.send_msg(msg);
        pipe.closefile();
    }
    else if (type == "send")
    {
        string to_user, chat_message;
        ss >> to_user >> chat_message;

        if (to_user.empty() || chat_message.empty())
        {
            UserLog::log("send", "username or message can not be empty");
            print_help();
            return;
        }
        else if (to_user.size() > 64 || chat_message.size() > 64)
        {
            UserLog::log("send", "chat message or username to long");
            print_help();
            return;
        }

        // 构造消息
        Protocal::Msg::MsgRecv msg;
        strcpy(msg.from, global::chat_client_data().get_username().c_str());
        strcpy(msg.to, to_user.c_str());
        strcpy(msg.msg, chat_message.c_str());

        // 发送
        MsgPipe pipe;
        pipe.openfile();
        pipe.send_msg(msg);
        pipe.closefile();
    }
    else
    {
        // 使用错误，提示正确用法
        if (!global::chat_client_data().get_username().empty())
            print_help();
    }
}

void UserInput::eof_callback(int err) {}