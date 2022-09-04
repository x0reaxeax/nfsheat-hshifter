#ifndef _MEMSCAN_H
#define _MEMSCAN_H

#include <Windows.h>

typedef struct _MEMBLOCK {
    HANDLE hProc;
    unsigned char *addr;
    int size;
    unsigned char *buffer;

    unsigned char *searchmask;
    int matches;
    int data_size;

    struct _MEMBLOCK *next;
} MEMBLOCK;

typedef enum {
    COND_UNCONDITIONAL,
    COND_EQUALS,

    COND_INCREASED,
    COND_DECREASED,
} SEARCH_CONDITION;


MEMBLOCK *new_scan(DWORD pid);
void scangear(MEMBLOCK *scan, DWORD pid, unsigned int gear);
void print_matches(MEMBLOCK *mb_list, int data_size, uintptr_t *result_addr, UINT8 *max_index);
void free_scan(MEMBLOCK *mb_list);

#endif // !_MEMSCAN_H
