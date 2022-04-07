#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

namespace UtilFile
{
    static bool dir_exists(const std::string &path)
    {
        struct stat file_stat;
        bool res = (stat(path.c_str(), &file_stat) == 0) && (S_ISDIR(file_stat.st_mode));
        return res;
    }

    static bool dir_create(const std::string &path)
    {
        if (mkdir(path.c_str(), 0777) != 0)
        {
            perror("create_dir");
            return false;
        }
        return true;
    }

    static bool dir_remove(const std::string &path)
    {
        return rmdir(path.c_str()) == 0;
    }

    static bool file_exists(const std::string &path)
    {
        return access(path.c_str(), F_OK) != -1;
    }

    static int is_valid_fd(int fd)
    {
        return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
    }

    static bool fd_is_invalid_or_closed(int fd)
    {
        return fcntl(fd, F_GETFL) < 0 && errno == EBADF;
    }

} // namespace UtilFile

namespace UtilError
{
    static void error_exit(std::string msg, bool print_perror)
    {
        if (print_perror)
            perror(msg.c_str());
        else
            std::cout << msg << std::endl;
#ifdef DEBUG
        throw "error";
#else
        exit(EXIT_FAILURE);
#endif
    }
} // namespace UtilError

namespace UtilSystem
{
    // 变身守护进程
    static int init_daemon()
    {
        int pid;
        int i;
        /*忽略终端I/O信号，STOP信号*/
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        pid = fork();

        // 结束父进程，使得子进程成为后台进程
        if (pid > 0)
            exit(0);
        else if (pid < 0)
            UtilError::error_exit("fork fail while initializing deamon", true);

        // 建立一个新的进程组，在这个新的进程组中，子进程成为这个进程组的首进程，以使该进程脱离所有终端
        setsid();

        // 再次新建一个子进程，退出父进程，保证该进程不是进程组长，同时让该进程无法再打开一个新的终端
        pid = fork();
        if (pid > 0)
            exit(0);
        else if (pid < 0)
            UtilError::error_exit("fork fail while initializing deamon", true);

        // 关闭所有从父进程继承的不再需要的文件描述符
        for (i = 0; i < NOFILE; i++)
            close(i);

        // 改变工作目录，使得进程不与任何文件系统联系
        // chdir("/");

        // 将文件当时创建屏蔽字设置为0
        umask(0);

        // 忽略SIGCHLD信号
        signal(SIGCHLD, SIG_IGN);

        return 0;
    }
} // namespace UtilSystem

#endif // __UTIL_HPP__