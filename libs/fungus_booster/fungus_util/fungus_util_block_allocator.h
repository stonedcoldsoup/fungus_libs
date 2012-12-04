#ifndef FUNGUSUTIL_BLOCK_ALLOCATOR_H
#define FUNGUSUTIL_BLOCK_ALLOCATOR_H

#include "fungus_util_common.h"
#include "fungus_util_constexpr.h"
#include "fungus_util_hash_map.h"

namespace fungus_util
{
#ifndef FUNGUSUTIL_NO_BLOCK_ALLOCATOR
    template <typename blockT, size_t n_blocks>
    class block_allocator
    {
    private:
        struct block_set;
        struct block_t
        {
            typename std::aligned_storage
                <sizeof(blockT), std::alignment_of<blockT>::value>::type
                storage;

            block_set *m_set;
        };

        fungus_util_constexpr_assert(n_blocks > 0, n_blocks_check);
        static constexpr size_t block_tize = sizeof(block_t);

        block_set **m_block_sets;
        size_t n_block_sets;
        size_t n_alloced;

        struct _s_nil {};
        typedef hash_map<default_hash<block_set *, _s_nil>> set_map;
        set_map m_set_map;

        FUNGUSUTIL_ALWAYS_INLINE inline void expand()
        {
            size_t n_block_sets_new = n_block_sets << 1;
            block_set **m_block_sets_new = new block_set *[n_block_sets_new];

            for (size_t i = 0; i < n_block_sets; ++i)
                m_block_sets_new[i] = m_block_sets[i];

            if (n_block_sets_new > m_set_map.table_size() / 3 * 2)
                m_set_map.rehash(n_block_sets_new * 2);

            for (size_t i = n_block_sets; i < n_block_sets_new; ++i)
            {
                m_block_sets_new[i] = new block_set();
                m_set_map.insert(std::move(typename set_map::entry(m_block_sets_new[i], _s_nil())));
            }

            delete[] m_block_sets;

            m_block_sets = m_block_sets_new;
            n_block_sets = n_block_sets_new;
        }

