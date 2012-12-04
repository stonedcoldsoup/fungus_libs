#ifndef FUNGUSUTIL_CFG_FILE_H
#define FUNGUSUTIL_CFG_FILE_H

#include "fungus_util_common.h"
#include "fungus_util_string_op.h"
#include "fs/fungus_util_fs.h"

#include <vector>

namespace fungus_util
{
    class FUNGUSUTIL_API cfg_file
    {
    public:
        class FUNGUSUTIL_API field
        {
        private:
            std::string              m_name;
            std::vector<std::string> m_entries;

            bool b_empty;
        public:
            field();

            field(field &&m_field);
            field(const field &m_field);

            field &operator =(field &&m_field);
            field &operator =(const field &m_field);

            void clear();
            bool empty() const;

            const std::vector<std::string> &entries() const;
            const std::string &name() const;

            void set_name(const std::string &m_name);
            void add_entry(const std::string &m_entry);
        };

    private:
        v_whitespace_op *m_wop;
        std::vector<field> m_fields;

    public:
        cfg_file(const cfg_file &m_cfg);
        cfg_file(cfg_file &&m_cfg);

        cfg_file &operator =(const cfg_file &m_cfg);
        cfg_file &operator =(cfg_file &&m_cfg);

        cfg_file(const v_whitespace_op &m_wop = v_whitespace_op_inst<trim_following, trim_leading>());
        cfg_file(std::istream &m_is, const v_whitespace_op &m_wop = v_whitespace_op_inst<trim_following, trim_leading>());

        ~cfg_file();

              std::vector<field> &fields();
        const std::vector<field> &fields() const;

        // returns the number of fields grabbed.
        size_t load(std::istream &m_is);
        bool save(std::ostream &m_os) const;

        template <typename... inputT>
        cfg_file &field_from_strings(const std::string &m_name, const inputT&... m_entries);
    };

    namespace detail
    {
        template <typename inputT>
        struct __cfg_field_proc;

        template <>
        struct __cfg_field_proc<std::string>
        {
            static inline void __impl(cfg_file::field &m_field, const std::string &m_in)
            {
                m_field.add_entry(m_in);
            }
        };

        template <>
        struct __cfg_field_proc<std::vector<std::string>>
        {
            static inline void __impl(cfg_file::field &m_field, const std::vector<std::string> &m_in)
            {
                for (const std::string &m_entry: m_in)
                    m_field.add_entry(m_entry);
            }
        };

        template <typename... inputT>
        struct cfg_field_from_strings_impl;

        template <typename inputT, typename... inputU>
        struct cfg_field_from_strings_impl<inputT, inputU...>
        {
            static inline void __impl(cfg_file::field &m_field, const inputT &m_in, const inputU&... m_next)
            {
                __cfg_field_proc<inputT>::__impl(m_field, m_in);
                cfg_field_from_strings_impl<inputU...>::__impl(m_field, m_next...);
            }
        };

        template <>
        struct cfg_field_from_strings_impl<>
        {
            static inline void __impl(cfg_file::field &m_field) {}
        };
    }

    template <typename... inputT>
    static inline cfg_file::field cfg_field_from_strings(const std::string &m_name, const inputT&... m_entries)
    {
        cfg_file::field m_field;

        m_field.set_name(m_name);
        detail::cfg_field_from_strings_impl<inputT...>::__impl(m_field, m_entries...);

        return m_field;
    }

    // convenience wrapper, allows concatenation of cfg field construction.
    template <typename... inputT>
    cfg_file &cfg_file::field_from_strings(const std::string &m_name, const inputT&... m_entries)
    {
        m_fields.push_back(cfg_field_from_strings(m_name, m_entries...));
        return *this;
    }

    template <typename valueT>
    class cfg_field_reader
    {
    private:
        std::string m_name;
        std::vector<valueT> m_entries;

        bool b_found;

        void extract_values(const cfg_file::field &m_field)
        {
            m_name = m_field.name();

            for (const std::string &m_entry: m_field.entries())
            {
                valueT m_v;
                from_string(m_entry, m_v);
                m_entries.push_back(std::move(m_v));
            }
        }
    public:
        cfg_field_reader(const std::string &m_name, const cfg_file &m_file):
            m_name(m_name),
            m_entries(),
            b_found(false)
        {
            for (const cfg_file::field &m_field: m_file.fields())
            {
                if (m_field.name() == m_name)
                {
                    b_found = true;
                    extract_values(m_field);
                    break;
                }
            }
        }

        const std::vector<valueT> &entries() const
        {
            return m_entries;
        }

        const std::string &name() const
        {
            return m_name;
        }

        bool exists() const {return b_found;}
        bool empty() const {return !b_found || m_entries.empty();}
    };

    template <>
    class cfg_field_reader<std::string>
    {
    private:
        std::string m_name;
        std::vector<std::string> m_entries;

        bool b_found;

        void extract_values(const cfg_file::field &m_field)
        {
            m_name = m_field.name();
            m_entries = m_field.entries();
        }
    public:
        cfg_field_reader(const std::string &m_name, const cfg_file &m_file):
            m_name(m_name),
            m_entries(),
            b_found(false)
        {
            for (const cfg_file::field &m_field: m_file.fields())
            {
                if (m_field.name() == m_name)
                {
                    b_found = true;
                    extract_values(m_field);
                    break;
                }
            }
        }

        const std::vector<std::string> &entries() const
        {
            return m_entries;
        }

        const std::string &name() const
        {
            return m_name;
        }

        bool exists() const {return b_found;}
        bool empty() const {return !b_found || m_entries.empty();}
    };
};

#endif
