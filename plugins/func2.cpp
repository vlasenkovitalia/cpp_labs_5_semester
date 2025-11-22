#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern "C" {
    typedef double (*plugin_fn_t)(const double* args, int nargs);
    struct PluginDescriptor {
        const char* name;
        int min_arity;
        int max_arity;
        plugin_fn_t fn;
    };
    
    static double sin_degrees(const double* args, int nargs) {
        if (nargs != 1) throw std::runtime_error("sin() expects 1 argument");
        double deg = args[0];
        double rad = deg * (M_PI / 180.0);
        return std::sin(rad);
    }

    __declspec(dllexport) bool RegisterPlugin(PluginDescriptor* outDesc, char* errbuf, int errbuf_len) {
        if (!outDesc) {
            if (errbuf && errbuf_len > 0) strncpy(errbuf, "Null outDesc", errbuf_len - 1);
            return false;
        }
        outDesc->name = "sin";
        outDesc->min_arity = 1;
        outDesc->max_arity = 1;
        outDesc->fn = &sin_degrees;
        return true;
    }
}
