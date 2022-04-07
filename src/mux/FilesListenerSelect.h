#ifndef __FILE_LISTENER_SELECT_H__
#define __FILE_LISTENER_SELECT_H__

#include <sys/select.h>

#include "src/mux/FilesListener.h"

// 文件集合进行监听
class FilesListenerSelect : public FilesListener
{
private:
    // select用
    fd_set read_fd_set_;

public:
    FilesListenerSelect(bool use_thread_pool)
        : FilesListener(use_thread_pool) { FD_ZERO(&read_fd_set_); }

    ~FilesListenerSelect()
    {
        if (use_thread_pool_)
            delete pool_;
    }

    // 添加要监听的管道
    bool add_fd(std::shared_ptr<FileDescriptor> file)
    {
        if (FilesListener::add_fd(file))
        {
            int fd = file->get_fd();
            FD_SET(fd, &read_fd_set_);
            return true;
        }
        return false;
    }

    // 删除要监听的管道
    bool remove_fd(std::shared_ptr<FileDescriptor> file)
    {
        if (FilesListener::remove_fd(file))
        {
            int fd = file->get_fd();
            FD_CLR(fd, &read_fd_set_);
            return true;
        }
        return false;
    }

    // 调用回调函数
    void listen_select()
    {
        while (true)
        {
            // 获取最大的fd
            int max_fd = files_.rbegin()->first;

            // 等待任意管道来消息
            int res;
            fd_set tmp = read_fd_set_;
            if ((res = select(max_fd + 1, &tmp, NULL, NULL, NULL)) != -1)
            {
                // 遍历查询哪个管道就绪
                bool has_fd = false;
                for (auto &&p : files_)
                {
                    int fd = p.first;

                    // 调用预先定义的回调函数
                    if (FD_ISSET(fd, &tmp))
                    {
                        auto file = p.second;

                        // 使用线程池处理
                        if (use_thread_pool_)
                            pool_->submit([&file]()
                                          { file->recv_callback(); });
                        // 同步阻塞处理
                        else
                            file->recv_callback();

                        has_fd = true;
                        break;
                    }
                }

                if (has_fd)
                    continue;

                UtilError::error_exit("select, but no fd is ready", false);
            }

#ifdef DEBUG
            // select失败
            UtilError::error_exit("select failed", true);
#else
            Log::warn("select failed");
#endif
        }
    }
};

#endif