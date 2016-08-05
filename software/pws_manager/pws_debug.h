///////////////////////////////////////////////////////////
// pws_debug.h
///////////////////////////////////////////////////////////
#ifndef __PWS_DEBUG_H__
#define __PWS_DEBUG_H__

#ifdef DEBUG_LOGOUT_STDIO

#ifdef DEBUG_LOGOUT_FILE
    #define PWS_DEBUG(fmt, ...) \
    { \
        char dout[512]; \
        printf( "[%s] %s:%u # " fmt, \
                  __DATE__, __FILE__, \
                  __LINE__, ##__VA_ARGS__ ); \
        snprintf( dout, sizeof(dout), "echo -n \"[%s] %s:%u # " fmt "\" >> /pws/log/pws_manager.log", \
                  __DATE__, __FILE__, \
                  __LINE__, ##__VA_ARGS__ ); \
        system(dout); \
    }
#else
    #define PWS_DEBUG(fmt, ...) \
        printf( "[%s] %s:%u # " fmt, \
                  __DATE__, __FILE__, \
                  __LINE__, ##__VA_ARGS__ )
#endif	// DEBUG_LOGOUT_FILE

#else

#ifdef DEBUG_LOGOUT_FILE
    #define PWS_DEBUG(fmt, ...) \
    { \
        char dout[512]; \
        snprintf( dout, sizeof(dout), "echo -n \"[%s] %s:%u # " fmt "\" >> /pws/log/pws_manager.log", \
                  __DATE__, __FILE__, \
                  __LINE__, ##__VA_ARGS__ ); \
        system(dout); \
    }
#else
    #define PWS_DEBUG( fmt, ... )
#endif	// DEBUG_LOGOUT_FILE

#endif	// DEBUG_LOGOUT_STDIO

#endif // __PWS_DEBUG_H__
