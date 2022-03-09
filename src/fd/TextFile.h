#ifndef __TEXT_FILE_H__
#define __TEXT_FILE_H__

#include "src/fd/FileWithPath.h"

// 文本文件
class TextFile : public FileWithPath
{
public:
    // TextFile(const TextFile &) = delete;
    // TextFile &operator=(const TextFile &) = delete;
    TextFile(const std::string &s, FileOpenMode open_mode);
    int createfile();
};

class WriteOnlyTextFile : public TextFile
{
public:
    WriteOnlyTextFile(const std::string &s);
};

class ReadOnlyTextFile : public TextFile
{
public:
    ReadOnlyTextFile(const std::string &s);
};

#endif // __TEXT_FILE_H__