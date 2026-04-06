// configuration.h - NSan Configuration Definitions

#ifndef NSAN_CONFIGURATION_H
#define NSAN_CONFIGURATION_H

// Default configuration values
constexpr double DEFAULT_NSAN_EPSILON = 1e-5;
constexpr double DEFAULT_NSAN_REL_EPSILON = 1e-5;
constexpr int DEFAULT_NSAN_VERBOSITY = 1;

/// Initialize NSan configuration from environment variables.
void init_nsan_configuration();

#endif // NSAN_CONFIGURATION_H
