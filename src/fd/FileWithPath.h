#ifndef __FILE_WITH_PATH_H__
#define __FILE_WITH_PATH_H__

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "src/utils/util.hpp"
#include "src/fd/FileDescriptor.h"

// 带路径的文件，比如命名管道、普通文件
class FileWithPath : public FileDescriptor
{
protected:
    std::string path_;
    // 判读路径上文件是否存在
    bool file_exits();

public:
    // FileWithPath(const FileWithPath &) = delete;
    // FileWithPath &operator=(const FileWithPath &) = delete;
    FileWithPath(const std::string &s, FileOpenMode open_mode);
    int deletefile();
    int openfile();
};

#endif // __FILE_WITH_PATH_H__