        FUNGUSUTIL_NO_ASSIGN(block_allocator);
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline block_allocator(block_allocator &&m_block_allocator):
            m_block_sets(m_block_allocator.m_block_sets),
            n_block_sets(m_block_allocator.n_block_sets),
            n_alloced(m_block_allocator.n_alloced),
            m_set_map(std::move(m_block_allocator.m_set_map))
        {
            m_block_allocator.m_block_sets = nullptr;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline block_allocator(size_t n_block_sets = 1):
            m_block_sets(nullptr),
            n_block_sets(n_block_sets == 0 ? 1 : n_block_sets),
            n_alloced(0),
            m_set_map(n_block_sets * 2)
        {
            m_block_sets = new block_set *[n_block_sets];

            for (size_t i = 0; i < n_block_sets; ++i)
            {
                m_block_sets[i] = new block_set();
                m_set_map.insert(std::move(typename set_map::entry(m_block_sets[i], _s_nil())));
            }
        }

        FUNGUSUTIL_ALWAYS_INLINE inline ~block_allocator()
        {
            if (m_block_sets)
            {
                for (size_t i = 0; i < n_block_sets; ++i)
                    delete m_block_sets[i];

                delete[] m_block_sets;
            }
        }

        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_block_sets() const {return n_block_sets;}
        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_blocks()     const {return n_block_sets * n_blocks;}
        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_alloced()    const {return n_alloced;}

        template <typename... argT>
        FUNGUSUTIL_ALWAYS_INLINE inline blockT *create(argT&&... argV)
        {
            fungus_util_assert(m_block_sets, "fungus_util::block_allocator::create(): this instance has been moved!");

            if (++n_alloced > count_blocks())
                expand();

            //for (size_t i = 0; i < n_block_sets && p == nullptr; ++i)
            unsigned char *p = nullptr;
            block_set *m_set = nullptr;

            if (n_block_sets > 1)
            {
                auto it = m_set_map.begin();
                if (it != m_set_map.end())
                    m_set = it->key;
            }
            else
                m_set = m_block_sets[0];

            p = m_set->alloc();
            if (m_set->n_alloced >= n_blocks)
                m_set_map.erase(m_set);

            if (p)
            {
                blockT *b = new(p) blockT(std::forward<argT>(argV)...);
                return b;
            }
            else
                return nullptr;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool destroy(blockT *b)
        {
            fungus_util_assert(m_block_sets, "fungus_util::block_allocator::destroy(): this instance has been moved!");

            unsigned char *p = (unsigned char *)b;

            block_t *p_b = block_set::get_from_p(p);
            if (p_b->m_set->n_alloced > 0 && p_b->m_set->dealloc(p))
            {
                if (n_block_sets > 1)
                {
                    if (m_set_map.find(p_b->m_set) == m_set_map.end())
                        m_set_map.insert(std::move(typename set_map::entry(p_b->m_set, _s_nil())));
                }

                b->~blockT();
                --n_alloced;

                return true;
            }
            else
                return false;
        }
    };

    template <typename blockT, size_t n_blocks>
    struct block_allocator<blockT, n_blocks>::block_set
    {
        size_t n_alloced;

        block_t   m_blocks[n_blocks];
        block_t  *m_free_stack[n_blocks];
        block_t **p_free_stack;

        block_set(block_set      &&m_set) = delete;
        block_set(const block_set &m_set) = delete;

        FUNGUSUTIL_ALWAYS_INLINE inline block_set():
            n_alloced(0)
        {
            for (size_t i = 0; i < n_blocks; ++i)
                m_free_stack[i] = m_blocks + i;

            p_free_stack = m_free_stack + n_blocks;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline ~block_set() {}

        FUNGUSUTIL_ALWAYS_INLINE inline unsigned char *alloc()
        {
            if (n_alloced < n_blocks)
            {
                ++n_alloced;
                block_t *p = *--p_free_stack;
                p->m_set = this;

                return p->storage.__data;
            }
            else
                return nullptr;
        }

        FUNGUSUTIL_ALWAYS_INLINE static inline block_t *get_from_p(unsigned char *p)
        {
            return static_cast<block_t *>(static_cast<void *>(p));
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool dealloc(unsigned char *p)
        {
            block_t *p_b = get_from_p(p);

            bool success = (p_b->m_set == this)         &&
                           (p_b >= m_blocks)            &&
                           (p_b <  m_blocks + n_blocks) &&
                           (n_alloced > 0);

            if (success)
            {
                --n_alloced;
                *p_free_stack++ = p_b;
            }

            return success;
        }
    };
#else
    template <typename blockT, size_t n_blocks>
    class block_allocator
    {
    private:
        bool   b_moved;
        size_t n_alloced;

        FUNGUSUTIL_NO_ASSIGN(block_allocator);
    public:
        FUNGUSUTIL_ALWAYS_INLINE inline block_allocator(block_allocator &&m_block_allocator):
            b_moved(false), n_alloced(m_block_allocator.n_alloced)
        {
            m_block_allocator.b_moved = true;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline block_allocator(size_t n_block_sets = 1):
            b_moved(false), n_alloced(0)
        {}

        FUNGUSUTIL_ALWAYS_INLINE inline ~block_allocator()
        {}

        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_block_sets() const {return 1;}
        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_blocks()     const {return UINT32_MAX;}
        FUNGUSUTIL_ALWAYS_INLINE inline size_t count_alloced()    const {return n_alloced;}

        template <typename... argT>
        FUNGUSUTIL_ALWAYS_INLINE inline blockT *create(argT&&... argV)
        {
            fungus_util_assert(!b_moved, "fungus_util::block_allocator::create(): this instance has been moved!");

            blockT *b = new(std::nothrow) blockT(std::forward<argT>(argV)...);
            if (b)
                ++n_alloced;

            return b;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool destroy(blockT *b)
        {
            fungus_util_assert(!b_moved, "fungus_util::block_allocator::destroy(): this instance has been moved!");

            delete b;
            --n_alloced;

            return true;
        }
    };
#endif

    template <typename keyT, typename valueT, typename block_allocatorT,
              hash_entry_ptr_deletion_policy _delete_policy = hash_entry_ptr_delete>
    class block_allocator_object_hash:
        public default_hash_no_replace<keyT, valueT *, _delete_policy>
    {
    private:
        block_allocatorT &m_allocator;
    public:
        typedef typename default_hash_no_replace<keyT, valueT *>::data_type data_type;

        block_allocator_object_hash(block_allocatorT &m_allocator):
            m_allocator(m_allocator) {}

        inline hash_entry_action hash_entry_on_remove(const keyT &key,
                                                      valueT     *value,
                                                      data_type  &data)
        {
            m_allocator.destroy(value);
            return hash_entry_complete_action;
        }

        inline void hash_entry_force_remove(const keyT &key,
                                            valueT     *value,
                                            data_type  &data)
        {
            m_allocator.destroy(value);
        }
    };
}

#endif
