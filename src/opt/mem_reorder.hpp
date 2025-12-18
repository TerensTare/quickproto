
#pragma once

#include "builder.hpp"

// TODO: do you need the whole builder here or just the registry?
// TODO: run memory reordering, then DCE
// TODO: can/should this run when the edge is constructed?

inline void memory_reorder(builder &bld) noexcept
{
    auto &&mem_chain = bld.reg.storage<mem_effect>();
    auto const &reads = bld.reg.storage<mem_read>();
    auto const &writes = bld.reg.storage<mem_write>();

    for (auto &&[id, mem] : mem_chain.each())
    {
        auto earliest_dep = mem.prev;

        while (mem_chain.contains(earliest_dep))
        {
            auto &&prev = mem_chain.get(earliest_dep);

            if (writes.contains(earliest_dep) &&
                // TODO: correctly check for offset equality (include -1 check)
                ((mem.target != prev.target) || (mem.tag != prev.tag)))
            {
                // Reads/writes can happen in parallel with writes as long as they target different places or offsets
                earliest_dep = prev.prev;
            }
            else
            {
                break;
            }

            // TODO: more optimizations
        }

        mem.prev = earliest_dep;
    }

    for (auto &&[id, mem] : entt::basic_view{mem_chain, reads}.each())
    {
        auto earliest_dep = mem.prev;

        while (mem_chain.contains(earliest_dep))
        {
            auto &&prev = mem_chain.get(earliest_dep);

            if (reads.contains(earliest_dep))
            {
                // Read operations can happen in parallel
                earliest_dep = prev.prev;
            }
            else
            {
                break;
            }

            // TODO: more optimizations
        }

        mem.prev = earliest_dep;
    }
}