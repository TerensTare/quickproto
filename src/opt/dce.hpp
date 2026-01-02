
#pragma once

#include "builder.hpp"

// TODO: this again, but for global nodes
inline void prune_dead_code(builder &bld, entt::entity ret) noexcept
{
    // TODO: should this be here or somewhere else?
    // bld.report_errors();

    // auto &&new_nodes = bld.reg.storage<void>((entt::id_type)visibility::maybe_reachable);
    // auto &&visited = bld.reg.storage<void>((entt::id_type)visibility::reachable);
    // auto &&globals = bld.reg.storage<void>((entt::id_type)visibility::global);

    // auto const &ins_storage = bld.reg.storage<node_inputs>();
    // auto const &ctrl = bld.reg.storage<ctrl_effect>();
    // auto const &mem = bld.reg.storage<mem_effect>();

    // // TODO: cache this storage because why not
    // std::vector<entt::entity> to_visit;
    // to_visit.push_back(ret);

    // while (!to_visit.empty())
    // {
    //     auto const top = to_visit.back();
    //     to_visit.pop_back();

    //     // TODO: is this ever the case that this node was visited previously?
    //     if (visited.contains(top))
    //         continue;

    //     // TODO: add other links here as well
    //     // TODO: maybe just follow the effect and memory chain instead?
    //     // ^ also you can check here `visited` so that you don't push into the vector, causing a possible reallocation
    //     // ^ lemma: if there is only one candidate node to add to `to_visit`, you don't need `to_visit` at all
    //     auto &&ins = ins_storage.get(top).nodes;
    //     to_visit.insert(to_visit.end(), ins.begin(), ins.end());

    //     if (ctrl.contains(top))
    //         to_visit.push_back(ctrl.get(top).target);

    //     // TODO: is this correct?
    //     // TODO: recheck now that `mem_effect` has different members
    //     if (mem.contains(top))
    //     {
    //         to_visit.push_back(mem.get(top).target);
    //         to_visit.push_back(mem.get(top).prev);
    //     }

    //     visited.emplace(top);
    // }

    // // TODO: these are unreachable, so maybe leave a warning for significant nodes? (functions, etc.)
    // {
    //     // auto to_cut_view = entt::basic_view{std::tie(new_nodes), std::tie(visited)};
    //     // TODO: is this correct?
    //     auto to_cut_view = entt::basic_view{std::tie(bld.reg.storage<entt::entity>()), std::tie(visited, globals)};
    //     bld.reg.destroy(to_cut_view.begin(), to_cut_view.end());
    // }

    // // TODO: is this correct?
    // {
    //     auto &&to_cut = bld.reg.storage<void>((entt::id_type)visibility::unreachable);
    //     bld.reg.destroy(to_cut.begin(), to_cut.end());
    // }

    // new_nodes.clear();
}
