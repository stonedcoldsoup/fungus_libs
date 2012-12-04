#ifndef FUNGUSUTIL_MULTI_TREE_H
#define FUNGUSUTIL_MULTI_TREE_H

#include "fungus_util_common.h"
#ifdef FUNGUSUTIL_CPP11_PARTIAL

#include "fungus_util_hash_map.h"

namespace fungus_util
{
    enum multi_tree_node_type
    {
        multi_tree_node_group,
        multi_tree_node_leaf
    };

    enum class multi_tree_iterator_target
    {
        groups, nodes, leaves
    };

    class public_multi_tree_node;

    template <class multi_tree_nodeT>
    class multi_tree_node
    {
    protected:
        struct _s_nil {};

        typedef multi_tree_node<multi_tree_nodeT>                       multi_tree_base_type;

        typedef multi_tree_nodeT                                        node_type;
        typedef hash_map<default_hash_no_replace<node_type *, _s_nil>>  node_hash_map;
        typedef hash_map<ref_ctr_hash<node_type *, _s_nil>>             leaf_hash_map;

        typedef typename node_hash_map::iterator                        node_iterator;
        typedef typename node_hash_map::const_iterator                  node_const_iterator;
        typedef typename node_hash_map::entry                           node_hash_map_entry;

        typedef typename leaf_hash_map::iterator                        leaf_iterator;
        typedef typename leaf_hash_map::const_iterator                  leaf_const_iterator;
        typedef typename leaf_hash_map::entry                           leaf_hash_map_entry;

    private:
        multi_tree_node_type type;
        node_type *self;

        node_hash_map groups;
        node_hash_map nodes;
        leaf_hash_map leaves;

        inline void add_leaves(const leaf_hash_map &m_map)
        {
            for (const auto &entry: m_map)
                leaves.insert(entry);

            for (auto &entry: groups)
                entry.key->add_leaves(m_map);
        }

        inline void remove_leaves(const leaf_hash_map &m_map)
        {
            for (const auto &entry: m_map)
                leaves.erase(entry.key);

            for (auto &entry: groups)
                entry.key->remove_leaves(m_map);
        }

        friend class public_multi_tree_node;

        template <typename multi_tree_nodeU, multi_tree_iterator_target _targ>
        friend class multi_tree_iterator_factory;
    protected:
        multi_tree_node(node_type *self, multi_tree_node_type type):
            type(type), self(self), groups(), nodes(), leaves()
        {
            fungus_util_assert(self, "multi_tree_node: self is invalid!");

            if (type == multi_tree_node_leaf)
                leaves.insert(node_hash_map_entry(self, _s_nil()));
        }

        virtual ~multi_tree_node()
        {
            node_hash_map __groups(groups);
            node_hash_map __nodes(nodes);

            if (type == multi_tree_node_group)
            {
                for (auto &entry: __nodes)
                    remove_node(entry.key);
            }

            for (auto &entry: __groups)
                entry.key->remove_node(self);
        }

        virtual bool add_node(node_type *node)
        {
            if (type == multi_tree_node_group && node)
            {
                multi_tree_node *base_node = dynamic_cast<multi_tree_node *>(node);

                if (!base_node)
                    return false;

                if (base_node->groups.insert(node_hash_map_entry(self, _s_nil())) != base_node->groups.end() &&
                    nodes.insert(node_hash_map_entry(node, _s_nil()))             != nodes.end())
                {
                    add_leaves(base_node->leaves);

                    return true;
                }
                else
                {
                    base_node->groups.erase(self);
                    nodes.erase(node);
                }
            }

            return false;
        }

        virtual bool remove_node(node_type *node)
        {
            if (type == multi_tree_node_group && node)
            {
                multi_tree_node *base_node = dynamic_cast<multi_tree_node *>(node);

                if (!base_node)
                    return false;

                auto node_it  = nodes.find(node);
                auto group_it = base_node->groups.find(self);

                if (node_it  != nodes.end() &&
                    group_it != base_node->groups.end())
                {
                    nodes.erase(node_it);
                    base_node->groups.erase(group_it);

                    remove_leaves(base_node->leaves);

                    return true;
                }
            }

            return false;
        }

        virtual bool is_group() const
        {
            return type == multi_tree_node_group;
        }

        virtual bool has_node(const node_type *node) const
        {
            return (type == multi_tree_node_group && node)      ? nodes.find(const_cast<node_type *>(node))  != nodes.end()   : false;
        }

        virtual bool has_leaf(const node_type *leaf) const
        {
            return (leaf && leaf->type == multi_tree_node_leaf) ? leaves.find(const_cast<node_type *>(leaf)) != leaves.end() : false;
        }

        virtual size_t count_groups() const  {return groups.size();}
        virtual size_t count_nodes()  const  {return nodes.size(); }
        virtual size_t count_leaves() const  {return leaves.size();}

