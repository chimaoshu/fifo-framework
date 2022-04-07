#include <functional>

#include "src/fd/NamedPipe.h"
#include "src/mux/FilesListener.h"
#include "src/config/ConfigReader.h"

// 协议
#include "src/app/server/model/chat_models.h"

// 数据
#include "src/app/server/controller/global.h"

// 注册管道
class RegPipe : public ReadOnlyFIFO<Protocal::Reg::RegRecv>
{
public:
    RegPipe();
};

// 登录管道
class LoginPipe : public ReadOnlyFIFO<Protocal::Login::LoginRecv>
{
public:
    LoginPipe();
};

// 发送消息管道
class MsgPipe : public ReadOnlyFIFO<Protocal::Msg::MsgRecv>
{
public:
    MsgPipe();
};

// 注销管道
class LogoutPipe : public ReadOnlyFIFO<Protocal::Logout::LogoutRecv>
{
public:
    LogoutPipe();
};