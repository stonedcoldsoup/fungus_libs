#include "fungus_util_string_op.h"

namespace fungus_util
{
    whitespace_string::whitespace_string(const std::string &m_str): m_str(m_str) {}
    whitespace_string::whitespace_string(std::string &&m_str): m_str(std::move(m_str)) {}

    whitespace_string::whitespace_string(const whitespace &m_wh): m_str(m_wh)                      {}
    whitespace_string::whitespace_string(whitespace_string &&m_whs): m_str(std::move(m_whs.m_str)) {}

    whitespace_string &whitespace_string::operator =(const std::string &m_str) {this->m_str = m_str; return *this;}
    whitespace_string &whitespace_string::operator =(std::string &&m_str) {this->m_str = std::move(m_str); return *this;}

    whitespace_string &whitespace_string::operator =(const whitespace &m_wh) {m_str = m_wh; return *this;}
    whitespace_string &whitespace_string::operator =(whitespace_string &&m_whs) {m_str = std::move(m_whs.m_str); return *this;}

    whitespace_string::operator const std::string &() const {return m_str;}

    std::string trim_leading_whitespace(const std::string &m_str, const whitespace &m_white)
    {
        std::string r = m_str;

        std::string::size_type i = r.find_first_not_of(m_white);
        return i != std::string::npos ?
               r.substr(i)            :
               "";
    }

    std::string trim_following_whitespace(const std::string &m_str, const whitespace &m_white)
    {
        // TODO: revise to more standard method.
        std::string m_white_str = m_white;
        for (std::string::size_type i = m_str.size() - 1; i < std::string::npos; --i)
        {
            if (m_white_str.find(m_str[i]) == std::string::npos)
                return m_str.substr(0, ++i);
        }

        return "";
    }

    std::string trim_whitespace(const std::string &m_str, const whitespace &m_white)
    {
        return trim_leading_whitespace(trim_following_whitespace(m_str, m_white), m_white);
    }
}
