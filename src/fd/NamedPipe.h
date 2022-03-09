#ifndef __NAMED_PIPE_H__
#define __NAMED_PIPE_H__

#include <string>
#include <functional>
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "src/fd/FileWithPath.h"

// 命名管道
// 两个模板参数分别为读与写的协议
template <typename RecvMsgStruct, typename RetMsgStruct>
class NamedPipe : public FileWithPath
{
private:
    // 用户定义的逻辑处理函数
    // 传入为RecvMsgStruct
    // 函数内容为处理接收到的消息
    using ProcFuncType = std::function<bool(RecvMsgStruct)>;
    ProcFuncType process_func_;
    bool has_process_func_ = false;

public:
    // NamedPipe(const NamedPipe &) = delete;
    // NamedPipe &operator=(const NamedPipe &) = delete;
    NamedPipe(const std::string &path, FileOpenMode open_mode)
        : FileWithPath(path, open_mode) {}
    // 读到内容时运行回调函数
    int readfile(void *buf, size_t n)
    {
        int res = FileDescriptor::readfile(buf, n);
        std::string debug = std::to_string(res) + "/" + std::to_string(n) +
                            " bytes was read from: " + path_;
        Log::debug(debug);
        return res;
    }
    // 写管道
    int writefile(void *buf, size_t n)
    {
        int res = FileDescriptor::writefile(buf, n);
        // log
        std::string debug = std::to_string(res) + "/" + std::to_string(n) +
                            " bytes was sent to: " + path_;
        Log::debug(debug);
        return res;
    }
    // 创建命名管道，成功返回1
    int createfile()
    {
        // 上锁
        std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);

        // 文件存在则删除
        if (file_exits())
        {
            Log::warn("FIFO exists, delete and create again");
            deletefile();
        }

        if (mkfifo(path_.c_str(), 0777) != 0)
        {
            std::string err = "create FIFO " + path_ + " failed";
            UtilError::error_exit(err, true);
        }
        Log::debug("file " + path_ + " create success");
        return 1;
    }
    // EOF时重新打开文件
    void eof_callback(int err)
    {
        Log::debug("EOF err: " + std::string(strerror(err)));
        {
            std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);
            closefile();
            openfile();
        }
        Log::info("fd " + std::to_string(fd_) + " is invalid or closed, close file and reopen again");
    }
    // 接收协议，读端用
    bool recv_msg(RecvMsgStruct &msg) { return recv_struct<RecvMsgStruct>(msg); }
    // 发送协议，写端用
    bool send_msg(RetMsgStruct msg) { return send_struct<RetMsgStruct>(msg); }
    // 接收到内容时的处理函数，供读端使用
    void recv_callback()
    {
        // 未定义处理函数
        if (!has_process_func_)
            UtilError::error_exit("namedpipe: process function not defined", false);

        // 接收
        RecvMsgStruct received_msg;
        bool recv_sucess = recv_msg(received_msg);

        // 接收失败
        if (!recv_sucess)
            return;

        // 调用用户定义的处理函数
        bool proc_success = process_func_(received_msg);

        // 处理失败
        if (!proc_success)
        {
            Log::debug("process failed");
            return;
        }

        // 处理成功
        Log::debug("process success");
    }
    // 设置处理函数，读端用
    void set_process_func(ProcFuncType process_func)
    {
        process_func_ = process_func;
        has_process_func_ = true;
    }
};

// 只读命名管道
template <typename RecvMsgStruct>
class ReadOnlyFIFO : public NamedPipe<RecvMsgStruct, int>
{
public:
    ReadOnlyFIFO(const std::string &path)
        : NamedPipe<RecvMsgStruct, int>(path, FileOpenMode::ReadOnly) {}
};

// 只写命名管道
template <typename RetMsgStruct>
class WriteOnlyFIFO : public NamedPipe<int, RetMsgStruct>
{
public:
    WriteOnlyFIFO(const std::string &path)
        : NamedPipe<int, RetMsgStruct>(path, FileOpenMode::WriteOnly) {}
};

#endif // __NAMED_PIPE_H__