#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "jn_std.h"

ERROR jn_spin_lock(spin_t *spin)
{
    u32 ret = 0;
    while (ret == 0) {
        __asm__ __volatile__("movl $0x00, %%eax;\
                              movl $0x01, %%ebx;\
                              movl %0, %%ecx;\
                              lock cmpxchg %%ebx,(%%ecx); \
                              jne 1f; \
                              movl $0x01, %1; \
                              1:"
                             :"+m"(spin), "=m"(ret)
                             :
                             :"eax","ebx","ecx");
    }

    return ERROR_NONE;
}

ERROR jn_spin_unlock(spin_t *spin)
{
    __asm__ __volatile__("movl $0x00, %0":"=m"(*spin)::"memory");

    return ERROR_NONE;
}

