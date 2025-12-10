
#include <chrono>
#include <memory>

#include "backends/dot.hpp"
#include "parser/all.hpp"

// TODO: parallelize DCE function calls, spawning them in a background thread each time a function is parsed
// ^ how does this play out with "no forward declarations?"

// TODO:
// - visual debugger with tunable steppable vs non-steppable coroutines for compiler passes
// - DLL plugins for compilation phases
// - lemma: now that you have memory ordering; any operation that occurs in a `Store` before being connected to the main `State` is const-foldable (but not the only case)
// - more build flags:
// ^ option to specify name of passes to run; also presets with certain passes such as opt(1), opt(2), debug or profile
// ^ deduce backend based on `-o <out-name>.<ext>`

// DONE:
// - peephole nodes during parsing, not after function parsed
// ^ because peephole only affects a node and its children
// ^ so add overload to `builder.make` that peepholes for you

struct cmd_args final
{
    inline static cmd_args parse(int argc, char **argv) noexcept;

    char const *in_path;
    char const *out_path;
    bool opt;
};

struct file_deleter final
{
    inline static void operator()(FILE *f) noexcept { fclose(f); }
};

using file_ptr = std::unique_ptr<FILE, file_deleter>;

inline size_t read_file(char const *path, char **out) noexcept;

int main(int argc, char **argv)
{
    auto args = cmd_args::parse(argc, argv);

    std::unique_ptr<char> text;
    auto n = read_file(args.in_path, std::out_ptr(text));
    ensure(n != -1, "Cannot read input file!");

    auto const start = std::chrono::system_clock::now();

    auto p = parser{
        .scan{.text = text.get()},
    };

    p.package();

    auto f = fopen(args.out_path, "w");
    ensure(f, "Cannot open output file!");

    // TODO: call this from somewhere else
    memory_reorder(p.bld);
    // TODO: run DCE after memory reordering

    dot_backend dot;
    dot.compile(f, p.bld.reg);

    auto const finish = std::chrono::system_clock::now();
    std::println("Compiling {} took {:%T}", args.in_path, finish - start);

    fclose(f);

    return 0;
}

inline cmd_args cmd_args::parse(int argc, char **argv) noexcept
{
    if (argc == 1)
    {
        std::string_view name{argv[0]};
        std::make_signed_t<size_t> name_start = name.length();
        while (--name_start > -1)
        {
            if (name[name_start] == '\\' or name[name_start] == '/')
            {
                ++name_start;
                break;
            }
        }

        std::println(
            "Usage:\n"
            "\t{} <file-name> [-o <out-name>]\n\n"
            "Where:\n"
            "\t<file-name> - name of file to compile\n"
            "\t<out-name> - name of the output file to produce (default to out.dot)",
            name.substr(name_start) //
        );

        // std::exit(0);
    }

    auto in_path = "sample.qp";
    // char const *in_path = argv[1];
    char const *out_path = "out.dot";
    bool opt = false;

    for (int i = 2; i < argc;)
    {
        if (strcmp(argv[i], "-o"))
        {
            ++i;
            ensure(i < argc, "Expected file name after `-o` parameter");

            out_path = argv[i++];
        }
        else if (strcmp(argv[i], "-O"))
        {
            ++i;
            opt = true;
        }
        // TODO: if not a flag, it should be a source
    }

    return cmd_args{
        .in_path = in_path,
        .out_path = out_path,
        .opt = opt,
    };
}

inline size_t read_file(char const *path, char **out) noexcept
{
    file_ptr _file{fopen(path, "r")};
    auto f = _file.get();

    if (!f)
        return size_t(-1);

    fseek(f, 0, SEEK_END);
    size_t n = ftell(f);
    *out = (char *)::operator new(n);
    rewind(f);

    // Thanks a lot, Windows
    n = fread(*out, 1, n, f);
    (*out)[n] = '\0';
    return n;
}