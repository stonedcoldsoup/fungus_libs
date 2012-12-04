#ifndef FUNGUSUTIL_HASH_MAP_H
#define FUNGUSUTIL_HASH_MAP_H

#include "fungus_util_common.h"

#ifdef FUNGUSUTIL_CPP11_PARTIAL

#include "fungus_util_prime.h"
#include "fungus_util_type_info_wrap.h"
#include "fungus_util_bidirectional_container_base.h"
#include "fungus_util_constexpr.h"
#include "fungus_util_sfinae.h"

#include <map>
#include <iterator>

namespace fungus_util
{
    enum hash_entry_action
    {
        hash_entry_no_action,
        hash_entry_complete_action
    };

    enum hash_entry_ptr_deletion_policy
    {
        hash_entry_ptr_delete,
        hash_entry_ptr_no_delete
    };

    typedef uint32_t map_index_type;
    struct default_hash_data_type {};

    template <size_t nbytes>
    class __default_primitive_hash_functor;

    #define __FUNGUSUTIL_SMALL_PRIMITIVE_HASH_FUNCTOR_DEF(Size, Bits, KeyT)                 \
    template <>                                                                             \
    class __default_primitive_hash_functor<Size>                                            \
    {                                                                                       \
    private:                                                                                \
        __default_primitive_hash_functor32 __fn;                                            \
    public:                                                                                 \
        typedef KeyT key_type;                                                              \
                                                                                            \
        FUNGUSUTIL_ALWAYS_INLINE                                                            \
        inline map_index_type operator()(key_type key, map_index_type max) const            \
        {                                                                                   \
            return __fn((typename __default_primitive_hash_functor32::key_type)key, max);   \
        }                                                                                   \
    };                                                                                      \
                                                                                            \
    typedef __default_primitive_hash_functor<Size> __default_primitive_hash_functor##Bits   ;

    template <>
    class __default_primitive_hash_functor<4>
    {
    public:
        typedef uint32_t key_type;

        // hash function by Thomas Wang
        inline map_index_type operator()(key_type key, map_index_type max) const FUNGUSUTIL_ALWAYS_INLINE
        {
            key = (key ^ 61) ^ (key >> 16);
            key = key + (key << 3);
            key = key ^ (key >> 4);
            key = key * 0x27d4eb2d;
            key = key ^ (key >> 15);
            return key % max;
        }
    };

    template <>
    class __default_primitive_hash_functor<8>
    {
    public:
        typedef uint64_t key_type;

        // source unknown
        FUNGUSUTIL_ALWAYS_INLINE inline map_index_type operator()(key_type key, map_index_type max) const
        {
            key = (~key) + (key << 18); // key = (key << 18) - key - 1;
            key = key ^ (key >> 31);
            key = key * 21; // key = (key + (key << 2)) + (key << 4);
            key = key ^ (key >> 11);
            key = key + (key << 6);
            key = key ^ (key >> 22);

            return (map_index_type)key % max;
        }
    };

    typedef __default_primitive_hash_functor<4> __default_primitive_hash_functor32;
    typedef __default_primitive_hash_functor<8> __default_primitive_hash_functor64;

    __FUNGUSUTIL_SMALL_PRIMITIVE_HASH_FUNCTOR_DEF(1, 8,  uint8_t)
    __FUNGUSUTIL_SMALL_PRIMITIVE_HASH_FUNCTOR_DEF(2, 16, uint16_t)

    template <typename T, typename U>
    constexpr bool __sizeof_greater_equal_than()
    {
        return constexpr_sizeof<T>() >= constexpr_sizeof<U>();
    }

    template <typename keyT>
    class __default_hash_key_fn_impl
    {
    private:
        typedef __default_primitive_hash_functor<constexpr_sizeof<keyT>()> __functor_type;

        typedef keyT                                                       __instance_key_type;
        typedef typename __functor_type::key_type                          __functor_key_type;

        fungus_util_constexpr_assert((__sizeof_greater_equal_than<__functor_key_type, keyT>()), sizecheck);

        union __key_union
        {
            __functor_key_type  fk;
            __instance_key_type ik;
        };

