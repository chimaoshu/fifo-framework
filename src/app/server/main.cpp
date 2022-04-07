#include "src/mux/FilesListenerEpoll.h"
#include "src/mux/FilesListenerSelect.h"
#include "src/app/server/controller/chat_server_pipes.h"

int main()
{
    // UtilSystem::init_daemon();

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
    // FilesListenerSelect listener(use_thread_pool);
    FilesListenerEpoll listener(use_thread_pool);
    listener.add_fd(reg_pipe);
    listener.add_fd(login_pipe);
    listener.add_fd(msg_pipe);
    listener.add_fd(logout_pipe);

    // 开始服务器
    listener.listen_select();
}