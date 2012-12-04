#ifdef FUNGUSUTIL_POSIX
#include "fungus_util_fs.h"

static inline bool do_mkdir(const char *m_path, mode_t m_mode)
{
    struct stat m_stat;
    bool b_success = true;

    if (stat(m_path, &m_stat) != 0)
    {
        // directory doesn't exist
        if (mkdir(m_path, m_mode) != 0)
            b_success = false;
    }
    else if (!S_ISDIR(m_stat.st_mode))
    {
        errno = ENOTDIR;
        b_success = false;
    }

    return b_success;
}

namespace fungus_util
{
    namespace fs
    {
        // this is a mess!  CLEAN ME THE FUCK UP!
        bool make_path(const std::string &m_path, uint32_t m_permission_flags)
        {
            mode_t m_mode = (mode_t)m_permission_flags;

            char *pp, *sp;
            bool b_success = true;
            char *copypath = strdup(m_path.c_str());

            pp = copypath;
            while (b_success && (sp = strchr(pp, '/')) != 0)
            {
                if (sp != pp)
                {
                    /* Neither root nor double slash in path */
                    *sp = '\0';
                    b_success = do_mkdir(copypath, m_mode);
                    *sp = '/';
                }
                pp = sp + 1;
            }

            if (b_success)
                b_success = do_mkdir(m_path.c_str(), m_mode);

            free(copypath);
            return b_success;
        }

        bool ofstream_make_path_open(std::ofstream &m_os,
                                     const std::string &m_path,
                                     std::ios_base::openmode m_openmode,
                                     uint32_t m_dir_flags)
        {
            // try to open the file
            m_os.open(m_path, m_openmode);
            bool b_success = m_os;

            // if we failed, make sure the path exists and try again.
            if (!b_success)
            {
                // look for a slash
                std::string m_path_cpy(m_path);
                size_t i = m_path_cpy.find_last_of('/');

                // if we found a slash...
                if (i != std::string::npos)
                {
                    m_path_cpy.erase(i); // ...delete everything from the slash on so that we just have the containing directory.
                    if (make_path(m_path_cpy, m_dir_flags)) // make the path.
                    {
                        // try again and get success
                        m_os.open(m_path, m_openmode);
                        b_success = m_os;
                    }
                }
            }

            // return success (the ostream in question can be queried for details regarding failure).
            return b_success;
        }
    }
}
#endif
