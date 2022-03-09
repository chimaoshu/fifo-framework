#ifndef __STDIO_HPP__
#define __STDIO_HPP__

#include "src/fd/FileDescriptor.h"

class Stdio : public FileDescriptor
{
public:
    Stdio(FileOpenMode mode) : FileDescriptor(mode) { is_open_ = true; }
    int openfile() { return 0; };
    int createfile() { return 0; };
    int deletefile() { return 0; };
};

class Stdin : public Stdio
{
public:
    Stdin() : Stdio(FileOpenMode::ReadOnly) {}
};

class Stdout : public Stdio
{
public:
    Stdout() : Stdio(FileOpenMode::WriteOnly) {}
};

#endif // __STDIO_HPP__