        FUNGUSUTIL_ALWAYS_INLINE static inline __functor_key_type convert(const __instance_key_type &key)
        {
            __key_union ku;
            ku.ik = key;
            return ku.fk;
        }
    public:
        FUNGUSUTIL_ALWAYS_INLINE static inline map_index_type __impl(const keyT &key, map_index_type max)
        {
            static const __functor_type __functor = __functor_type();

            return __functor(convert(key), max);
        }
    };

    template <>
    class __default_hash_key_fn_impl<std::string>
    {
    public:
        FUNGUSUTIL_ALWAYS_INLINE static inline map_index_type __impl(const std::string &key, map_index_type max)
        {
            map_index_type hash = 0;

			for (size_t i = 0; i < key.size(); ++i)
                hash ^= (hash << 5) + (hash >> 2) + key[i];

			return hash % max;
        }
    };

    template <>
    class __default_hash_key_fn_impl<type_info_wrap>
    {
    public:
        FUNGUSUTIL_ALWAYS_INLINE static inline map_index_type __impl(const type_info_wrap &key, map_index_type max)
        {
            return __default_hash_key_fn_impl<std::string>::__impl
                    (key.name(), max);
        }
    };

    template <typename keyT, typename valueT, typename dataT = default_hash_data_type,
              hash_entry_ptr_deletion_policy _delete_policy = hash_entry_ptr_delete>
    class hash
    {
    public:
        typedef   keyT   key_type;
        typedef   valueT value_type;
        typedef   dataT  data_type;

