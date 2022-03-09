#ifndef __SELECTOR_H__
#define __SELECTOR_H__

#include <map>
#include <memory>

#include "src/ThreadPool/ThreadPool.hpp"
#include "src/config/ConfigReader.h"
#include "src/fd/FileDescriptor.h"
#include "src/fd/NamedPipe.h"

// 文件集合进行监听
class FilesListener
{
private:
    // fd to FileDescriptor
    std::map<int, std::shared_ptr<FileDescriptor>> files_;
    // select用
    fd_set read_fd_set_;
    // 使用线程池处理内容
    bool use_thread_pool_;
    ThreadPool *pool_ = NULL;

public:
    FilesListener(bool use_thread_pool)
        : use_thread_pool_(use_thread_pool)
    {
        FD_ZERO(&read_fd_set_);
        if (use_thread_pool_)
        {
            int thread_num = atoi(config::get("thread_num").c_str());
            pool_ = new ThreadPool(thread_num);
        }
    }

    ~FilesListener() { delete pool_; }

    // 添加要监听的管道
    bool add_fd(std::shared_ptr<FileDescriptor> file)
    {
        // 未打开则打开
        file->openfile();
        int fd = file->get_fd();

        // 已存在
        if (files_.find(fd) != files_.end())
        {
            Log::warn("file with fd " + std::to_string(fd) + " has already been added to selector");
            return false;
        }

        files_[fd] = file;
        FD_SET(fd, &read_fd_set_);
        return true;
    }

    // 删除要监听的管道
    bool remove_fd(std::shared_ptr<FileDescriptor> file)
    {
        int fd = file->get_fd();

        // 不存在
        auto it = files_.find(fd);
        if (it == files_.end())
        {
            Log::warn("file with fd " + std::to_string(fd) + " not exits in selector");
            return false;
        }

        // 删除
        files_.erase(it);
        FD_CLR(fd, &read_fd_set_);
        return true;
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

    // TODO
    void listen_poll() {}
    void listen_epoll() {}
};

#endif