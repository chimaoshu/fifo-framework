#ifndef __CHAT_MODEL__
#define __CHAT_MODEL__

#include <iostream>

// 协议，用户自己编写
namespace Protocal
{
    // 协议类型
    // 所有协议的结构体都以该字段开头，用于区分
    enum ProtocalType
    {
        RegRecv,
        RegRet,
        LoginRecv,
        LoginRet,
        MsgRecv,
        MsgRet,
        LogoutRecv,
        LogoutRet,
    };

    namespace Reg
    {
        struct RegRecv
        {
            ProtocalType protocal_type = ProtocalType::RegRecv;
            char username[64];
            char password[64];
        };

        enum RegStatus : int
        {
            register_success,
            username_has_been_registered,
            empty_username,
            empty_password
        };

        struct RegRet
        {
            ProtocalType protocal_type = ProtocalType::RegRet;
            RegStatus status;
        };

        static std::string get_string_by_status(RegStatus status)
        {
            static std::map<RegStatus, std::string> m{
                {register_success, "register success"},
                {username_has_been_registered, "username has been registered"},
                {empty_username, "empty username"},
                {empty_password, "empty password"},
            };
            return m[status];
        }
    } // namespace Reg

    namespace Login
    {
        struct LoginRecv
        {
            ProtocalType protocal_type = ProtocalType::LoginRecv;
            char username[64];
            char password[64];
        };

        enum LoginStatus : int
        {
            login_success,
            username_not_registered,
            error_password,
            max_online_user
        };

        struct LoginRet
        {
            ProtocalType protocal_type = ProtocalType::LoginRet;
            LoginStatus status;
        };

        static std::string get_string_by_status(LoginStatus status)
        {
            static std::map<LoginStatus, std::string> m{
                {login_success, "login success"},
                {username_not_registered, "username not registered"},
                {error_password, "error password"},
                {max_online_user, "online user has reached the max number"}
            };
            return m[status];
        }

    } // namespace Login

    namespace Msg
    {
        struct MsgRecv
        {
            ProtocalType protocal_type = ProtocalType::MsgRecv;
            char from[64];
            char to[64];
            char msg[64];
        };

        enum MsgStatus : int
        {
            forward_success,
            user_not_online,
            user_not_exist,
            you_not_online,
            you_not_registered
        };

        struct MsgRet
        {
            ProtocalType protocal_type = ProtocalType::MsgRet;
            MsgStatus status;
        };

        static std::string get_string_by_status(MsgStatus status)
        {
            static std::map<MsgStatus, std::string> m{
                {forward_success, "messafe forward success"},
                {user_not_online, "another user is not online"},
                {user_not_exist, "another user is not exist"},
                {you_not_online, "you are not online"},
                {you_not_registered, "you are not register"},
            };
            return m[status];
        }

    } // namespace Logout

    namespace Logout
    {
        struct LogoutRecv
        {
            ProtocalType protocal_type = ProtocalType::LogoutRecv;
            char username[64];
        };

        enum LogoutStatus : int
        {
            logout_success,
            username_not_registered,
            username_not_online,
        };

        struct LogoutRet
        {
            ProtocalType protocal_type = ProtocalType::LogoutRet;
            LogoutStatus status;
        };

        static std::string get_string_by_status(LogoutStatus status)
        {
            static std::map<LogoutStatus, std::string> m{
                {logout_success, "logout success"},
                {username_not_registered, "you are not registerd"},
                {username_not_online, "you are not online"},
            };
            return m[status];
        }

    } // namespace Logout
} // namespace Protocal

#endif // __CHAT_MODEL__