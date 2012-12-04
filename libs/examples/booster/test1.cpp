#include "fungus_booster/fungus_booster.h"

using namespace fungus_util;

class my_multi_tree_node: public public_multi_tree_node
{
public:
    int id;

    my_multi_tree_node(multi_tree_node_type type, int id):
        public_multi_tree_node(type), id(id) {}
};

typedef hash_map<default_hash_no_replace<int, my_multi_tree_node *>> node_hash_map;

static int id_ctr = 0;
node_hash_map nodes;

int main(int argc, char *argv[])
{
    std::cout << "commands:\n"
                 "  make   [group|leaf]              : make a new group node or make a new\n"
                 "                                     leaf node.\n"
                 "  add    [node_id] to   [group_id] : add a node to a group node.\n"
                 "  remove [node_id] from [group_id] : remove a node from a group node.\n"
                 "  children of [node_id]            : display a list of all children for\n"
                 "                                     that node.\n"
                 "  leaves of [node_id]              : display a list of all leaves for that\n"
                 "                                     node.\n"
                 "  groups of [node_id]              : display a list of all groups that node\n"
                 "                                     belongs to.\n"
                 "  quit                             : exit program.\n"
              << std::endl;

    for (;;)
    {
        std::string cmd;
        std::cin >> cmd;

        if (cmd == "make")
        {
            std::cin >> cmd;
            if (cmd == "group")
            {
                ++id_ctr;

                nodes.insert(node_hash_map::entry(id_ctr,
                             new my_multi_tree_node(multi_tree_node_group, id_ctr)));
                std::cout << "new group node " << id_ctr << std::endl;
            }
            else if (cmd == "leaf")
            {
                ++id_ctr;

                nodes.insert(node_hash_map::entry(id_ctr,
                             new my_multi_tree_node(multi_tree_node_leaf, id_ctr)));
                std::cout << "new leaf node " << id_ctr << std::endl;
            }
            else
                std::cout << "invalid command." << std::endl;
        }
        else if (cmd == "add")
        {
            int group_id, node_id;
            std::string to;

            std::cin >> node_id >> to >> group_id;

            auto group_it = nodes.find(group_id);
            auto node_it  = nodes.find(node_id);

            if (to != "to")
                std::cout << "syntax error." << std::endl;

            if (group_it == nodes.end())
            {
                std::cout << "group node " << group_id << " does not exist." << std::endl;
                continue;
            }

            if (node_it == nodes.end())
            {
                std::cout << "child node " << node_id << " does not exist." << std::endl;
                continue;
            }

            bool success = group_it->value->add_node(node_it->value);

            if (success)
                std::cout << "node " << node_id
                          << " is now a child of group node "
                          << group_id << '.' << std::endl;
            else
                std::cout << "could not add node " << node_id
                          << " as a child to group node "
                          << group_id << '.' << std::endl;
        }
        else if (cmd == "remove")
        {
            int group_id, node_id;
            std::string from;

            std::cin >> node_id >> from >> group_id;

            auto group_it = nodes.find(group_id);
            auto node_it  = nodes.find(node_id);

            if (from != "from")
                std::cout << "syntax error." << std::endl;

            if (group_it == nodes.end())
            {
                std::cout << "group node " << group_id << " does not exist." << std::endl;
                continue;
            }

            if (node_it == nodes.end())
            {
                std::cout << "child node " << node_id << " does not exist." << std::endl;
                continue;
            }

            bool success = group_it->value->remove_node(node_it->value);

            if (success)
                std::cout << "node " << node_id
                          << " is no longer a child of group node "
                          << group_id << '.' << std::endl;
            else
                std::cout << "could not remove node " << node_id
                          << " as a child from group node "
                          << group_id << '.' << std::endl;
        }
        else if (cmd == "children")
        {
            int node_id;
            std::string of;

            std::cin >> of >> node_id;

            if (of != "of")
            {
                std::cout << "syntax error." << std::endl;
                continue;
            }

            auto node_it = nodes.find(node_id);
            if (node_it == nodes.end())
            {
                std::cout << "node " << node_id << " does not exist." << std::endl;
                continue;
            }

            std::cout << "node " << node_id << " has children:" << std::endl << "  ";
            auto it_factory = node_it->value->iterator_factory<multi_tree_iterator_target::nodes>();
            for (auto &entry: it_factory)
                std::cout << static_cast<my_multi_tree_node *>(entry.key)->id << ' ';

            std::cout << std::endl;
        }
        else if (cmd == "leaves")
        {
            int node_id;
            std::string of;

            std::cin >> of >> node_id;

            if (of != "of")
            {
                std::cout << "syntax error." << std::endl;
                continue;
            }

            auto node_it = nodes.find(node_id);
            if (node_it == nodes.end())
            {
                std::cout << "node " << node_id << " does not exist." << std::endl;
                continue;
            }

            std::cout << "node " << node_id << " has leaves:" << std::endl << "  ";
            auto it_factory = node_it->value->iterator_factory<multi_tree_iterator_target::leaves>();
            for (auto &entry: it_factory)
                std::cout << static_cast<my_multi_tree_node *>(entry.key)->id << ' ';

            std::cout << std::endl;
        }
        else if (cmd == "groups")
        {
            int node_id;
            std::string of;

            std::cin >> of >> node_id;

            if (of != "of")
            {
                std::cout << "syntax error." << std::endl;
                continue;
            }

            auto node_it = nodes.find(node_id);
            if (node_it == nodes.end())
            {
                std::cout << "node " << node_id << " does not exist." << std::endl;
                continue;
            }

            std::cout << "node " << node_id << " is a member of groups:" << std::endl << "  ";
            auto it_factory = node_it->value->iterator_factory<multi_tree_iterator_target::groups>();
            for (auto &entry: it_factory)
                std::cout << static_cast<my_multi_tree_node *>(entry.key)->id << ' ';

            std::cout << std::endl;
        }
        else if (cmd == "quit")
            break;
        else
            std::cout << "invalid command." << std::endl;
    }

    nodes.clear();
    return 0;
}
