#include "src/fd/FileWithPath.h"

FileWithPath::FileWithPath(const std::string &s, FileOpenMode open_mode)
    : FileDescriptor(open_mode), path_(s) {}

bool FileWithPath::file_exits()
{
    int res = (access(path_.c_str(), F_OK) == 0);
    if (!res)
        Log::debug("file " + path_ + " not exists");
    return res;
}

int FileWithPath::deletefile()
{
    if (unlink(path_.c_str()) != 0)
    {
        std::string err = "remove file " + path_ + " fail";
        UtilError::error_exit(err, false);
    }
    Log::debug("file " + path_ + " delete success");
    return 1;
}

int FileWithPath::openfile()
{
    if (is_open_)
        return 1;

    // 只读与只写只通过模式判断，在打开文件时不限制，避免频繁EOF
    int mode;
    // if (open_mode_ == FileOpenMode::ReadOnly)
    //     mode = O_RDONLY | O_NONBLOCK;
    // else if (open_mode_ == FileOpenMode::WriteOnly)
    //     mode = O_WRONLY | O_NONBLOCK;
    // else
    mode = O_RDWR | O_NONBLOCK;

    if (!file_exits())
        createfile();

    fd_ = open(path_.c_str(), mode);
    if (fd_ == -1)
    {
        std::string err = "open file " + path_ + " fail";
        UtilError::error_exit(err, true);
    }
    is_open_ = true;
    Log::debug("file " + path_ + " open success with fd " + std::to_string(fd_));
    return 1;
}