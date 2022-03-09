# fifo-framework

Linux下的多线程命名管道通信框架

使用方法：参考 `src/app/client` 与 `src/app/server` 下例子（实现了一个简单聊天室的服务端与客户端），在 model 中定义你的数据模型，在 controller 中继承命名管道，将处理函数定义之后赋值即可。

## 框架做的事

监听开发者定义好的controller，接收到消息时按照开发者定义的model进行读取，将model作为入参传递到开发者定义的处理函数中，将处理函数丢进线程池进行处理。

### 一些可用的功能

#### 配置文件

在运行文件目录下创建 `./app.conf` 文件，每行表示一个kv值，用空格隔开，#为注释。

```conf
# 日志目录
log_dir /home/user/log_dir/
# 最大用户
max_online_user 5
```

配置文件kv的读取方法：框架实现了一个全局单例的静态类方法可以用于读取配置
```cpp
string value = config::get("reg_fifo_path");
// or
string key = "reg_fifo_path";
string value = config::get(key);
```

#### 日志

框架实现了一个全局单例的日志类，开发者需要在 `app.conf` 设置日志目录
```conf
log_dir /home/user/log_dir/
```

开发时可以直接调用
```cpp
Log::debug("xxxxx");
Log::info("xxxxx");
Log::warn("xxxxx");
Log::error("xxxxx");
```

另外开发者可以自己实现单例日志类，参考下面这个Log的实现（meyers singleton mode）：
```cpp
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
```

#### 让服务器变守护进程

```cpp
int main()
{
    UtilSystem::init_daemon();

    // xxxxx
}
```

## 开发者需要做的事

定义好 model 和 controller，将 controller 丢进 FileListener 进行监听。

### 定义 model

其中的 model 必须包括：服务端接收消息所用的结构体、服务端返回消息所用的结构体（同时也是客户端接收消息所用的结构体）

例子：
```cpp
// 协议，用户自己编写
namespace Protocal
{
    // 协议类型
    // 所有协议的结构体都以该字段开头，用于区分
    enum ProtocalType
    {
        RegRecv,
        RegRet,
        LoginRecv,
        LoginRet,
        MsgRecv,
        MsgRet,
        LogoutRecv,
        LogoutRet,
    };

    namespace Reg
    {
        // 服务端接收的消息
        struct RegRecv
        {
            ProtocalType protocal_type = ProtocalType::RegRecv;
            char username[64];
            char password[64];
        };

        enum RegStatus : int
        {
            register_success,
            username_has_been_registered,
            empty_username,
            empty_password
        };

        // 服务端返回的消息（同时也就是客户端接收的消息）
        struct RegRet
        {
            ProtocalType protocal_type = ProtocalType::RegRet;
            RegStatus status;
        };

        static std::string get_string_by_status(RegStatus status)
        {
            static std::map<RegStatus, std::string> m{
                {register_success, "register success"},
                {username_has_been_registered, "username has been registered"},
                {empty_username, "empty username"},
                {empty_password, "empty password"},
            };
            return m[status];
        }
    } // namespace Reg

    // ... 一些其他的API
    // namespace Login {}
}
```

### 服务端：定义服务端接收到消息时的回调函数

```cpp
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

        // 打开客户端的命名管道写入，返回内容给用户
        WriteOnlyFIFO<Protocal::Reg::RegRet> user_fifo(config::get("user_fifo_path") + username);
        user_fifo.openfile();
        user_fifo.send_msg(reg_ret);
        user_fifo.closefile();

        return true;
    };

    // 设置回调函数为上述函数
    this->set_process_func(handler);
}
```

### 服务端：main函数例子

```cpp
#include "src/app/server/controller/chat_server_pipes.h"
#include "src/mux/FileListener.h"

int main()
{
    UtilSystem::init_daemon();

    // 注册管道
    shared_ptr<FileDescriptor> reg_pipe = make_shared<RegPipe>();
    reg_pipe->createfile();

    // 登录管道
    shared_ptr<FileDescriptor> login_pipe = make_shared<LoginPipe>();
    login_pipe->createfile();

    // 发消息管道
    shared_ptr<FileDescriptor> msg_pipe = make_shared<MsgPipe>();
    msg_pipe->createfile();

    // 注销管道
    shared_ptr<FileDescriptor> logout_pipe = make_shared<LogoutPipe>();
    logout_pipe->createfile();

    // 添加到多路复用的监听集合中
    bool use_thread_pool = false;
    FilesListener listener(use_thread_pool);
    listener.add_fd(reg_pipe);
    listener.add_fd(login_pipe);
    listener.add_fd(msg_pipe);
    listener.add_fd(logout_pipe);

    // 开始服务器
    listener.listen_select();
}
```

### 客户端：客户端注册用于写服务端的命名管道

```cpp
// 注册管道用于写
RegPipe::RegPipe() : WriteOnlyFIFO<Protocal::Reg::RegRecv>(config::get("reg_fifo_path")) {}
```

### 客户端：定义客户端接收到服务端返回消息时的回调函数，需要根据model的首个字段（协议类型）进行不同的读取处理

例子：
```cpp
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
```

### 客户端：main函数例子

```cpp
#include <iostream>

#include "src/app/client/controller/chat_client_pipes.h"

using namespace std;

int main()
{
    cout << "please input username: " << endl;
    while (global::chat_client_data().get_username().empty())
    {
        string username;
        cin >> username;
        if (username.size() > 64)
        {
            UserLog::log("login", "username too long");
            continue;
        }
        global::chat_client_data().set_username(username);
    }

    // 创建管道
    shared_ptr<FileDescriptor> user_recv_pipe((FileDescriptor *)new UserRecvPipe(global::chat_client_data().get_username()));
    user_recv_pipe->createfile();

    // 标准输入
    shared_ptr<FileDescriptor> user_input_stdin((FileDescriptor *)new UserInput());

    bool use_thread_pool = false;
    FilesListener listener(use_thread_pool);
    listener.add_fd(user_recv_pipe);
    listener.add_fd(user_input_stdin);
    listener.listen_select();
}
```