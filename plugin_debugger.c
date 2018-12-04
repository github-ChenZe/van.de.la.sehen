#include "debugger_common.h"
#include "attribute_handler.h"
#include "ast_analyzer.h"
// #include "data_print.h"


/**
 *  no non-trivial code should be added to this function.
 *  this is only a caller to handlers.
 */

void finish_func(void* event, void* __unused__)
{
    push_print_func((tree) event);
    add_print_for_var((tree) event);
}

int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
    if (!plugin_default_version_check(version, &gcc_version)) 
    {
        printf("incompatible gcc/plugin versions\n");
        return 1;
    }

#ifdef DEBUGGING
    signal(SIGSEGV, segfault_handler);
#endif

    const char * const plugin_name = plugin_info->base_name;

    for (int i = 0; i < plugin_info->argc; i++) {
        printf("Plugin config: %s = %s\n", plugin_info->argv[i].key, plugin_info->argv[i].value);
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    register_callback(plugin_name, PLUGIN_ATTRIBUTES, register_attributes, NULL);
    register_callback(plugin_name, PLUGIN_FINISH_PARSE_FUNCTION, finish_func, NULL);
    // register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &my_passinfo);

    return 0;
}