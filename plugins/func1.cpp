#include <cmath>
#include <cstring>
#include <stdexcept>

extern "C" {
    typedef double (*plugin_fn_t)(const double* args, int nargs);
    struct PluginDescriptor {
        const char* name;
        int min_arity;
        int max_arity;
        plugin_fn_t fn;
    };

    static double ln_func(const double* args, int nargs) {
        if (nargs != 1) throw std::runtime_error("ln() expects 1 argument");
        double x = args[0];
        if (x <= 0.0) throw std::runtime_error("ln domain error: input must be > 0");
        return std::log(x);
    }

    __declspec(dllexport) bool RegisterPlugin(PluginDescriptor* outDesc, char* errbuf, int errbuf_len) {
        if (!outDesc) {
            if (errbuf && errbuf_len>0) strncpy(errbuf, "Null outDesc", errbuf_len-1);
            return false;
        }
        outDesc->name = "ln";
        outDesc->min_arity = 1;
        outDesc->max_arity = 1;
        outDesc->fn = &ln_func;
        return true;
    }
}
