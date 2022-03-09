#include <sstream>
#include <functional>

#include <string.h>

#include "src/fd/Stdio.hpp"
#include "src/fd/NamedPipe.h"
#include "src/mux/FileListener.h"
#include "src/config/ConfigReader.h"

// 协议
#include "src/app/client/model/chat_models.h"

// 数据
#include "src/app/client/controller/global.h"

// 注册管道用于写
class RegPipe : public WriteOnlyFIFO<Protocal::Reg::RegRecv>
{
public:
    RegPipe();
};

// 登录管道用于写
class LoginPipe : public WriteOnlyFIFO<Protocal::Login::LoginRecv>
{
public:
    LoginPipe();
};

// 发送消息管道用于写
class MsgPipe : public WriteOnlyFIFO<Protocal::Msg::MsgRecv>
{
public:
    MsgPipe();
};

// 下线管道用于写
class LogoutPipe : public WriteOnlyFIFO<Protocal::Logout::LogoutRecv>
{
public:
    LogoutPipe();
};

// 用户管道用于读，这个没办法指定协议了
class UserRecvPipe : public ReadOnlyFIFO<int>
{
public:
    UserRecvPipe(std::string username);

    // 重新定义回调
    void recv_callback();
};

// 标准输入用于读
class UserInput : public Stdin
{
private:
    void print_help();

public:
    // 重新定义回调，用于处理用户输入
    void recv_callback();
    void eof_callback(int err);
};