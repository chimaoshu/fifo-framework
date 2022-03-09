#ifndef __FILE_DESCRIPTOR_H__
#define __FILE_DESCRIPTOR_H__

#include <mutex>
#include <thread>
#include <memory>

#include <assert.h>
#include <unistd.h>

#include "src/utils/util.hpp"
#include "src/log/Log.hpp"

enum FileOpenMode : int8_t
{
    ReadOnly,
    WriteOnly,
    ReadAndWrite
};

class FileDescriptor
{
protected:
    int fd_;
    bool is_open_ = false;
    const FileOpenMode open_mode_;

    // 锁
    std::recursive_mutex file_operation_mutex_;

    // 检查文件是否开启
    void check_file_open();

    // 检查读取返回结果
    void check_write_result(int res);
    void check_read_result(int res);

    // 写端按照规定的协议发送消息，供写端使用
    template <typename RetMsgStruct>
    bool send_struct(RetMsgStruct msg)
    {
        // 写不会出现EOF
        int res = writefile(&msg, sizeof(RetMsgStruct));
        check_write_result(res);

        // 缺
        if (res < sizeof(RetMsgStruct))
        {
            UtilError::error_exit("fd " + std::to_string(fd_) + ": number of bytes write is not euqal to number of RetMsgStruct", false);
            return false;
        }

        return true;
    }

    // 读端按照规定的协议读取消息，供读端使用
    template <typename RecvMsgStruct>
    bool recv_struct(RecvMsgStruct &in)
    {
        RecvMsgStruct msg;
        int res = readfile(&msg, sizeof(RecvMsgStruct));

        // EOF或错误检查
        check_read_result(res);
        if (res == 0)
            return res;

        // 缺
        if (res < sizeof(RecvMsgStruct))
        {
            // 错误退出
            UtilError::error_exit("fd " + std::to_string(fd_) + ": number of bytes read is not euqal to number of MsgStuct", false);
            return false;
        }

        // 赋值，返回
        in = msg;
        return true;
    }

public:
    FileDescriptor() = delete;
    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileOpenMode open_mode_);
    int get_fd();

    virtual int readfile(void *buf, size_t n);  // 返回读取字节数，0表示EOF
    virtual int writefile(void *buf, size_t n); // 返回写入字节数，0表示没写入
    virtual int closefile();                    // 关闭文件
    virtual int openfile() = 0;                 // 打开文件，不存在则创建，已打开则不操作
    virtual int createfile() = 0;               // 创建文件，存在则删除重新创建
    virtual int deletefile() = 0;               // 删除文件
    virtual void eof_callback(int err) = 0;     // EOF时回调
    virtual void recv_callback() = 0;           // 有输入时回调

    void writeline(std::string &s);             // 写一行字符串，保证以换行符结尾
    std::string readline();                     // 读一行字符串，保证以换行符结尾
};
#endif // __FILE_DESCRIPTOR_H_