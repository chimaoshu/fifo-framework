# fifo-framework

Linux下的多线程命名管道通信框架

使用方法：参考 `src/app/client` 与 `src/app/server` 下例子（实现了一个简单聊天室的服务端与客户端），在 model 中定义你的数据模型，在 controller 中继承命名管道，将处理函数定义之后赋值即可。

## 框架做的事

监听开发者定义好的controller，接收到消息时按照开发者定义的model进行读取，将model作为入参传递到开发者定义的处理函数中，将处理函数丢进线程池进行处理。

### 一些可用的功能

#### 定义不同的文件

所有文件类都有共同的API，开发者只需要关注`eof_callback`与`recv_callback`，有时需要自己重新定义接收消息时的回调函数。
```cpp
int get_fd(); // 获取文件描述符

int readfile(void *buf, size_t n);  // 返回读取字节数，0表示EOF
int writefile(void *buf, size_t n); // 返回写入字节数，0表示没写入
int closefile();                    // 关闭文件
int openfile() = 0;                 // 打开文件，不存在则创建，已打开则不操作
int createfile() = 0;               // 创建文件，存在则删除重新创建
int deletefile() = 0;               // 删除文件
void eof_callback(int err) = 0;     // EOF时回调
void recv_callback() = 0;           // 有输入时回调

void writeline(std::string &s);             // 写一行字符串，保证以换行符结尾
std::string readline();                     // 读一行字符串，保证以换行符结尾
```

##### 定义命名管道

定义一个用于读的命名管道（比如服务端用于监听请求的管道），模板参数为开发者定义的model，后面会提到
```cpp
class RegPipe : public ReadOnlyFIFO<Protocal::Reg::RegRecv>
{
public:
    RegPipe();
};
```

定义一个用于写的命名管道（比如客户端用于向服务端写的管道）
```cpp
class RegPipe : public WriteOnlyFIFO<Protocal::Reg::RegRecv>
{
public:
    RegPipe();
};
```

##### 自定义标准输入，为其加上回调
```cpp
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
```

##### 定义socket
// TODO

#### 文件监听

实现了基于select、epoll的监听，开发者可以将定义好的命名管道或者stdin添加到listner中，文件描述符可读时会自动调用回调函数。

实例化listner类时需要传入一个bool值：是否使用线程池进行处理，true则使用多线程处理请求，false则单线程处理请求。

listner类拥有共同的API
```cpp
bool add_fd(std::shared_ptr<FileDescriptor> file)       // 添加要监听的文件描述符
bool remove_fd(std::shared_ptr<FileDescriptor> file)    // 删除要监听的文件描述符
void listen() = 0;                                      // 开始监听
```

使用示例
```cpp
// 实例化创建管道
shared_ptr<FileDescriptor> reg_pipe((FileDescriptor *)new RegPipe();
reg_pipe->createfile();
// 实例化标准输入
shared_ptr<FileDescriptor> user_input_stdin((FileDescriptor *)new UserInput());
// 是否使用线程池
bool use_thread_pool = false;
FilesListenerEpoll listener(use_thread_pool); // 使用epoll监听
// or 
FilesListenerSelect listener(use_thread_pool);  // 使用select监听
listener.add_fd(user_recv_pipe);
listener.add_fd(user_input_stdin);
listener.listen();
```

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

### 服务端：定义服务端接收到消息时的回调函数（controller）

```cpp
// 继承一个只读的命名管道，使用 this->set_process_func() 设置回调函数
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

### 服务端main函数：将前面定义好的API（命名管道）添加到select或者epoll进行监听

```cpp
#include "src/mux/FilesListenerEpoll.h"
#include "src/mux/FilesListenerSelect.h"
#include "src/app/server/controller/chat_server_pipes.h"

int main()
{
    // 可将服务端变守护进程
    UtilSystem::init_daemon();

    // 注册用的API
    shared_ptr<FileDescriptor> reg_pipe = make_shared<RegPipe>();
    reg_pipe->createfile();

    // 登录用的API
    shared_ptr<FileDescriptor> login_pipe = make_shared<LoginPipe>();
    login_pipe->createfile();

    // 是否使用线程池（false则为单线程处理）
    bool use_thread_pool = false;
    
    // 添加到多路复用的监听集合中，这里可以使用select或者epoll
    // FilesListenerSelect listener(use_thread_pool);
    FilesListenerEpoll listener(use_thread_pool);
    listener.add_fd(reg_pipe);
    listener.add_fd(login_pipe);

    // 开始服务器
    listener.listen();
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
    // case xxxxx
    default:
        UtilError::error_exit("invalid protocal type " + to_string(type), false);
        break;
    }
}
```

### 客户端：main函数例子（这里定义了stdin和客户端命名管道，将两个文件添加到select或epoll中进行监听）

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
    listener.listen();
}
```