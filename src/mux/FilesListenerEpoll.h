#ifndef __FILE_LISTENER_EPOLL_H__
#define __FILE_LISTENER_EPOLL_H__

#include <vector>
#include <sys/epoll.h>

#include "src/mux/FilesListener.h"

// 文件集合进行监听
class FilesListenerEpoll : public FilesListener
{
private:
    // epoll用
    int epoll_fd_;

public:
    FilesListenerEpoll(bool use_thread_pool)
        : FilesListener(use_thread_pool)
    {
        epoll_fd_ = epoll_create(256);
        if (epoll_fd_ == -1)
            UtilError::error_exit("epoll create error", true);
    }

    ~FilesListenerEpoll()
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
            // 增加管道到epoll
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
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
            // 2.6.9之前的Linux内核，ev必须非空，尽管DEL时会被忽略
            struct epoll_event ev;
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
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
            int nfds;
            struct epoll_event *events = new epoll_event[max_fd + 1];
            if ((nfds = epoll_wait(epoll_fd_, events, max_fd + 1, -1)) != -1)
            {
                // 遍历查询哪个管道就绪
                bool has_fd = false;
                for (int i = 0; i < nfds; i++)
                {
                    if (events[i].events & EPOLLIN)
                    {
                        int fd = events[i].data.fd;
                        auto file = files_[fd];

                        // 使用线程池处理
                        if (use_thread_pool_)
                            pool_->submit([&file]()
                                          { file->recv_callback(); });
                        // 同步阻塞处理
                        else
                            file->recv_callback();

                        has_fd = true;
                    }
                }
                if (has_fd)
                    continue;

                UtilError::error_exit("epoll, but no fd is ready", false);
            }

#ifdef DEBUG
            // epoll失败
            UtilError::error_exit("epoll failed", true);
#else
            Log::warn("epoll failed");
#endif
        }
    }
};

#endif