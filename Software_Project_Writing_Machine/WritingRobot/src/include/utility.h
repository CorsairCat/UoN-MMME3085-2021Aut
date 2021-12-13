//  __DEBUG_MODE__ or __RELEASE_MODE__
#ifndef __DEBUG_MODE__
//    #define __DEBUG_MODE__
#endif
#ifndef __RELEASE_MODE__
    #define __RELEASE_MODE__
#endif
#ifdef __RELEASE_MODE__
    #define Serial_Mode
#endif