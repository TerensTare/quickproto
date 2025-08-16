
#pragma once

#include "builder.hpp"

inline void cut_user_links(builder &b, node *n)
{
    auto const nid = n->id;

    auto const n_ins = n->inputs();
    auto const n_ins_cnt = n->in_size;

    for (size_t i{}; i < n_ins_cnt; ++i)
    {
        auto &&users = b.users[n_ins[i]->id];
        auto const users_cnt = users.size();

        for (size_t j{}; j < users_cnt - 1; ++j)
        {
            if (users[j]->id == nid)
            {
                users[j] = users.back();
                break;
            }
        }

        users.pop_back();
    }
}

inline void prune_dead_code(builder &b)
{
    bool done = false;
    while (!done)
    {
        done = true;

        for (size_t i{}; i < b.nodes.size() - 1; ++i)
        {
            auto node = b.nodes[i];

            if (b.users[node->id].size() == 0 && node->op != op::Return)
            {
                cut_user_links(b, node);

                b.nodes[i] = b.nodes.back();
                b.nodes.pop_back();

                done = false;
            }
        }
    }
}