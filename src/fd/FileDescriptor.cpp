#include "src/fd/FileDescriptor.h"

void FileDescriptor::check_file_open()
{
    std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);
    if (!is_open_)
    {
        std::string err = "invalid file operation without open!";
        UtilError::error_exit(err, false);
    }

    if (!UtilFile::is_valid_fd(fd_))
    {
        UtilError::error_exit("fd " + std::to_string(fd_) + " is invalid", false);
    }
    Log::debug("fd " + std::to_string(fd_) + " is valid");
}

void FileDescriptor::check_read_result(int res)
{
    // 读取失败，抛出异常
    if (res == -1)
    {
        std::string err = "file read error";
        UtilError::error_exit(err, true);
    }
    // EOF，可能是读过头或者写端关闭管道，调用回调函数
    else if (res == 0)
    {
        int err = errno;
        std::string info = "file read EOF, call EOF callback function";
        Log::info(info);
        eof_callback(err);
    }
}

void FileDescriptor::check_write_result(int res)
{
    // 写没有=0时EOF的选项
    if (res == -1)
    {
        std::string err = "file write error";
        UtilError::error_exit(err, true);
    }
}

FileDescriptor::FileDescriptor(FileOpenMode open_mode)
    : open_mode_(open_mode) {}

int FileDescriptor::readfile(void *buf, size_t n)
{
    // 上锁
    std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);

    if (open_mode_ == FileOpenMode::WriteOnly)
    {
        std::string err = "attempt to read in write only file.";
        UtilError::error_exit(err, false);
    }

    check_file_open();
    int res = read(fd_, buf, n);
    check_read_result(res);

    // log
    Log::debug(std::to_string(res) + "/" + std::to_string(n) +
               " bytes was read from: " + std::to_string(fd_));

    return res;
}

int FileDescriptor::get_fd() { return fd_; }

int FileDescriptor::writefile(void *buf, size_t n)
{
    // 上锁
    std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);

    if (open_mode_ == FileOpenMode::ReadOnly)
    {
        std::string err = "attempt to write in read only file.";
        UtilError::error_exit(err, false);
    }

    check_file_open();
    int res = write(fd_, buf, n);
    check_write_result(res);

    // log
    Log::debug(std::to_string(res) + "/" + std::to_string(n) +
               " bytes was sent to: " + std::to_string(fd_));

    return res;
}

int FileDescriptor::closefile()
{
    // 上锁
    std::unique_lock<std::recursive_mutex> lock(file_operation_mutex_);

    Log::debug("file with fd " + std::to_string(fd_) + " is closed");
    close(fd_);
    is_open_ = false;

    return 1;
}

void FileDescriptor::writeline(std::string &s)
{
    // 加上换行符
    if (s.back() != '\n')
        s += '\n';

    // 写入
    const char *arr = s.c_str();
    writefile((void *)arr, s.size());
}

std::string FileDescriptor::readline()
{
    char ch;
    std::string result;

    while (true)
    {
        // 一次读1字节
        int num_bytes_read = readfile(&ch, 1);

        // EOF退出
        if (num_bytes_read == 0)
        {
            if (result.back() != '\n')
                result += '\n';
            break;
        }

        // 非EOF
        result += ch;

        // 读到换行符
        if (ch == '\n')
            break;
    }

    return result;
}