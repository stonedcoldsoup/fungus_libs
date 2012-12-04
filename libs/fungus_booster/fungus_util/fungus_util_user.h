#ifndef FUNGUSUTIL_USER_H
#define FUNGUSUTIL_USER_H

#include "fungus_util_common.h"

#ifdef FUNGUSUTIL_POSIX
#include <string>
#include <sys/types.h>
#include <pwd.h>

namespace fungus_util
{
    class user_info
    {
    private:
        passwd *__fis_passwd;

        std::string m_shell, m_user, m_home;
    public:
        user_info():
            __fis_passwd(getpwuid(getuid())),
            m_shell(__fis_passwd->pw_shell),
            m_user(__fis_passwd->pw_name),
            m_home(__fis_passwd->pw_dir)
        {}

        ~user_info()
        {
            __fis_passwd = nullptr;
        }

        inline const std::string &get_shell() const
        {
            return m_shell;
        }

        inline const std::string &get_name() const
        {
            return m_user;
        }

        inline const std::string &get_home_dir() const
        {
            return m_home;
        }

        inline std::string get_password()
        {
            return __fis_passwd->pw_passwd;
        }

        void refresh()
        {
            __fis_passwd = getpwuid(getuid());

            m_shell = __fis_passwd->pw_shell;
            m_user  = __fis_passwd->pw_name;
            m_home  = __fis_passwd->pw_dir;
        }
    };
}

#endif
#endif
