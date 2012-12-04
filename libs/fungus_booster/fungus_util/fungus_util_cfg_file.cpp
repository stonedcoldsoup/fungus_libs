#include "fungus_util_cfg_file.h"

namespace fungus_util
{
    cfg_file::field::field():
        b_empty(false)
    {
        clear();
    }

    cfg_file::field::field(field &&m_field)
    {
        *this = std::move(m_field);
    }

    cfg_file::field::field(const field &m_field)
    {
        *this = m_field;
    }

    cfg_file::field &cfg_file::field::operator =(field &&m_field)
    {
        m_name = std::move(m_field.m_name);
        m_entries = std::move(m_field.m_entries);
        b_empty = m_field.b_empty;

        return *this;
    }

    cfg_file::field &cfg_file::field::operator =(const field &m_field)
    {
        m_name = m_field.m_name;
        m_entries = m_field.m_entries;
        b_empty = m_field.b_empty;

        return *this;
    }

    void cfg_file::field::clear()
    {
        if (!b_empty)
        {
            m_name = "";
            m_entries.clear();

            b_empty = true;
        }
    }

    bool cfg_file::field::empty() const
    {
        return b_empty;
    }

    const std::vector<std::string> &cfg_file::field::entries() const {return m_entries;}
    const std::string &cfg_file::field::name() const                 {return m_name;}

    void cfg_file::field::set_name(const std::string &m_name)
    {
        this->m_name = m_name;

        if (b_empty && !this->m_name.empty()) b_empty = false;
    }

    void cfg_file::field::add_entry(const std::string &m_entry)
    {
        m_entries.push_back(m_entry);

        b_empty = false;
    }

    cfg_file::cfg_file(const cfg_file &m_cfg):
        m_wop(m_cfg.m_wop->clone()),
        m_fields(m_cfg.m_fields)
    {}

    cfg_file::cfg_file(cfg_file &&m_cfg):
        m_wop(m_cfg.m_wop->clone()),
        m_fields(std::move(m_cfg.m_fields))
    {}

    cfg_file &cfg_file::operator =(const cfg_file &m_cfg)
    {
        m_wop = m_cfg.m_wop->clone();
        m_fields = m_cfg.m_fields;
        return *this;
    }

    cfg_file &cfg_file::operator =(cfg_file &&m_cfg)
    {
        m_wop = m_cfg.m_wop->clone();
        m_fields = std::move(m_cfg.m_fields);
        return *this;
    }

    cfg_file::cfg_file(const v_whitespace_op &m_wop):
        m_wop(m_wop.clone()),
        m_fields()
    {}

    cfg_file::cfg_file(std::istream &m_is, const v_whitespace_op &m_wop):
        m_wop(m_wop.clone()),
        m_fields()
    {
        load(m_is);
    }

    cfg_file::~cfg_file()
    {
        delete m_wop;
    }

          std::vector<cfg_file::field> &cfg_file::fields()       {return m_fields;}
    const std::vector<cfg_file::field> &cfg_file::fields() const {return m_fields;}

    // returns the number of fields grabbed.
    size_t cfg_file::load(std::istream &m_is)
    {
        m_fields.clear();

        // the first thing we do is scan until we find the first
        // field.
        std::string m_line, m_name = "";
        while (std::getline(m_is, m_line))
        {
            m_line = (*m_wop)(m_line);

            // a line that starts with a '[' is a field name.
            if (m_line[0] == '[')
            {
                m_name = m_line.substr(1, m_line.size() - 2);
                break;
            }
        }

        // if we reached the end of the file and
        // no name line was found, we do nothing.
        if (m_name.empty())
            return 0;

        // now we do a normal scan, warm started
        // with the first field.
        field m_field;                  // define the field here so it is never allocated unless it's needed.
        m_field.set_name(m_name);
        while (std::getline(m_is, m_line))
        {
            m_line = (*m_wop)(m_line);

            // skip empty lines.
            if (!m_line.empty())
            {
                // found a name line, upload field and start new one.
                if (m_line[0] == '[')
                {
                    if (!m_field.empty())
                        m_fields.push_back(std::move(m_field)); // move the current field on to the list

                    // start a new field.
                    field m_new_field;
                    m_new_field.set_name(m_line.substr(1, m_line.size() - 2));

                    // move the new field in to the workspace field.
                    m_field = std::move(m_new_field);
                }
                else // add this line as an entry.
                    m_field.add_entry(m_line);
            }
        }

        if (!m_field.empty())
            m_fields.push_back(std::move(m_field));

        return m_fields.size();
    }

    bool cfg_file::save(std::ostream &m_os) const
    {
        bool b_ok = true;
        for (const field &m_field: m_fields)
        {
            // if an error ocurred, abort.
            // the user can check the stream
            // for details, as is standard
            // in C++.
            if (!m_os)
            {
                b_ok = false;
                break;
            }

            // skip empty fields
            if (!m_field.empty())
            {
                // write out the name
                m_os << '[' << (*m_wop)(m_field.name()) << "]\n";

                // write out each entry under that name.
                for (const std::string &m_entry: m_field.entries())
                    m_os << (*m_wop)(m_entry) << '\n';
            }
        }

        if (b_ok)
            m_os.flush();

        return b_ok;
    }
}
