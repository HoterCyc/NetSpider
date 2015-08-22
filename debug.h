#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG

#define PRINT(Info) \
    do {\
        printf("%s %d: %s\n", __FILE__, __LINE__, Info); \
    } while(0) 

#endif
