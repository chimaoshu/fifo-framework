#include <string.h>

#include <iostream>

#include "src/fd/FileDescriptor.h"
#include "src/fd/FileWithPath.h"
#include "src/fd/NamedPipe.h"
#include "src/fd/Stdio.hpp"
#include "src/fd/TextFile.h"

using namespace std;

struct RegRecv
{
    char username[64];
    char password[64];
};

void test_named_pipe()
{
    WriteOnlyFIFO<char *> pipe("/home/user/pipes/server/user_register");
    pipe.openfile();
    string msg = "123";
    pipe.writeline(msg);
    pipe.closefile();

    WriteOnlyFIFO<RegRecv> pipe2("/home/user/pipes/server/user_register");
    pipe2.openfile();
    RegRecv reg;
    string username = "123";
    string password = "456";
    strcpy(reg.username, username.c_str());
    strcpy(reg.password, password.c_str());
    pipe2.send_msg(reg);
    pipe2.closefile();
}

int main()
{
    test_named_pipe();
}