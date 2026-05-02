#ifndef NSAN_CONFIGURATION_H
#define NSAN_CONFIGURATION_H

#include <cstdlib>

struct NSanConfig {
    double epsilon;
    int verbosity;
};

inline NSanConfig& get_nsan_config() {
    static NSanConfig config = {
        .epsilon = 1e-5,
        .verbosity = 1
    };
    return config;
}

inline void init_nsan_configuration() {
    const char* env_eps = std::getenv("NSAN_REL_EPSILON");
    if (env_eps) {
        get_nsan_config().epsilon = std::atof(env_eps);
    }

    const char* env_verb = std::getenv("NSAN_VERBOSITY");
    if (env_verb) {
        get_nsan_config().verbosity = std::atoi(env_verb);
    }
}

#endif