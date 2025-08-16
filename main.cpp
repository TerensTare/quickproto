
#include "parser.hpp"

// TODO:
// - visual debugger with tunable steppable vs non-steppable coroutines for compiler passes
// - DLL plugins for compilation phases
// - builder should have a default block; the global one
// - lemma: now that you have memory ordering; any operation that occurs in a `Store` before being connected to the main `State` is const-foldable (but not the only case)

int main()
{
    auto text =
        R"(
func main() {
    a := (123 - 456 * 2 * -1) + 4;
    b := 20 + 10;
    b++;
    a -= b;
    return b * 3;
})";

    auto p = parser{
        .scan{.text = text},
    };

    p.prog();

    auto g = p.bld.freeze();

    auto f = fopen("out.dot", "w");
    ensure(f, "Cannot open `out.dot`!");

    g.export_dot(f);

    return 0;
}