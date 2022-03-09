#include "chat_server_pipes.h"

using namespace std;

RegPipe::RegPipe() : ReadOnlyFIFO(config::get("reg_fifo_path"))
{
    // 回调函数
    auto handler = [](Protocal::Reg::RegRecv reg_recv) -> bool
    {
        Protocal::Reg::RegRet reg_ret;
        string username(reg_recv.username);
        string password(reg_recv.password);

        // 空用户名
        if (username.empty())
            reg_ret.status = Protocal::Reg::empty_username;
        // 空密码
        else if (password.empty())
            reg_ret.status = Protocal::Reg::empty_password;
        // 添加用户成功
        else if (global::chat_server_data().add_register_user(reg_recv.username, reg_recv.password))
            reg_ret.status = Protocal::Reg::register_success;
        // 已经注册过了
        else
            reg_ret.status = Protocal::Reg::username_has_been_registered;

        // 返回内容给用户
        WriteOnlyFIFO<Protocal::Reg::RegRet> user_fifo(config::get("user_fifo_path") + username);
        user_fifo.openfile();
        user_fifo.send_msg(reg_ret);
        user_fifo.closefile();

        return true;
    };

    // 设置回调函数为上述函数
    this->set_process_func(handler);
}

LoginPipe::LoginPipe() : ReadOnlyFIFO(config::get("login_fifo_path"))
{
    // 回调函数
    auto handler = [](Protocal::Login::LoginRecv login_recv) -> bool
    {
        Protocal::Login::LoginRet login_ret;
        string username(login_recv.username);
        string password(login_recv.password);

        // 用户名未注册
        if (!global::chat_server_data().user_is_registered(username))
            login_ret.status = Protocal::Login::username_not_registered;
        // 密码错误
        else if (!global::chat_server_data().add_online_user(username, password))
            login_ret.status = Protocal::Login::error_password;
        // 达到最大用户上限
        else if (global::chat_server_data().user_num_reached_the_max_limit())
            login_ret.status = Protocal::Login::max_online_user;
        // 成功
        else
            login_ret.status = Protocal::Login::login_success;

        // 返回内容给用户
        WriteOnlyFIFO<Protocal::Login::LoginRet> user_fifo(config::get("user_fifo_path") + username);
        user_fifo.openfile();
        user_fifo.send_msg(login_ret);
        user_fifo.closefile();

        return true;
    };

    // 设置回调函数为上述函数
    this->set_process_func(handler);
}

MsgPipe::MsgPipe() : ReadOnlyFIFO(config::get("msg_fifo_path"))
{
    // 回调函数
    auto handler = [](Protocal::Msg::MsgRecv msg_recv) -> bool
    {
        Protocal::Msg::MsgRet msg_ret;

        string from(msg_recv.from);
        string to(msg_recv.to);
        string chat_msg(msg_recv.msg);

        // 发送者未注册
        if (!global::chat_server_data().user_is_registered(from))
            msg_ret.status = Protocal::Msg::you_not_registered;
        // 发送者未登录
        else if (!global::chat_server_data().user_is_online(from))
            msg_ret.status = Protocal::Msg::you_not_online;
        // 接收者未注册
        if (!global::chat_server_data().user_is_registered(to))
            msg_ret.status = Protocal::Msg::user_not_exist;
        // 接收者未登录
        else if (!global::chat_server_data().user_is_online(to))
        {
            msg_ret.status = Protocal::Msg::user_not_online;
            // 缓存消息
            global::chat_server_data().add_msg(msg_recv);
        }
        // 成功
        else
        {
            msg_ret.status = Protocal::Msg::forward_success;

            // 转发消息给to
            WriteOnlyFIFO<Protocal::Msg::MsgRecv> to_fifo(config::get("user_fifo_path") + to);
            to_fifo.openfile();
            to_fifo.send_msg(msg_recv);
            to_fifo.closefile();
        }

        // 返回内容给from
        WriteOnlyFIFO<Protocal::Msg::MsgRet> from_fifo(config::get("user_fifo_path") + from);
        from_fifo.openfile();
        from_fifo.send_msg(msg_ret);
        from_fifo.closefile();

        return true;
    };

    // 设置回调函数为上述函数
    this->set_process_func(handler);
}

LogoutPipe::LogoutPipe() : ReadOnlyFIFO(config::get("logout_fifo_path"))
{
    // 回调函数
    auto handler = [](Protocal::Logout::LogoutRecv logout_recv) -> bool
    {
        Protocal::Logout::LogoutRet logout_ret;
        string username(logout_recv.username);

        // 用户未注册
        if (!global::chat_server_data().user_is_registered(username))
            logout_ret.status = Protocal::Logout::username_not_registered;
        // 用户不在线
        if (!global::chat_server_data().remove_online_user(username))
            logout_ret.status = Protocal::Logout::username_not_online;
        // 成功
        else
            logout_ret.status = Protocal::Logout::logout_success;

        // 返回内容给from
        WriteOnlyFIFO<Protocal::Logout::LogoutRet> user_fifo(config::get("user_fifo_path") + username);
        user_fifo.openfile();
        user_fifo.send_msg(logout_ret);
        user_fifo.closefile();

        return true;
    };

    // 设置回调函数为上述函数
    this->set_process_func(handler);
}