        FUNGUSUTIL_ALWAYS_INLINE inline map_index_type    hash_key(const keyT &key, map_index_type max)
        {
            return __default_hash_key_fn_impl<keyT>::__impl(key, max);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_insert(const keyT &key,
                                                      valueT &value,
                                                      dataT &data)
        {
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_replace(const keyT &key,
                                                       valueT &old_value,
                                                       valueT &value,
                                                       dataT &data)
        {
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_remove(const keyT &key,
                                                      valueT &value,
                                                      dataT &data)
        {
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline void hash_entry_force_remove(const keyT &key,
                                            valueT &value,
                                            dataT &data)
        {}
    };

    template <typename keyT, typename valueT, typename dataT>
    class hash<keyT, valueT *, dataT, hash_entry_ptr_delete>
    {
    public:
        typedef   keyT    key_type;
        typedef   valueT *value_type;
        typedef   dataT   data_type;

        FUNGUSUTIL_ALWAYS_INLINE inline map_index_type    hash_key(const keyT &key, map_index_type max)
        {
            return __default_hash_key_fn_impl<keyT>::__impl(key, max);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_insert(const keyT &key,
                                                      valueT *value,
                                                      dataT &data)
        {
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_replace(const keyT &key,
                                                       valueT &old_value,
                                                       valueT *value,
                                                       dataT &data)
        {
            delete old_value;
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_remove(const keyT &key,
                                                      valueT *value,
                                                      dataT &data)
        {
            delete value;
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline void hash_entry_force_remove(const keyT &key,
                                            valueT *value,
                                            dataT &data)
        {
            delete value;
        }
    };

    template <typename keyT, typename valueT,
              hash_entry_ptr_deletion_policy _delete_policy = hash_entry_ptr_delete>
    class default_hash:
        public hash<keyT, valueT, default_hash_data_type, _delete_policy>
    {};

    template <typename keyT, typename valueT,
              hash_entry_ptr_deletion_policy _delete_policy = hash_entry_ptr_delete>
    class default_hash_no_replace: public default_hash<keyT, valueT, _delete_policy>
    {
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_replace(const keyT &key,
                                                       valueT &old_value,
                                                       valueT &value,
                                                       default_hash_data_type __dummy)
        {
            return hash_entry_no_action;
        }
    };

    template <bool _b_implemented, typename valueT>
    struct __ref_ctr_hash_values_equal;

    template <typename valueT>
    struct __ref_ctr_hash_values_equal<true, valueT>
    {
        static inline bool __impl(const valueT &a, const valueT &b)
        {
            return a == b;
        }
    };

    template <typename valueT>
    struct __ref_ctr_hash_values_equal<false, valueT>
    {
        static inline bool __impl(const valueT &a, const valueT &b)
        {
            return true;
        }
    };

    template <typename keyT, typename valueT,
              hash_entry_ptr_deletion_policy _delete_policy = hash_entry_ptr_delete>
    class ref_ctr_hash: public hash<keyT, valueT, uint32_t, _delete_policy>
    {
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_insert(const keyT &key,
                                                      valueT &value,
                                                      uint32_t &nrefs)
        {
            nrefs = 1;
            return hash_entry_complete_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_remove(const keyT &key,
                                                      valueT &value,
                                                      uint32_t &nrefs)
        {
            if (--nrefs == 0)
            {
                return hash<keyT, valueT, uint32_t>::
                    hash_entry_on_remove(key, value, nrefs);
            }
            else
                return hash_entry_no_action;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_entry_action hash_entry_on_replace(const keyT &key,
                                                       valueT &old_value,
                                                       valueT &value,
                                                       uint32_t &nrefs)
        {
            if (__ref_ctr_hash_values_equal<sfinae::supports_equal_to<valueT>::value, valueT>::__impl(old_value, value))
            {
                ++nrefs;
                return hash_entry_no_action;
            }
            else
            {
                nrefs = 1;
                return hash<keyT, valueT, uint32_t>::
                    hash_entry_on_replace(key, old_value, value, nrefs);
            }
        }
    };

    class hash_resize_policy
    {
    public:
        // returns 0 if no resize needed
        // returns new table size otherwise
        size_t resize_to(size_t table_size, size_t n)
        {
            if (n < table_size)
                return 0;
            else
            {
                while (n >= table_size)
                    table_size = next_prime(table_size + 1);

                return table_size;
            }
        }
    };

    template <typename hashT, size_t init_size = 47, typename hash_resize_policyT = hash_resize_policy>
    class hash_map: public __container_list_base
    {
    public:
        typedef typename hashT::key_type   key_type;
        typedef typename hashT::value_type value_type;
        typedef typename hashT::data_type  data_type;

        class entry
        {
        public:
            key_type   key;
            value_type value;
            data_type  data;

            FUNGUSUTIL_ALWAYS_INLINE inline entry(): key(), value(), data() {}

            FUNGUSUTIL_ALWAYS_INLINE inline entry(entry &&e):
                key(std::move(e.key)), value(std::move(e.value)), data(std::move(e.data))
            {}

            FUNGUSUTIL_ALWAYS_INLINE inline entry(const entry &e):
                key(e.key), value(e.value), data(e.data)
            {}

            FUNGUSUTIL_ALWAYS_INLINE inline entry(const key_type &key, value_type &&value):
                key(key), value(std::move(value)), data()
            {}

            FUNGUSUTIL_ALWAYS_INLINE inline entry(const key_type &key, const value_type &value):
                key(key), value(value), data()
            {}

            FUNGUSUTIL_ALWAYS_INLINE inline operator std::pair<key_type, value_type>()
            {
                return std::pair<key_type, value_type>(key, value);
            }

            FUNGUSUTIL_ALWAYS_INLINE inline operator const std::pair<const key_type, value_type>() const
            {
                return std::pair<const key_type, value_type>(key, value);
            }
        private:
            FUNGUSUTIL_ALWAYS_INLINE inline entry(const key_type &key, value_type &&value, const data_type &data):
                key(key), value(std::move(value)), data(data)
            {}

            FUNGUSUTIL_ALWAYS_INLINE inline entry(const key_type &key, const value_type &value, const data_type &data):
                key(key), value(value), data(data)
            {}

            friend class hash_map;
        };
    private:
        struct cell: __container_list_base::elem
        {
            hash_map *parent;

            entry content;

            cell *chain_next;

            FUNGUSUTIL_ALWAYS_INLINE inline cell(entry &&c, hash_map *parent):
                __container_list_base::elem(parent), content(std::move(c)), chain_next(nullptr)
            {}
        };

        struct cell_manip
        {
            cell *&__top;
            cell *__cell;
            cell *__prev;

            FUNGUSUTIL_ALWAYS_INLINE inline cell_manip(cell *&top): __top(top), __cell(top), __prev(nullptr) {}
            FUNGUSUTIL_ALWAYS_INLINE inline cell_manip(const cell_manip &s): __top(s.__top), __cell(s.__top), __prev(s.__prev) {}

            FUNGUSUTIL_ALWAYS_INLINE inline void reset()
            {
                __cell = __top;
                __prev = nullptr;
            }

            FUNGUSUTIL_ALWAYS_INLINE inline void insert(cell *__new_cell)
            {
                __new_cell->chain_next = __top;
                __top = __new_cell;
            }

            FUNGUSUTIL_ALWAYS_INLINE inline bool find(const key_type &key)
            {
                reset();

                while (__cell)
                {
                    if (__cell->content.key == key)
                        break;

                    __prev = __cell;
                    __cell = __cell->chain_next;
                }

                return __cell != nullptr;
            }

            FUNGUSUTIL_ALWAYS_INLINE inline bool remove()
            {
                bool success = __cell != nullptr;

                if (success)
                {
                    if (__top == __cell)
                        __top = __cell->chain_next;
                    else
                        __prev->chain_next = __cell->chain_next;
                }

                return success;
            }
        };

        size_t m_table_size;
        mutable cell **table;

        mutable hashT __hash_impl;
        mutable hash_resize_policyT __resize_policy;
    public:
        class iterator;
        class const_iterator;

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(const hash_map &__map):
            __container_list_base(),
            m_table_size(__map.m_table_size),
            __hash_impl(__map.__hash_impl),
            __resize_policy(__map.__resize_policy)
        {
            table = new cell *[this->m_table_size];
            memset(table, 0, sizeof(cell *) * this->m_table_size);

            *this = __map;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(hash_map &&__map):
            __container_list_base(),
            table(nullptr)
        {
            *this = std::move(__map);
        }

        template <size_t __init_size, typename __hash_resize_policyT>
        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(const hash_map<hashT, __init_size, __hash_resize_policyT> &__map):
            __container_list_base(),
            m_table_size(__map.m_table_size),
            __hash_impl(__map.__hash_impl),
            __resize_policy(__map.__resize_policy)
        {
            table = new cell *[this->m_table_size];
            memset(table, 0, sizeof(cell *) * this->m_table_size);

            *this = __map;
        }

        template <size_t __init_size, typename __hash_resize_policyT>
        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(hash_map<hashT, __init_size, __hash_resize_policyT> &&__map):
            __container_list_base(),
            table(nullptr)
        {
            *this = std::move(__map);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(const std::map<key_type, value_type> &__map, hashT hash = hashT(), hash_resize_policyT resize_policy = hash_resize_policyT()):
            __container_list_base(),
            m_table_size(next_prime(__map.size() * 2)),
            __hash_impl(hash),
            __resize_policy(resize_policy)
        {
            table = new cell *[this->m_table_size];
            memset(table, 0, sizeof(cell *) * this->m_table_size);

            *this = __map;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(size_t m_table_size = init_size, hashT hash = hashT(), hash_resize_policyT resize_policy = hash_resize_policyT()):
            __container_list_base(),
            m_table_size((size_t)next_prime((uint32_t)m_table_size)),
            __hash_impl(hash),
            __resize_policy(resize_policy)
        {
            table = new cell *[this->m_table_size];
            memset(table, 0, sizeof(cell *) * this->m_table_size);
        }

        template <typename iterator_type>
        FUNGUSUTIL_ALWAYS_INLINE inline hash_map(const iterator_type &it_b, const iterator_type &it_e):
            __container_list_base(),
            m_table_size((size_t)next_prime((uint32_t)init_size)),
            __hash_impl(hashT()),
            __resize_policy(hash_resize_policyT())
        {
            table = new cell *[this->m_table_size];
            memset(table, 0, sizeof(cell *) * this->m_table_size);

            assign(it_b, it_e);
        }

        FUNGUSUTIL_ALWAYS_INLINE virtual inline ~hash_map()
        {
            clear();
            if (table) delete[] table;
        }

        template <typename iterator_type>
        FUNGUSUTIL_ALWAYS_INLINE inline void assign(const iterator_type &it_b, const iterator_type &it_e)
        {
            clear();
            for (auto it = it_b; it != it_e; ++it)
                insert(*it);
        }

        template <typename container_type>
        FUNGUSUTIL_ALWAYS_INLINE inline void assign(const container_type &c)
        {
            clear();
            assign(c.begin(), c.end());
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map &operator =(const hash_map &__map)
        {
            if (&__map != this)
                assign(__map);

            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline hash_map &operator =(hash_map &&__map)
        {
            if (&__map != this)
            {
                clear();
                if (table) delete[] table;

                __base_first    = std::move(__map.__base_first);
                __base_last     = std::move(__map.__base_last);
                __base_n_elems  = std::move(__map.__base_n_elems);

                table        = std::move(__map.table);
                m_table_size = std::move(__map.m_table_size);
                __hash_impl  = std::move(__map.__hash_impl);
                __resize_policy = std::move(__map.__resize_policy);

                __map.__base_first   = nullptr;
                __map.__base_last    = nullptr;
                __map.__base_n_elems = 0;

                __map.table = nullptr;
            }

            return *this;
        }

        template <size_t __init_size, typename __hash_resize_policyT>
        FUNGUSUTIL_ALWAYS_INLINE inline hash_map &operator =(hash_map<hashT, __init_size, __hash_resize_policyT> &&__map)
        {
            if (&__map != this)
            {
                clear();
                if (table) delete[] table;

                __base_first    = std::move(__map.__base_first);
                __base_last     = std::move(__map.__base_last);
                __base_n_elems  = std::move(__map.__base_n_elems);

                table        = std::move(__map.table);
                m_table_size = std::move(__map.m_table_size);
                __hash_impl  = std::move(__map.__hash_impl);
                __resize_policy = std::move(__map.__resize_policy);

                __map.__base_first   = nullptr;
                __map.__base_last    = nullptr;
                __map.__base_n_elems = 0;

                __map.table = nullptr;
            }

            return *this;
        }

        template <typename container_type>
        FUNGUSUTIL_ALWAYS_INLINE inline hash_map &operator =(const container_type &c)
        {
            assign(c.begin(), c.end());
            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline size_t table_size() const
        {
            return m_table_size;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline void clear()
        {
            for (auto &it: *this)
                __hash_impl.hash_entry_force_remove(it.key, it.value, it.data);

            __container_list_base::clear();

            if (table)
                memset(table, 0, sizeof(cell *) * m_table_size);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator begin()
        {
            return __base_first ? iterator(static_cast<cell *>(__base_first)) : iterator();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator begin() const
        {
            return __base_first ? const_iterator(static_cast<cell *>(__base_first)) : const_iterator();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator end()
        {
            return iterator();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator end() const
        {
            return const_iterator();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator insert(const std::pair<key_type, value_type> &pair)
        {
            return insert(entry(pair.first, pair.second));
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator insert(const entry &__entry)
        {
            return insert(std::move(entry(__entry)));
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator insert(entry &&__entry)
        {
            hash_entry_action action = hash_entry_no_action;
            cell_manip m(table[__hash_impl.hash_key(__entry.key, m_table_size)]);
            cell *__cell = nullptr;

            if (m.find(__entry.key))
            {
                action = __hash_impl.hash_entry_on_replace(__entry.key, m.__cell->content.value, __entry.value, m.__cell->content.data);
                if (action == hash_entry_complete_action)
                {
                    __cell = m.__cell;
                    __cell->content.value = std::move(__entry.value);
                }
            }
            else
            {
                action = __hash_impl.hash_entry_on_insert(__entry.key, __entry.value, __entry.data);
                if (action == hash_entry_complete_action)
                {
                    __cell = new cell(std::move(__entry), this);
                    m.insert(__cell);
                }
            }

            if (action == hash_entry_complete_action)
            {
                size_t m_new_table_size = __resize_policy.resize_to(m_table_size, size());
                if (m_new_table_size > 0)
                    rehash(m_new_table_size);

                return iterator(__cell);
            }
            else
                return end();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool erase(const key_type &key)
        {
            hash_entry_action action = hash_entry_no_action;
            cell_manip m(table[__hash_impl.hash_key(key, m_table_size)]);

            if (m.find(key))
            {
                cell *__cell = m.__cell;
                action = __hash_impl.hash_entry_on_remove(key, __cell->content.value, __cell->content.data);
                if (action == hash_entry_complete_action)
                {
                    if (m.remove())
                        delete __cell;
                    else
                        action = hash_entry_no_action;
                }
            }
            m.reset();

            if (action == hash_entry_complete_action)
            {
                size_t m_new_table_size = __resize_policy.resize_to(m_table_size, size());
                if (m_new_table_size > 0)
                    rehash(m_new_table_size);

                return true;
            }
            else
                return false;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator erase(const iterator &it)
        {
            if (it.__cell)
            {
                iterator jt = it; ++jt;
                if (erase(it.__cell->content.key))
                    return jt;
                else
                    return it;
            }
            else
                return end();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator find(const key_type &key)
        {
            cell_manip m(table[__hash_impl.hash_key(key, m_table_size)]);
            return m.find(key) ? iterator(m.__cell) : end();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator find(const key_type &key) const
        {
            cell_manip m(table[__hash_impl.hash_key(key, m_table_size)]);
            return m.find(key) ? const_iterator(m.__cell) : end();
        }

        FUNGUSUTIL_ALWAYS_INLINE inline void rehash(size_t _new_table_size)
        {
            const size_t new_table_size = next_prime(_new_table_size);

            cell **new_table = new cell *[new_table_size];
            memset(new_table, 0, sizeof(cell *) * new_table_size);

            for (auto it = begin(); it != end(); ++it)
            {
                cell *__cell = it.__cell;

                __cell->chain_next = nullptr;
                cell_manip m(new_table[__hash_impl.hash_key(__cell->content.key, new_table_size)]);
                m.insert(__cell);
            }

            delete[] table;

            table        = new_table;
            m_table_size = new_table_size;
        }
    };

    template <typename hashT, size_t init_size, typename __hash_resize_policyT>
    class hash_map<hashT, init_size, __hash_resize_policyT>::iterator:
        public std::iterator<std::forward_iterator_tag, entry>
    {
    private:
        bool is_iterator;

        mutable cell *__cell;

        FUNGUSUTIL_ALWAYS_INLINE inline iterator(cell *__cell):       is_iterator(true), __cell(__cell) {}

        friend class hash_map<hashT, init_size>;
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline iterator():                   is_iterator(false),          __cell(nullptr) {}
        FUNGUSUTIL_ALWAYS_INLINE inline iterator(const iterator &it): is_iterator(it.is_iterator), __cell(it.__cell) {}

        FUNGUSUTIL_ALWAYS_INLINE inline iterator &operator =(const iterator &it)
        {
            this->is_iterator = it.is_iterator;
            this->__cell      = it.__cell;

            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline entry &operator *() const
        {
            fungus_util_assert(__cell, "Attempted to dereference a hash_map iterator that was empty!\n");
            return __cell->content;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline entry *operator ->() const
        {
            return &(this->operator *());
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator &operator ++()
        {
            if (__cell) __cell = static_cast<cell *>(__cell->__base_next);
            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline iterator operator ++(int)
        {
            iterator temp = *this;
            if (__cell) __cell = static_cast<cell *>(__cell->__base_next);
            return temp;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool operator ==(const iterator &it) const
        {
            return __cell == it.__cell;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool operator !=(const iterator &it) const
        {
            return __cell != it.__cell;
        }
    };

    template <typename hashT, size_t init_size, typename __hash_resize_policyT>
    class hash_map<hashT, init_size, __hash_resize_policyT>::const_iterator:
        public std::iterator<std::forward_iterator_tag, const entry>
    {
    private:
        bool is_iterator;

        cell *__cell;

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator(cell *__cell):             is_iterator(true), __cell(__cell) {}

        friend class hash_map<hashT, init_size>;
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator():                         is_iterator(false),          __cell(nullptr) {}
        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator(const iterator &it):       is_iterator(it.is_iterator), __cell(it.__cell) {}
        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator(const const_iterator &it): is_iterator(it.is_iterator), __cell(it.__cell) {}

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator &operator =(const iterator &it)
        {
            this->is_iterator = it.is_iterator;
            this->__cell      = it.__cell;

            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator &operator =(const const_iterator &it)
        {
            this->is_iterator = it.is_iterator;
            this->__cell      = it.__cell;

            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const entry &operator *() const
        {
            fungus_util_assert(__cell, "Attempted to dereference a hash_map iterator that was empty!\n");
            return __cell->content;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const entry *operator ->() const
        {
            return &(this->operator *());
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator &operator ++()
        {
            if (__cell) __cell = static_cast<cell *>(__cell->__base_next);
            return *this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline const_iterator operator ++(int)
        {
            iterator temp = *this;
            if (__cell) __cell = static_cast<cell *>(__cell->__base_next);
            return temp;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool operator ==(const const_iterator &it) const
        {
            return __cell == it.__cell;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool operator !=(const const_iterator &it) const
        {
            return __cell != it.__cell;
        }
    };
}

#endif
#endif
