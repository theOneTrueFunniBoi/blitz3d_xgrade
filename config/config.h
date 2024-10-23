
#ifndef CONFIG_H
#define CONFIG_H

#define BASE_VERSION 1001

#if BB_LIBSGD_ENABLED
    #define VERSION (0x30000 | BASE_VERSION)
#elif BB_FMOD_ENABLED
    #define VERSION (0x10000 | BASE_VERSION)
#else	// SOLOUD
    #define VERSION (0x20000 | BASE_VERSION)
#endif

#endif
