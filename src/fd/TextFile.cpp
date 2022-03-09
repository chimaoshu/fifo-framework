#include "src/fd/TextFile.h"

TextFile::TextFile(const std::string &s, FileOpenMode open_mode)
    : FileWithPath(s, open_mode) {}

int TextFile::createfile()
{
    // 文件存在则删除
    if (file_exits())
    {
        Log::warn("file exists, delete and create again");
        deletefile();
    }

    if (creat(path_.c_str(), 0777) != 0)
    {
        std::string err = "create text file " + path_ + " failed";
        UtilError::error_exit(err, true);
    }
    return 1;
}

WriteOnlyTextFile::WriteOnlyTextFile(const std::string &s)
    : TextFile(s, FileOpenMode::WriteOnly) {}

ReadOnlyTextFile::ReadOnlyTextFile(const std::string &s)
    : TextFile(s, FileOpenMode::ReadOnly) {}