        virtual node_iterator begin_groups() {return groups.begin();}
        virtual node_iterator begin_nodes()  {return nodes.begin(); }
        virtual leaf_iterator begin_leaves() {return leaves.begin();}

        virtual node_iterator end_groups()   {return groups.end();}
        virtual node_iterator end_nodes()    {return nodes.end(); }
        virtual leaf_iterator end_leaves()   {return leaves.end();}

        virtual node_const_iterator begin_groups() const {return groups.begin();}
        virtual node_const_iterator begin_nodes()  const {return nodes.begin(); }
        virtual leaf_const_iterator begin_leaves() const {return leaves.begin();}

        virtual node_const_iterator end_groups()   const {return groups.end();}
        virtual node_const_iterator end_nodes()    const {return nodes.end(); }
        virtual leaf_const_iterator end_leaves()   const {return leaves.end();}

        virtual bool groups_empty() const {return groups.empty();}
        virtual bool nodes_empty()  const {return nodes.empty(); }
        virtual bool leaves_empty() const {return leaves.empty();}
    };

    template <class multi_tree_nodeT, multi_tree_iterator_target _targ>
    class multi_tree_iterator_factory;

    template <class multi_tree_nodeT>
    class multi_tree_iterator_factory<multi_tree_nodeT, multi_tree_iterator_target::groups>
    {
    private:
        multi_tree_nodeT &node;
    public:
        typedef typename multi_tree_nodeT::node_iterator       iterator;
        typedef typename multi_tree_nodeT::node_const_iterator const_iterator;

        typedef typename multi_tree_nodeT::node_hash_map       hash_map;
        typedef typename multi_tree_nodeT::node_hash_map_entry hash_map_entry;

        multi_tree_iterator_factory(multi_tree_nodeT &node): node(node)  {}
        multi_tree_iterator_factory(multi_tree_nodeT *node): node(*node) {}

        multi_tree_iterator_factory(const multi_tree_nodeT &node): node(const_cast<multi_tree_nodeT &>(node))  {}
        multi_tree_iterator_factory(const multi_tree_nodeT *node): node(const_cast<multi_tree_nodeT &>(*node)) {}

        iterator       begin()       {return node.begin_groups();}
        const_iterator begin() const {return node.begin_groups();}

        iterator       end()         {return node.end_groups();}
        const_iterator end()   const {return node.end_groups();}
    };

    template <class multi_tree_nodeT>
    class multi_tree_iterator_factory<multi_tree_nodeT, multi_tree_iterator_target::nodes>
    {
    private:
        multi_tree_nodeT &node;
    public:
        typedef typename multi_tree_nodeT::node_iterator       iterator;
        typedef typename multi_tree_nodeT::node_const_iterator const_iterator;

        typedef typename multi_tree_nodeT::node_hash_map       hash_map;
        typedef typename multi_tree_nodeT::node_hash_map_entry hash_map_entry;

        multi_tree_iterator_factory(multi_tree_nodeT &node): node(node)  {}
        multi_tree_iterator_factory(multi_tree_nodeT *node): node(*node) {}

        multi_tree_iterator_factory(const multi_tree_nodeT &node): node(const_cast<multi_tree_nodeT &>(node))  {}
        multi_tree_iterator_factory(const multi_tree_nodeT *node): node(const_cast<multi_tree_nodeT &>(*node)) {}

        iterator       begin()       {return node.begin_nodes();}
        const_iterator begin() const {return node.begin_nodes();}

        iterator       end()         {return node.end_nodes();}
        const_iterator end()   const {return node.end_nodes();}
    };

    template <class multi_tree_nodeT>
    class multi_tree_iterator_factory<multi_tree_nodeT, multi_tree_iterator_target::leaves>
    {
    private:
        multi_tree_nodeT &node;
    public:
        typedef typename multi_tree_nodeT::leaf_iterator       iterator;
        typedef typename multi_tree_nodeT::leaf_const_iterator const_iterator;

        typedef typename multi_tree_nodeT::leaf_hash_map       hash_map;
        typedef typename multi_tree_nodeT::leaf_hash_map_entry hash_map_entry;

        multi_tree_iterator_factory(multi_tree_nodeT &node): node(node)  {}
        multi_tree_iterator_factory(multi_tree_nodeT *node): node(*node) {}

        multi_tree_iterator_factory(const multi_tree_nodeT &node): node(const_cast<multi_tree_nodeT &>(node))  {}
        multi_tree_iterator_factory(const multi_tree_nodeT *node): node(const_cast<multi_tree_nodeT &>(*node)) {}

        iterator       begin()       {return node.begin_leaves();}
        const_iterator begin() const {return node.begin_leaves();}

