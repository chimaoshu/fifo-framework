#include <iostream>

#include "src/mux/FilesListenerSelect.h"
#include "src/mux/FilesListenerEpoll.h"
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
    // FilesListenerSelect listener(use_thread_pool);
    FilesListenerEpoll listener(use_thread_pool);
    listener.add_fd(user_recv_pipe);
    listener.add_fd(user_input_stdin);
    listener.listen_select();
}