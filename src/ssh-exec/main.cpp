#include <ryml.hpp>
#include <c4/c4core_all.hpp>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <memory>

#if defined(_WIN32) || defined(_WIN64)
# include <ws2tcpip.h>
# include <winsock.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif


// Not empty definitions filter with ability to raise a preprocessor error on not defined definitions or empty definitions:
//  MSVC (2017) utilized errors:
//  * not defined:   `C2124: divide or mod by zero`
//  * defined empty: `C1017: invalid integer constant expression`
//  GCC (5.4) utilized errors:
//  * not defined:   `division by zero in #if`
//  * defined empty: `operator '||' has no left operand`
//
#if defined(__GNUC__) && __GNUC__ >= 3
#   define ERROR_IF_EMPTY_PP_DEF(x, args...) (x || (!defined(x ## args) && 0/x))
#else
#   define ERROR_IF_EMPTY_PP_DEF(x) (x || (!defined (## x ##) && 0/x))
#endif

#if defined(_WIN32) || defined(_WIN64)
#   define SHUT_RDWR SD_BOTH
#endif

#if defined(_WIN32) || defined(_WIN64)
#   define ENABLE_WIN32_LAMBDA_TRY_FINALLY 1
#   define ENABLE_CXX_LAMBDA_TRY_FINALLY 0
#else
#   define ENABLE_WIN32_LAMBDA_TRY_FINALLY 0
#   define ENABLE_CXX_LAMBDA_TRY_FINALLY 1
#endif

#define UTILITY_PP_CONCAT_(v1, v2) v1 ## v2
#define UTILITY_PP_CONCAT(v1, v2) UTILITY_PP_CONCAT_(v1, v2)

#define UTILITY_PP_LINE_ __LINE__
#define UTILITY_PP_LINE UTILITY_PP_LINE_

#define UTILITY_PP_VA_ARGS_(...)    __VA_ARGS__
#define UTILITY_PP_VA_ARGS(...)     UTILITY_PP_VA_ARGS_(__VA_ARGS__)

#define UTILITY_DECLARE_LINE_UNIQUE_TYPE(token)     ::utility::identity<struct UTILITY_PP_CONCAT(UTILITY_PP_CONCAT(token, _L), UTILITY_PP_LINE)>

#if ERROR_IF_EMPTY_PP_DEF(ENABLE_WIN32_LAMBDA_TRY_FINALLY)
#   ifdef LAMBDA_TRY_BEGIN
#       error: LAMBDA_TRY_BEGIN already defined
#   endif

// NOTE:
//  lambda to bypass msvc error: `error C2712: Cannot use ... in functions that require object unwinding`
//
// NOTE:
//  `do` and `while(false);` between macroses is required to avoid quantity of errors around missed brackets and in the same time requires to use `{}` brackets separately.
//
#   define LAMBDA_TRY_BEGIN(return_type)    [&]() -> ::utility::lambda_try_return_holder<return_type> { __try { return [&]() -> ::utility::lambda_try_return_holder<return_type> { do
#   define LAMBDA_TRY_BEGIN_VA(...)         [&]() -> ::utility::lambda_try_return_holder<UTILITY_PP_VA_ARGS(__VA_ARGS__)> { __try { return [&]() -> ::utility::lambda_try_return_holder<UTILITY_PP_VA_ARGS(__VA_ARGS__)> { do
#   define LAMBDA_TRY_FINALLY()             while(false); return {}; }(); } __finally { do
#   define LAMBDA_TRY_END()                 while(false); } return {}; }().get();
#   define LAMBDA_TRY_RETURN(v)             return ::utility::make_lambda_try_return_holder(v)
#   define LAMBDA_TRY_RETURN_VA(...)        return ::utility::make_lambda_try_return_holder(UTILITY_PP_VA_ARGS(__VA_ARGS__))
#endif

#if ERROR_IF_EMPTY_PP_DEF(ENABLE_CXX_LAMBDA_TRY_FINALLY)
#   ifdef LAMBDA_TRY_BEGIN
#       error: LAMBDA_TRY_BEGIN already defined
#   endif

