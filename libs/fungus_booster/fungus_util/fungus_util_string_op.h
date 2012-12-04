#ifndef FUNGUSUTIL_STRING_OP_H
#define FUNGUSUTIL_STRING_OP_H

#include "fungus_util_common.h"

#define FUNGUSUTIL_DEFAULT_WHITESPACE_STRING " \t\v\r\f\n"

namespace fungus_util
{
    class FUNGUSUTIL_API whitespace
    {
    public:
        virtual operator const std::string &() const = 0;
    };

    class FUNGUSUTIL_API whitespace_string: public whitespace
    {
    private:
        std::string m_str;
    public:
        whitespace_string(const std::string &m_str = FUNGUSUTIL_DEFAULT_WHITESPACE_STRING);
        whitespace_string(std::string &&m_str);

        whitespace_string(const whitespace &m_wh);
        whitespace_string(whitespace_string &&m_whs);

        whitespace_string &operator =(const std::string &m_str);
        whitespace_string &operator =(std::string &&m_str);

        whitespace_string &operator =(const whitespace &m_wh);
        whitespace_string &operator =(whitespace_string &&m_whs);

        virtual operator const std::string &() const;
    };

    // TODO: add locale whitespace class.

    FUNGUSUTIL_API std::string trim_leading_whitespace(const std::string &m_str, const whitespace &m_white = whitespace_string());
    FUNGUSUTIL_API std::string trim_following_whitespace(const std::string &m_str, const whitespace &m_white = whitespace_string());
    FUNGUSUTIL_API std::string trim_whitespace(const std::string &m_str, const whitespace &m_white = whitespace_string());

    // TODO: change the whitespace op suite to follow a more consistent
    // idiom.  use the same virtual/concrete pragmatic construct as
    // path_op in the budding fs module.
    enum whitespace_op_e
    {
        trim_leading,
        trim_following
    };

    template <whitespace_op_e... _whitespace_op>
    struct whitespace_op;

    template <whitespace_op_e... _whitespace_op>
    struct whitespace_op<trim_leading, _whitespace_op...>
    {
        inline std::string operator()(const std::string &m_in, const whitespace &m_white = whitespace_string())
        {
            static whitespace_op<_whitespace_op...> m_next_op;
            return m_next_op(trim_leading_whitespace(m_in, m_white), m_white);
        }
    };

    template <whitespace_op_e... _whitespace_op>
    struct whitespace_op<trim_following, _whitespace_op...>
    {
        inline std::string operator()(const std::string &m_in, const whitespace &m_white = whitespace_string())
        {
            static whitespace_op<_whitespace_op...> m_next_op;
            return m_next_op(trim_following_whitespace(m_in, m_white), m_white);
        }
    };

    template <>
    struct whitespace_op<>
    {
        inline std::string operator()(const std::string &m_in, const whitespace &m_white = whitespace_string())
        {
            return m_in;
        }
    };

    struct v_whitespace_op
    {
        virtual ~v_whitespace_op() {}
        virtual std::string operator()(const std::string &m_in, const whitespace &m_white = whitespace_string()) = 0;
        virtual v_whitespace_op *clone() const = 0;
    };

    template <whitespace_op_e... _whitespace_op>
    struct v_whitespace_op_inst: v_whitespace_op
    {
        virtual std::string operator()(const std::string &m_in, const whitespace &m_white = whitespace_string())
        {
            static whitespace_op<_whitespace_op...> m_op;
            return m_op(m_in, m_white);
        }

        virtual v_whitespace_op *clone() const {return new v_whitespace_op_inst;}
    };
}

#endif
