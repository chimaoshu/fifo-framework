#ifndef __FILE_LISTENER_H__
#define __FILE_LISTENER_H__

#include <map>
#include <memory>

#include "src/ThreadPool/ThreadPool.hpp"
#include "src/config/ConfigReader.h"
#include "src/fd/FileDescriptor.h"
#include "src/fd/NamedPipe.h"

// 文件集合进行监听
class FilesListener
{
protected:
    // fd to FileDescriptor
    std::map<int, std::shared_ptr<FileDescriptor>> files_;
    // 使用线程池处理内容
    bool use_thread_pool_;
    ThreadPool *pool_ = NULL;

public:
    FilesListener(bool use_thread_pool)
        : use_thread_pool_(use_thread_pool)
    {
        if (use_thread_pool_)
        {
            int thread_num = atoi(config::get("thread_num").c_str());
            pool_ = new ThreadPool(thread_num);
        }
    }

    ~FilesListener()
    {
        if (use_thread_pool_)
            delete pool_;
    }

    // 添加要监听的文件描述符
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
        return true;
    }

    // 删除要监听的文件描述符
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
        return true;
    }

    // 监听
    virtual void listen() = 0;
};

#endif