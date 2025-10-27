#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

#if 0 //ndef NDEBUG
#define DBG_USE_LOG
#endif

#ifndef DBG_USE_LOG

#define DBG_LOG(...)
#define DBG_M()
#define DBG_ONCE_LOG(...)

#else
#include <stdio.h>
#include <stdarg.h>

static inline void DbgLog(char const* fmt, ...) {
    FILE* fp = fopen("error.log", "ab+");
    if (fp) {
        va_list arg;
        va_start(arg, fmt);
        vfprintf(fp, fmt, arg);
        va_end(arg);
        fclose(fp);
    }
}

static inline void DbgMark(char const* fname, int line, char const* fnc) {
    DbgLog("%s (%d) : %s\n", fname, line, fnc);
}

#define DBG_LOG(...)    DbgLog(__VA_ARGS__)
#define DBG_M()         DbgMark(__FILE__, __LINE__, __func__)

#define DBG_ONCE_LOG(...)           \
    do {                            \
        static char __s_flag = 0;   \
        if (!__s_flag) {            \
            __s_flag = 1;           \
            DbgLog(__VA_ARGS__);    \
        }                           \
    } while (0)

#endif

#endif  // DBG_H_INCLUDED
