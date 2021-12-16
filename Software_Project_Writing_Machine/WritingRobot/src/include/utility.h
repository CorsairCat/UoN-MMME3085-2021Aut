//  __DEBUG_MODE__ or __RELEASE_MODE__
#ifndef __MACH__
    // cant enable release mode under macOS
    // decide here which mode is set
    #ifndef __DEBUG_MODE__
    //    #define __DEBUG_MODE__
    #endif
    #ifndef __RELEASE_MODE__
        #define __RELEASE_MODE__
    #endif
#else
    #ifndef __DEBUG_MODE__
        #define __DEBUG_MODE__
    #endif
#endif
#ifdef __RELEASE_MODE__
    #define Serial_Mode
#endif