// NOTE:
//  `do` and `while(false);` between macroses is required to avoid quantity of errors around missed brackets and in the same time requires to use `{}` brackets separately.
//
// CAUTION:
//  Potential code breakage under `Microsoft Visual C++ 2019` around `static const auto & ... = [&]() { ... };` expression because of `static` keyword.
//  DO NOT UNCOMMENT.
//
#   define LAMBDA_TRY_BEGIN(return_type)    [&]() -> ::utility::lambda_try_return_holder<return_type> { \
                                                /*static*/ const auto & lambda_try_catch = [&](bool lambda_try_throw_finally) -> ::utility::lambda_try_return_holder<return_type> { \
                                                    try { if (lambda_try_throw_finally) throw UTILITY_DECLARE_LINE_UNIQUE_TYPE(lambda_try_throw_finally){}; do
#   define LAMBDA_TRY_BEGIN_VA(...)         [&]() -> ::utility::lambda_try_return_holder<return_type> { \
                                                /*static*/ const auto & lambda_try_catch = [&](bool lambda_try_throw_finally) -> ::utility::lambda_try_return_holder<UTILITY_PP_VA_ARGS(__VA_ARGS__)> { \
                                                    try { if (lambda_try_throw_finally) throw UTILITY_DECLARE_LINE_UNIQUE_TYPE(lambda_try_throw_finally){}; do
#   define LAMBDA_TRY_FINALLY()             while(false); } catch(...) { do
#   define LAMBDA_TRY_END()                 while(false); if(!lambda_try_throw_finally) throw; } return {}; }; auto ret = lambda_try_catch(false); lambda_try_catch(true); return ret; }().get();
#   define LAMBDA_TRY_RETURN(v)             return ::utility::make_lambda_try_return_holder(v)
#   define LAMBDA_TRY_RETURN_VA(...)        return ::utility::make_lambda_try_return_holder(UTILITY_PP_VA_ARGS(__VA_ARGS__))
#endif

#define C_STR(csubstr) to_str(csubstr).c_str()
#define C_IPSTR(ip_int) to_ipstr(ip_int).c_str()


namespace utility
{
    // naked return usage protection in lambda-try
    template <typename T = void>
    class lambda_try_return_holder;

    template <typename T>
    class lambda_try_return_holder
    {
    public:
        lambda_try_return_holder(lambda_try_return_holder&&) = default;
        lambda_try_return_holder(const lambda_try_return_holder&) = default;

        lambda_try_return_holder()
            : this_()
        {
        }

        explicit lambda_try_return_holder(T&& v)
            : this_(std::forward<T>(v))
        {
        }

        explicit lambda_try_return_holder(const T& v)
            : this_(v)
        {
        }

        lambda_try_return_holder& operator =(const lambda_try_return_holder&) = default;
        lambda_try_return_holder& operator =(lambda_try_return_holder&&) = default;

        operator T& ()
        {
            return this_;
        }

        operator const T& ()
        {
            return this_;
        }

        T& get()
        {
            return this_;
        }

        const T& get() const
        {
            return this_;
        }

        T&& move()
        {
            return std::move(this_);
        }

    private:
        T this_;
    };

    template <>
    class lambda_try_return_holder<void>
    {
    public:
        lambda_try_return_holder(lambda_try_return_holder&&) = default;
        lambda_try_return_holder(const lambda_try_return_holder&) = default;

        lambda_try_return_holder()
        {
        }

        lambda_try_return_holder& operator =(const lambda_try_return_holder&) = default;
        lambda_try_return_holder& operator =(lambda_try_return_holder&&) = default;

        void get() const
        {
        }

        void move()
        {
        }
    };

    template <typename T>
    lambda_try_return_holder<T> make_lambda_try_return_holder(T&& v)
    {
        return lambda_try_return_holder<T>{ std::forward<T>(v) };
    }

    template <typename T>
    lambda_try_return_holder<T> make_lambda_try_return_holder(const T& v)
    {
        return lambda_try_return_holder<T>{ v };
    }

    lambda_try_return_holder<void> make_lambda_try_return_holder()
    {
        return lambda_try_return_holder<void>{};
    }

    // std::identity is depricated in msvc2017

    template <typename T>
    struct identity
    {
        using type = T;
    };
}


inline std::string to_str(c4::csubstr substr)
{
    return std::string{ substr.str, substr.len };
}

inline std::string to_ipstr(int ip)
{
    struct sockaddr_in sa;
    char str[INET_ADDRSTRLEN];

    sa.sin_addr.s_addr = ip;

    inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);

    return std::string{ str };
}

struct copy_settings
{
    struct flags {
        enum flags_t
        {
            // by default target files overwrite is disallowed
            allow_next_target_files_overwrite = 0x01, // allow target files overwrite, applies to all next items
            no_target_files_overwrite = 0x02  // skip target file existence with a warning, applies to current item only
        };
    };

    copy_settings() : flags() {}

    copy_settings(flags::flags_t flags_) : flags(flags_) {}

    copy_settings(const copy_settings& settings) = default;
    copy_settings(copy_settings&& settings) = default;

    // clear current item flags
    void clear_current()
    {
        flags = flags::flags_t(flags & ~flags::no_target_files_overwrite);
    }

    flags::flags_t flags;
};

struct remote
{
    c4::csubstr name;
    c4::csubstr user;
    c4::csubstr pass;
    int ip;
    int port;
};

using remotes_t = std::vector<remote>;

struct exec
{
    struct command
    {
        enum command_t
        {
            none = 0,
            copy_file = 1
        };

        command(command_t type_) : type(type_) {}
        virtual ~command() {}

        command_t type;
    };

    using commands_t = std::vector<std::shared_ptr<command> >;

    struct copy_file : command
    {
        copy_file() : command(command::copy_file) {}

        copy_file(const copy_file&) = default;
        copy_file(copy_file&&) = default;

        copy_file(c4::csubstr from_, c4::csubstr to_, remotes_t remotes_, copy_settings settings_) :
            command(command::copy_file),
            from(from_),
            to(to_),
            remotes(remotes_),
            settings(settings_)
        {
        }

        c4::csubstr   from;
        c4::csubstr   to;
        remotes_t     remotes;
        copy_settings settings;
    };

    commands_t commands;
};

template <typename T, typename Func>
T& update_inherited_stack(std::stack<T>& stack, typename std::stack<T>::size_type max_stack_size, Func&& stack_top_update_pred, bool push_new)
{
    // deallocate levels in the stack
    while (stack.size() > max_stack_size) {
        stack.pop();
    }

    assert(max_stack_size && stack.size());

    // first level must be already allocated
    auto* stack_top_ptr = &stack.top();

    // update top item
    stack_top_update_pred();

    if (push_new) {
        // allocate new top with inheritance
        stack.push(stack.top());
    }

    return stack.top();
}

int get_file_contents(const char* filename, std::string& out)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "could not open " << filename << std::endl;
        return 1;
    }

    std::ostringstream contents;
    contents << in.rdbuf();
    out = contents.str();

    return 0;
}

bool add_remote(remotes_t& remotes, remote& remote_to_add)
{
    if (remote_to_add.name.empty()) {
        fprintf(stderr, "error: remote name is empty\n");
        return false;
    }
    if (remote_to_add.user.empty()) {
        fprintf(stderr, "error: remote user is empty: name=`%s`\n",
            C_STR(remote_to_add.name));
        return false;
    }
    if (!remote_to_add.ip || !remote_to_add.port) {
        fprintf(stderr, "error: remote ip or port is invalid: name=`%s` ip=`%s` port=`%u`\n",
            C_STR(remote_to_add.name), C_IPSTR(remote_to_add.ip), remote_to_add.port);
        return false;
    }

    for (auto& remote : remotes) {
        if (remote.name == remote_to_add.name) {
            fprintf(stderr, "error: remote name is not unique: name=`%s`\n",
                C_STR(remote_to_add.name));
            return false;
        }
        if (remote.ip == remote_to_add.ip && remote.port == remote_to_add.port) {
            fprintf(stderr, "error: remote ip and port is not unique: name=`%s` ip=`%s` port=`%u`\n",
                C_STR(remote_to_add.name), C_IPSTR(remote_to_add.ip), remote_to_add.port);
            return false;
        }
    }

    remotes.push_back(remote_to_add);

    return true;
}

bool read_remotes_yaml(ryml::NodeRef& node, remotes_t& remotes)
{
    remote remote_to_add;
    bool has_remote_to_add = false;

    for (auto remote_items : node.children()) {
        if (remote_items.is_map()) {
            for (auto remote_item : remote_items.children()) {
                if (remote_item.has_key()) {
                    auto remote_item_key = remote_item.key();

                    if (!remote_item_key.compare("name")) {
                        if (has_remote_to_add) {
                            if (!add_remote(remotes, remote_to_add)) {
                                return false;
                            }
                        }
                        if (remote_item.has_val()) {
                            remote_to_add = { remote_item.val(), {}, {}, 0, 0 };
                        }
                        has_remote_to_add = true;
                    }
                    else if (!remote_item_key.compare("user")) {
                        if (remote_item.has_val()) {
                            remote_to_add.user = remote_item.val();
                        }
                        has_remote_to_add = true;
                    }
                    else if (!remote_item_key.compare("pass")) {
                        if (remote_item.has_val()) {
                            remote_to_add.pass = remote_item.val();
                        }
                        has_remote_to_add = true;
                    }
                    else if (!remote_item_key.compare("ip")) {
                        if (remote_item.has_val()) {
                            auto ip_val = remote_item.val();

                            struct sockaddr_in sa;

                            // store this IP address in sa:
                            inet_pton(AF_INET, C_STR(ip_val), &(sa.sin_addr));

                            remote_to_add.ip = sa.sin_addr.s_addr;
                        }
                        has_remote_to_add = true;
                    }
                    else if (!remote_item_key.compare("port")) {
                        if (remote_item.has_val()) {
                            auto ip_val = remote_item.val();
                            if (ip_val.is_integer()) {
                                remote_to_add.port = atoi(ip_val.str);
                            }
                        }
                        has_remote_to_add = true;
                    }
                }
            }
        }
    }

    if (has_remote_to_add) {
        if (!add_remote(remotes, remote_to_add)) {
            return false;
        }
        has_remote_to_add = false; // just in case
    }

    return true;
}

bool read_exec_copy_settings_yaml(ryml::NodeRef& node, exec& exec, copy_settings& copy_settings, bool is_next_commands_settings)
{
    if (node.is_map()) {
        for (auto copy_settings_item : node.children()) {
            if (copy_settings_item.has_key()) {
                auto copy_settings_item_key = copy_settings_item.key();

                if (!copy_settings_item_key.compare("flags")) {
                    if (copy_settings_item.is_seq()) {
                        for (auto copy_settings_flags_item : copy_settings_item.children()) {
                            if (is_next_commands_settings) {
                                if (!copy_settings_flags_item.val().compare("allow-next-target-files-overwrite")) {
                                    copy_settings.flags = copy_settings::flags::flags_t(copy_settings.flags | copy_settings::flags::allow_next_target_files_overwrite);
                                }
                                else if (!copy_settings_flags_item.val().compare("deny-next-target-files-overwrite")) {
                                    copy_settings.flags = copy_settings::flags::flags_t(copy_settings.flags & ~copy_settings::flags::allow_next_target_files_overwrite);
                                }
                                else {
                                    // malformed config
                                    fprintf(stderr, "error: malformed config: `exec/copy/settings/flags/*`\n");
                                    return false;
                                }
                            }
                            else {
                                if (!copy_settings_flags_item.val().compare("no-target-files-overwrite")) {
                                    copy_settings.flags = copy_settings::flags::flags_t(copy_settings.flags | copy_settings::flags::no_target_files_overwrite);
                                }
                                else {
                                    // malformed config
                                    fprintf(stderr, "error: malformed config: `exec/copy/settings/flags/*`\n");
                                    return false;
                                }
                            }
                        }
                    }
                    else {
                        // malformed config
                        fprintf(stderr, "error: malformed config: `exec/copy/settings/flags`\n");
                        return false;
                    }
                }
                else {
                    // malformed config
                    fprintf(stderr, "error: malformed config: `exec/copy/settings`\n");
                    return false;
                }
            }
        }
    }
    else {
        // malformed config
        fprintf(stderr, "error: malformed config: `exec/copy/settings`\n");
        return false;
    }

    return true;
}

bool read_exec_copy_files_yaml(ryml::NodeRef& node, exec& exec, std::stack<copy_settings>& copy_settings_stack, std::stack<remotes_t>& remotes_stack)
{
    c4::csubstr from_file; // reused

    for (auto exec_copy_items : node.children()) {
        if (exec_copy_items.is_map()) {
            c4::csubstr to_file;
            bool has_from_file = false;
            bool has_to_file = false;

            // search for command settings at first
            for (auto exec_copy_sub_item : exec_copy_items.children()) {
                if (exec_copy_sub_item.has_key()) {
                    auto exec_copy_sub_item_key = exec_copy_sub_item.key();

                    if (!exec_copy_sub_item_key.compare("from")) {
                        if (has_from_file) {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/files/from`\n");
                            return false;
                        }

                        has_from_file = true;

                        if (exec_copy_sub_item.has_val()) {
                            from_file = exec_copy_sub_item.val();
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/files`\n");
                            return false;
                        }
                    }
                    else if (!exec_copy_sub_item_key.compare("to")) {
                        if (has_to_file) {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/files/from`\n");
                            return false;
                        }

                        has_to_file = true;

                        if (exec_copy_sub_item.has_val()) {
                            to_file = exec_copy_sub_item.val();
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/files`\n");
                            return false;
                        }
                    }
                    else {
                        // malformed config
                        fprintf(stderr, "error: malformed config: `exec/copy`\n");
                        return false;
                    }
                }
                else {
                    // malformed config
                    fprintf(stderr, "error: malformed config: `exec/copy`\n");
                    return false;
                }
            }

            if (!has_to_file || from_file.empty()) {
                // malformed config
                fprintf(stderr, "error: malformed config: `exec/copy/files`\n");
                return false;
            }

            exec.commands.push_back(
                std::make_shared<exec::copy_file>(
                    from_file, to_file, remotes_stack.top(), copy_settings_stack.top()));
        }
        else {
            // malformed config
            fprintf(stderr, "error: malformed config: `exec/copy`\n");
            return false;
        }
    }

    return true;
}

bool read_exec_copy_yaml(ryml::NodeRef& node, exec& exec, std::stack<copy_settings>& copy_settings_stack, std::stack<remotes_t>& remotes_stack, bool has_copy_settings)
{
    for (auto exec_copy_items : node.children()) {
        if (exec_copy_items.has_key()) {
            auto exec_copy_items_key = exec_copy_items.key();

            // copy settings without files
            if (!exec_copy_items_key.compare("settings")) {
                if (has_copy_settings) {
                    // malformed config
                    fprintf(stderr, "error: malformed config: `exec/copy/settings`\n");
                    return false;
                }

                // first level must be already allocated
                auto& current_copy_settings = update_inherited_stack(copy_settings_stack, 1,
                    [&]() -> void {
                        // always clear current item flags for level 1
                        copy_settings_stack.top().clear_current();
                    },
                    false);

                if (!read_exec_copy_settings_yaml(exec_copy_items, exec, current_copy_settings, true)) {
                    return false;
                }

                // skip all the rest
                return true;
            }
            else {
                // malformed config
                fprintf(stderr, "error: malformed config: `exec/copy`\n");
                return false;
            }
        }
        else if (exec_copy_items.is_map()) {
            ryml::NodeRef exec_copy_setings_node;
            bool has_copy_files = false;

            // search for command settings at first
            for (auto exec_copy_sub_item : exec_copy_items.children()) {
                if (exec_copy_sub_item.has_key()) {
                    auto exec_copy_sub_item_key = exec_copy_sub_item.key();

                    if (!exec_copy_sub_item_key.compare("files")) {
                        if (!has_copy_files) {
                            has_copy_files = true;
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/files`\n");
                            return false;
                        }
                    }
                    else if (!exec_copy_sub_item_key.compare("settings")) {
                        if (!exec_copy_setings_node.valid()) {
                            exec_copy_setings_node = exec_copy_sub_item;
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy/settings`\n");
                            return false;
                        }
                    }
                    else {
                        // malformed config
                        fprintf(stderr, "error: malformed config: `exec/copy`\n");
                        return false;
                    }
                }
                else {
                    // malformed config
                    fprintf(stderr, "error: malformed config: `exec/copy`\n");
                    return false;
                }
            }

            if (!has_copy_files && !exec_copy_setings_node.valid()) {
                // malformed config
                fprintf(stderr, "error: malformed config: `exec/copy`\n");
                return false;
            }

            // first level must be already allocated
            auto& current_copy_settings = update_inherited_stack(copy_settings_stack, 3,
                [&]() -> void {
                    // always clear current item flags for level > 2
                    if (copy_settings_stack.size() > 2) {
                        copy_settings_stack.top().clear_current();
                    }
                },
                exec_copy_setings_node.valid());

            if (exec_copy_setings_node.valid()) {
                // read copy command settings
                if (!read_exec_copy_settings_yaml(exec_copy_setings_node, exec, current_copy_settings, !has_copy_files)) {
                    return false;
                }
            }

            if (has_copy_files) {
                for (auto exec_copy_sub_item : exec_copy_items.children()) {
                    if (exec_copy_sub_item.has_key()) {
                        auto exec_copy_sub_item_key = exec_copy_sub_item.key();

                        if (!exec_copy_sub_item_key.compare("files")) {
                            if (!read_exec_copy_files_yaml(exec_copy_sub_item, exec, copy_settings_stack, remotes_stack)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        else {
            // malformed config
            fprintf(stderr, "error: malformed config: `exec/copy`\n");
            return false;
        }
    }

    return true;
}

bool read_exec_yaml(ryml::NodeRef& node, exec& exec, std::stack<copy_settings>& copy_settings_stack, std::stack<remotes_t>& remotes_stack)
{
    for (auto exec_items : node.children()) {
        if (exec_items.is_map()) {
            ryml::NodeRef exec_setings_node;
            bool has_copy = false;

            // search for command settings at first
            for (auto exec_item : exec_items.children()) {
                if (exec_item.has_key()) {
                    auto exec_item_key = exec_item.key();

                    if (!exec_item_key.compare("copy")) {
                        if (!has_copy) {
                            has_copy = true;
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/copy`\n");
                            return false;
                        }
                    }
                    else if (!exec_item_key.compare("settings")) {
                        if (!exec_setings_node.valid()) {
                            exec_setings_node = exec_item;
                        }
                        else {
                            // malformed config
                            fprintf(stderr, "error: malformed config: `exec/settings`\n");
                            return false;
                        }
                    }
                    else {
                        // malformed config
                        fprintf(stderr, "error: malformed config: `exec`\n");
                        return false;
                    }
                }
                else {
                    // malformed config
                    fprintf(stderr, "error: malformed config: `exec`\n");
                    return false;
                }
            }

            if (!has_copy) {
                // malformed config
                fprintf(stderr, "error: malformed config: `exec`\n");
                return false;
            }

            // first level must be already allocated
            auto& current_copy_settings = update_inherited_stack(copy_settings_stack, 1,
                [&]() {
                    // always clear current item flags for level 1
                    copy_settings_stack.top().clear_current();
                },
                exec_setings_node.valid());

            if (has_copy) {
                if (exec_setings_node.valid()) {
                    // read copy command settings
                    if (!read_exec_copy_settings_yaml(exec_setings_node, exec, current_copy_settings, false)) {
                        return false;
                    }
                }

                for (auto exec_item : exec_items.children()) {
                    if (exec_item.has_key()) {
                        auto exec_item_key = exec_item.key();

                        if (!exec_item_key.compare("copy")) {
                            if (!read_exec_copy_yaml(exec_item, exec, copy_settings_stack, remotes_stack, exec_setings_node.valid())) {
                                return false;
                            }
                        }
                    }
                }
            }
            //else ...
        }
        else {
            // malformed config
            fprintf(stderr, "error: malformed config: `exec`\n");
            return false;
        }
    }

    return true;
}

int main(int argc, const char* argv[])
{
    do (void)0;
     while (false);
    int ret = 0;
    int arg_offset_begin = 0;

    if (argv[0][0] != '/') { // arguments shift detection
        arg_offset_begin = 1;
    }

    if (!argc || !argv[0]) {
        fputs("error: invalid command line format", stderr);
        return 255;
    }

    std::string config_file_path = "ssh-exec.yml";

    int arg_offset = 0 + arg_offset_begin;
    const char* arg;

    while (argc >= arg_offset + 1)
    {
        arg = argv[arg_offset];
        if (!arg) {
            fputws(L"error: invalid command line format", stderr);
            return 255;
        }

        if (!strcmp(arg, "/c")) {
            arg_offset += 1;

            if (argc >= arg_offset + 1) {
                arg = argv[arg_offset];
                if (!arg) {
                    fputs("error: invalid command line format", stderr);
                    return 255;
                }

                config_file_path.assign(arg);
            }
            else {
                fprintf(stderr, "error: invalid flag: `%s`\n", arg);
                return 255;
            }
        }
        else {
            fprintf(stderr, "error: invalid flag: `%s`\n", arg);
            return 255;
        }

        arg_offset += 1;
    }

    FILE* config_file_handle = NULL;

#if defined(_WIN32) || defined(_WIN64)
    bool is_wsa_inited = false;
#endif
    bool is_libssh2_inited = false;

    return ret = LAMBDA_TRY_BEGIN(int)
    {
        std::string config_file_str;

        if (get_file_contents(config_file_path.c_str(), config_file_str)) {
            fprintf(stderr, "error: could not open file: `%s`\n", config_file_path.c_str());
            LAMBDA_TRY_RETURN(1);
        }

        ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(config_file_str));

        auto tree_root = tree.rootref();

        exec exec;

        {
            // just in case, must be deconstructed before the yaml tree holder
            std::stack<copy_settings> copy_settings_stack;
            std::stack<remotes_t> remotes_stack;

            for (auto child_node : tree_root.children()) {
                if (child_node.has_key()) {
                    auto child_key = child_node.key();

                    // remotes
                    if (!child_key.compare(ryml::csubstr{ "remotes" })) {
                        remotes_stack = {};
                        remotes_stack.push(remotes_t{});

                        auto& current_remotes = remotes_stack.top();

                        if (!read_remotes_yaml(child_node, current_remotes)) {
                            LAMBDA_TRY_RETURN(2);
                        }

                        if (current_remotes.empty()) {
                            fprintf(stderr, "error: no root remotes\n");
                            LAMBDA_TRY_RETURN(2);
                        }
                    }
                    else if (!child_key.compare(ryml::csubstr{ "exec" })) {
                        copy_settings_stack = {};
                        copy_settings_stack.push(copy_settings{});

                        exec.commands.clear();

                        if (!read_exec_yaml(child_node, exec, copy_settings_stack, remotes_stack)) {
                            LAMBDA_TRY_RETURN(3);
                        }

                        if (exec.commands.empty()) {
                            fprintf(stderr, "error: no commands to execute\n");
                            LAMBDA_TRY_RETURN(3);
                        }

                        // connect to remotes

                        // network one time initialization
#if defined(_WIN32) || defined(_WIN64)
                        if (!is_wsa_inited) {
                            WSADATA wsaData;
                            int res;
                            if (res = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
                                fprintf(stderr, "error: WSAStartup initialization failed: %08X (%u)\n", res, res);
                                LAMBDA_TRY_RETURN(4);
                            }
                            is_wsa_inited = true;
                        }
#endif

                        const size_t copy_buf_size = 65536;
                        std::unique_ptr<uint8_t[]> copy_buf_ptr = std::make_unique<uint8_t[]>(copy_buf_size);
#if defined(_WIN32) || defined(_WIN64)
                        char errno_buf[256];
#endif
                        const char* errno_str = nullptr;

                        LAMBDA_TRY_BEGIN(int)
                        {
                            if (!is_libssh2_inited) {
                                if (libssh2_init(0)) {
                                    fprintf(stderr, "error: libssh2 initialization failed\n");
                                    LAMBDA_TRY_RETURN(4);
                                }
                                is_libssh2_inited = true;
                            }

                            auto& remotes = remotes_stack.top();

                            for (const auto& remote : remotes) {
                                SOCKET sock = -1;
                                struct sockaddr_in sin {};
                                LIBSSH2_SESSION* ssh_session = nullptr;
                                LIBSSH2_SFTP* sftp_session = nullptr;
                                LIBSSH2_SFTP_HANDLE* sftp_handle = nullptr;

                                LAMBDA_TRY_BEGIN(int)
                                {
                                    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                                    if (sock < 0) {
                                        fprintf(stderr, "error: socket cretion failed\n");
                                        LAMBDA_TRY_RETURN(5);
                                    }

                                    sin.sin_family = AF_INET;
                                    sin.sin_port = htons(remote.port);
                                    sin.sin_addr.s_addr = remote.ip;

                                    printf("connecting: name=`%s` ip=`%s` port=`%u`\n",
                                        C_STR(remote.name), C_IPSTR(remote.ip), remote.port);

                                    if (connect(sock, (struct sockaddr*)&sin, sizeof(sin))) {
#if defined(_WIN32) || defined(_WIN64)
                                        strerror_s(errno_buf, errno);
#else
                                        errno_str = std::strerror(errno);
#endif
                                        fprintf(stderr, "error: remote connection is failed: name=`%s` ip=`%s` port=`%u` error=%08X `%s`\n",
                                            C_STR(remote.name), C_IPSTR(remote.ip), remote.port, errno, errno_str);
                                        LAMBDA_TRY_RETURN(6);
                                    }

                                    printf("  - connected.\n");

                                    if (!(ssh_session = libssh2_session_init())) {
                                        fprintf(stderr, "error: remote SSH session creation is failed: name=`%s` ip=`%s` port=`%u`\n",
                                            C_STR(remote.name), C_IPSTR(remote.ip), remote.port);
                                        LAMBDA_TRY_RETURN(7);
                                    }

                                    libssh2_session_set_blocking(ssh_session, 1);

                                    if (libssh2_session_handshake(ssh_session, sock)) {
                                        fprintf(stderr, "error: remote SSH session handshake is failed: name=`%s` ip=`%s` port=`%u`\n",
                                            C_STR(remote.name), C_IPSTR(remote.ip), remote.port);
                                        LAMBDA_TRY_RETURN(8);
                                    }

                                    printf("logging SSH session: user=`%s`\n", C_STR(remote.user));

                                    if (libssh2_userauth_password(ssh_session, C_STR(remote.user), C_STR(remote.pass))) {
                                        fprintf(stderr, "error: remote SSH session login is failed: name=`%s` ip=`%s` port=`%u` user=`%s`\n",
                                            C_STR(remote.name), C_IPSTR(remote.ip), remote.port, C_STR(remote.user));
                                        LAMBDA_TRY_RETURN(64);
                                    }

                                    printf("  - logged.\n");

                                    if (!(sftp_session = libssh2_sftp_init(ssh_session))) {
                                        fprintf(stderr, "error: remote SFTP session creation is failed: name=`%s` ip=`%s` port=`%u` user=`%s`\n",
                                            C_STR(remote.name), C_IPSTR(remote.ip), remote.port, C_STR(remote.user));
                                        LAMBDA_TRY_RETURN(128);
                                    }

                                    puts("");

                                    // execute commands

                                    puts("Executing:");

                                    for (const auto& command_ptr : exec.commands) {
                                        exec::command* exec_command_ptr = command_ptr.get();

                                        if (!exec_command_ptr) {
                                            continue; // just in case
                                        }

                                        switch (exec_command_ptr->type) {
                                        case exec::command::copy_file:
                                        {
                                            // copy file

                                            FILE* from_file_handle = nullptr;

                                            exec::copy_file* copy_file_ptr = static_cast<exec::copy_file*>(exec_command_ptr);

                                            printf("  - copy file:\n      from:  %s\n      to:    %s\n      flags: %08X\n",
                                                C_STR(copy_file_ptr->from), C_STR(copy_file_ptr->to), copy_file_ptr->settings.flags);

                                            LAMBDA_TRY_BEGIN(int)
                                            {
#if defined(_WIN32) || defined(_WIN64)
                                                fopen_s(&from_file_handle, C_STR(copy_file_ptr->from), "rb");
#else
                                                from_file_handle = fopen(C_STR(copy_file_ptr->from), "rb");
#endif
                                                if (!from_file_handle) {
                                                    fprintf(stderr, "error: local file open is failed\n");
                                                    LAMBDA_TRY_RETURN(0);
                                                }

                                                sftp_handle = libssh2_sftp_open(sftp_session, C_STR(copy_file_ptr->to),
                                                    LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                                    LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
                                                    LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
                                                if (!sftp_handle) {
                                                    fprintf(stderr, "error: remote file open is failed.\n");
                                                    LAMBDA_TRY_RETURN(0);
                                                }

                                                // copying...

                                                size_t read_bytes;
                                                ssize_t write_bytes = 0;

                                                do
                                                {
                                                    read_bytes = fread(copy_buf_ptr.get(), 1, copy_buf_size, from_file_handle);
                                                    if (read_bytes > 0)
                                                    {
                                                        write_bytes = libssh2_sftp_write(sftp_handle, (char*)copy_buf_ptr.get(), read_bytes);
                                                        if (write_bytes != read_bytes) {
                                                            fprintf(stderr, "error: remote file incomplete write: read=%zu (bytes) write=%zd (bytes)\n",
                                                                read_bytes, write_bytes);
                                                            LAMBDA_TRY_RETURN(0);
                                                        }
                                                    }
                                                } while (read_bytes > 0);

                                                printf("    write bytes: %zu\n", write_bytes);

                                                LAMBDA_TRY_RETURN(0);
                                            }
                                            LAMBDA_TRY_FINALLY()
                                            {
                                                if (sftp_handle) {
                                                    libssh2_sftp_close(sftp_handle);
                                                    sftp_handle = nullptr;
                                                }
                                                if (from_file_handle) {
                                                    fclose(from_file_handle);
                                                    from_file_handle = nullptr;
                                                }
                                            }
                                            LAMBDA_TRY_END()
                                        } break;
                                        }
                                    }

                                    LAMBDA_TRY_RETURN(0);
                                }
                                LAMBDA_TRY_FINALLY()
                                {
                                    puts("");

                                    if (sftp_session) {
                                        libssh2_sftp_shutdown(sftp_session);
                                        sftp_session = nullptr;
                                    }
                                    if (ssh_session) {
                                        libssh2_session_disconnect(ssh_session, ".");
                                        libssh2_session_free(ssh_session);
                                        ssh_session = nullptr;
                                    }

                                    printf("  - disconnected.\n\n");

                                    if (sock >= 0) {
                                        closesocket(sock);
                                        shutdown(sock, SHUT_RDWR); // just in case
                                        sock = -1;
                                    }
                                }
                                LAMBDA_TRY_END()
                            }

                            LAMBDA_TRY_RETURN(0);
                        }
                        LAMBDA_TRY_FINALLY()
                        {
                            libssh2_exit();
                            is_libssh2_inited = false;
                        }
                        LAMBDA_TRY_END()
                    }
                }
            }
        }

        LAMBDA_TRY_RETURN(0);
    }
    LAMBDA_TRY_FINALLY()
    {
        if (config_file_handle) {
            fclose(config_file_handle);
        }
        // dll uninit/unload always at last
#if defined(_WIN32) || defined(_WIN64)
        if (is_wsa_inited) {
            WSACleanup();
            is_wsa_inited = false;
        }
#endif
    }
    LAMBDA_TRY_END()
}