        iterator       end()         {return node.end_leaves();}
        const_iterator end()   const {return node.end_leaves();}
    };

    template <typename multi_tree_nodeT, multi_tree_iterator_target _targ>
    multi_tree_iterator_factory<multi_tree_nodeT, _targ>
    get_multi_tree_iterator_factory(multi_tree_nodeT &node)
    {
        return multi_tree_iterator_factory<multi_tree_nodeT, _targ>(node);
    }

    template <typename multi_tree_nodeT, multi_tree_iterator_target _targ>
    multi_tree_iterator_factory<multi_tree_nodeT, _targ>
    get_multi_tree_iterator_factory(multi_tree_nodeT *node)
    {
        return multi_tree_iterator_factory<multi_tree_nodeT, _targ>(node);
    }

    template <typename multi_tree_nodeT, multi_tree_iterator_target _targ>
    multi_tree_iterator_factory<multi_tree_nodeT, _targ>
    get_multi_tree_iterator_factory(const multi_tree_nodeT &node)
    {
        return multi_tree_iterator_factory<multi_tree_nodeT, _targ>(node);
    }

    template <typename multi_tree_nodeT, multi_tree_iterator_target _targ>
    multi_tree_iterator_factory<multi_tree_nodeT, _targ>
    get_multi_tree_iterator_factory(const multi_tree_nodeT *node)
    {
        return multi_tree_iterator_factory<multi_tree_nodeT, _targ>(node);
    }

    class public_multi_tree_node: public multi_tree_node<public_multi_tree_node>
    {
    public:
        typedef multi_tree_node<public_multi_tree_node>     multi_tree_base_type;

        typedef multi_tree_base_type::node_type             node_type;
        typedef multi_tree_base_type::node_hash_map         node_hash_map;
        typedef multi_tree_base_type::leaf_hash_map         leaf_hash_map;

        typedef multi_tree_base_type::node_iterator         node_iterator;
        typedef multi_tree_base_type::node_const_iterator   node_const_iterator;
        typedef multi_tree_base_type::node_hash_map_entry   node_hash_map_entry;

        typedef multi_tree_base_type::leaf_iterator         leaf_iterator;
        typedef multi_tree_base_type::leaf_const_iterator   leaf_const_iterator;
        typedef multi_tree_base_type::leaf_hash_map_entry   leaf_hash_map_entry;

        FUNGUSUTIL_ALWAYS_INLINE inline
        public_multi_tree_node(multi_tree_node_type type):
            multi_tree_base_type(this, type)
        {}

        virtual ~public_multi_tree_node() {}

        virtual bool add_node(node_type *node)             {return multi_tree_base_type::add_node(node);}
        virtual bool remove_node(node_type *node)          {return multi_tree_base_type::remove_node(node);}

        virtual bool is_group() const                      {return multi_tree_base_type::is_group();}
        virtual bool has_node(const node_type *node) const {return multi_tree_base_type::has_node(node);}
        virtual bool has_leaf(const node_type *leaf) const {return multi_tree_base_type::has_leaf(leaf);}

        virtual size_t count_groups() const  {return groups.size();}
        virtual size_t count_nodes()  const  {return nodes.size(); }
        virtual size_t count_leaves() const  {return leaves.size();}

        virtual node_iterator begin_groups() {return groups.begin();}
        virtual node_iterator begin_nodes()  {return nodes.begin(); }
        virtual leaf_iterator begin_leaves() {return leaves.begin();}

        virtual node_iterator end_groups()   {return groups.end();}
        virtual node_iterator end_nodes()    {return nodes.end(); }
        virtual leaf_iterator end_leaves()   {return leaves.end();}

        virtual node_const_iterator begin_groups() const {return groups.begin();}
        virtual node_const_iterator begin_nodes()  const {return nodes.begin(); }
        virtual leaf_const_iterator begin_leaves() const {return leaves.begin();}

        virtual node_const_iterator end_groups()   const {return groups.end();}
        virtual node_const_iterator end_nodes()    const {return nodes.end(); }
        virtual leaf_const_iterator end_leaves()   const {return leaves.end();}

        virtual bool groups_empty() const {return groups.empty();}
        virtual bool nodes_empty()  const {return nodes.empty(); }
        virtual bool leaves_empty() const {return leaves.empty();}

        template <multi_tree_iterator_target _targ>
        multi_tree_iterator_factory<public_multi_tree_node, _targ>
        iterator_factory()
        {
            return get_multi_tree_iterator_factory<public_multi_tree_node, _targ>(this);
        }

        template <multi_tree_iterator_target _targ>
        const multi_tree_iterator_factory<public_multi_tree_node, _targ>
        iterator_factory() const
        {
            return get_multi_tree_iterator_factory<public_multi_tree_node, _targ>(this);
        }
    };
}

#endif
#endif
