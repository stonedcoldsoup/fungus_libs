#ifndef FUNGUSUTIL_FS_H
#define FUNGUSUTIL_FS_H

#include "../fungus_util_common.h"
#ifdef FUNGUSUTIL_POSIX

#include "../fungus_util_user.h"

#include <fstream>
#include <cerrno>
//#include <cunistd>
#include <cstring>
#include <sys/stat.h>

// TODO:
//  Implement a filesystem class and handle the special case for home directory '~', as well as integrating
//  management for std::fstreams.
namespace fungus_util
{
    namespace fs
    {
        enum permission: uint32_t
        {
            permission_owner_read = S_IRUSR,
            permission_owner_write = S_IWUSR,
            permission_owner_exec = S_IXUSR,
            permission_owner_all = S_IRWXU,

            permission_group_read = S_IRGRP,
            permission_group_write = S_IWGRP,
            permission_group_exec = S_IXGRP,
            permission_group_all = S_IRWXG,

            permission_other_read = S_IROTH,
            permission_other_write = S_IWOTH,
            permission_other_exec = S_IXOTH,
            permission_other_all = S_IRWXO
        };

        struct FUNGUSUTIL_API path_op
        {
            virtual std::string operator()(const std::string &m_path) = 0;
        };

        template <typename... path_opT>
        struct path_op_batch;

        template <typename path_opT, typename... path_opU>
        struct path_op_batch<path_opT, path_opU...>: path_op
        {
            virtual inline std::string operator()(const std::string &m_path)
            {
                static path_opT m_policy;
                static path_op_batch<path_opU...> m_next;

                return m_next(m_policy(m_path));
            }
        };

        template <>
        struct path_op_batch<>: path_op
        {
            virtual inline std::string operator()(const std::string &m_path)
            {
                return m_path;
            }
        };

        template <char _home_dir_char = '~'>
        struct resolve_home_path_op: path_op
        {
            virtual inline std::string operator()(const std::string &m_path)
            {
                static user_info m_user_info;

                std::string r = m_path;
                std::string::size_type i;

                // when we find a '~' or (whatever _home_dir_char is), effectively
                // replace it with the home directory by appending the home directory
                // to the leading portion of the path, and then appending the following
                // portion after that.
                while ((i = r.find_first_of(_home_dir_char)) != std::string::npos)
                    r = r.substr(0, i) + m_user_info.get_home_dir() + r.substr(i + 1);

                return r;
            }
        };

        template <typename... path_opT>
        static inline std::string do_path_op(const std::string &m_path)
        {
            path_op_batch<path_opT...> m_op;
            return m_op(m_path);
        }

        FUNGUSUTIL_API bool make_path(const std::string &m_path, uint32_t m_permission_flags = permission_owner_all|permission_group_read);

        // an open function designed to replace the standard ofstream open
        // call.  creates the missing directories using the given permissions
        // flags if they don't exist.
        FUNGUSUTIL_API bool ofstream_make_path_open(std::ofstream &m_os,
                                                    const std::string &m_path,
                                                    std::ios_base::openmode m_openmode = std::ios_base::out|std::ios_base::trunc,
                                                    uint32_t m_dir_flags               = permission_owner_all|permission_group_read);
    }
}

#endif
#endif
