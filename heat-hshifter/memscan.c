/* ORIGINAL SOURCE: https://progamercity.net/c-code/904-memory-scanner-console-c.html */
#include "memscan.h"
#include <stdio.h>

#define IS_IN_SEARCH(mb,offset) (mb->searchmask[(offset)/8] & (1<<((offset)%8)))
#define REMOVE_FROM_SEARCH(mb,offset) mb->searchmask[(offset)/8] &= ~(1<<((offset)%8));
double finaladdress;

MEMBLOCK *create_memblock(HANDLE hProc, MEMORY_BASIC_INFORMATION *meminfo, int data_size) {
    MEMBLOCK *mb = malloc(sizeof(MEMBLOCK));

    if (mb) {
        mb->hProc = hProc;
        mb->addr = meminfo->BaseAddress;
        mb->size = (int) meminfo->RegionSize;
        mb->buffer = malloc(meminfo->RegionSize);
        mb->searchmask = malloc(meminfo->RegionSize / 8);
        if (mb->searchmask != 0) { memset(mb->searchmask, 0xff, (size_t) (meminfo->RegionSize / 8)); } else { return NULL; }
        mb->matches = (int) meminfo->RegionSize;
        mb->data_size = data_size;
        mb->next = NULL;
    }

    return mb;
}

void free_memblock(MEMBLOCK *mb) {
    if (mb) {
        if (mb->buffer) {
            free(mb->buffer);
        }

        if (mb->searchmask) {
            free(mb->searchmask);
        }

        free(mb);
    }
}

void update_memblock(MEMBLOCK *mb, SEARCH_CONDITION condition, unsigned int val) {
    static unsigned char tempbuf[128 * 1024];
    unsigned int bytes_left;
    unsigned int total_read;
    unsigned int bytes_to_read;
    unsigned int bytes_read;

    if (mb->matches > 0) {
        bytes_left = mb->size;
        total_read = 0;
        mb->matches = 0;

        while (bytes_left) {
            bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
            ReadProcessMemory(mb->hProc, mb->addr + total_read, tempbuf, bytes_to_read, (SIZE_T *) &bytes_read);
            if (bytes_read != bytes_to_read) break;

            unsigned int offset;

            for (offset = 0; offset < bytes_read; offset += mb->data_size) {
                if (IS_IN_SEARCH(mb, (total_read + offset))) {
                    unsigned int temp_val;
                    unsigned int prev_val = 0;

                    temp_val = *((unsigned int *) &tempbuf[offset]);
                    prev_val = *((unsigned int *) &mb->buffer[total_read + offset]);

                    if ((temp_val == val)) {
                        mb->matches++;
                    } else {
                        REMOVE_FROM_SEARCH(mb, (total_read + offset));
                    }
                }
            }

            memcpy(mb->buffer + total_read, tempbuf, bytes_read);

            bytes_left -= bytes_read;
            total_read += bytes_read;
        }

        mb->size = total_read;
    }
}

MEMBLOCK *create_scan(unsigned int pid, int data_size) {
    MEMBLOCK *mb_list = NULL;
    MEMORY_BASIC_INFORMATION meminfo;
    unsigned char *addr = 0;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (hProc) {
        while (1) {
            if (VirtualQueryEx(hProc, addr, &meminfo, sizeof(meminfo)) == 0) {
                break;
            }
#define WRITABLE (PAGE_READWRITE)
            if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE)) {
                MEMBLOCK *mb = create_memblock(hProc, &meminfo, data_size);
                if (mb) {
                    mb->next = mb_list;
                    mb_list = mb;
                }
            }
            addr = (unsigned char *) meminfo.BaseAddress + meminfo.RegionSize;
        }
    }

    return mb_list;
}

void free_scan(MEMBLOCK *mb_list) {
    CloseHandle(mb_list->hProc);

    while (mb_list) {
        MEMBLOCK *mb = mb_list;
        mb_list = mb_list->next;
        free_memblock(mb);
    }
}

void update_scan(MEMBLOCK *mb_list, SEARCH_CONDITION condition, unsigned int val) {
    MEMBLOCK *mb = mb_list;
    while (mb) {
        update_memblock(mb, condition, val);
        mb = mb->next;
    }
}

void dump_scan_info(MEMBLOCK *mb_list) {
    MEMBLOCK *mb = mb_list;

    while (mb) {
        int i;
        printf("[+] 0x%p %d\r\n", mb->addr, mb->size);

        for (i = 0; i < mb->size; i++) {
            printf("%02x", mb->buffer[i]);
        }
        printf("\r\n");

        mb = mb->next;
    }
}

uintptr_t peek(HANDLE hProc, int data_size, uintptr_t addr) {
    uintptr_t val = 0;

    if (ReadProcessMemory(hProc, (void *) addr, &val, data_size, NULL) == 0) {
        printf("[-] Peek failed..\r\n");
    }

    return val;
}

int get_match_count(MEMBLOCK *mb_list) {
    MEMBLOCK *mb = mb_list;
    int count = 0;

    while (mb) {
        count += mb->matches;
        mb = mb->next;
    }

    return count;
}

void print_matches(MEMBLOCK *mb_list, int data_size, uintptr_t *result_addr, UINT8 *max_index) {
    intptr_t offset;
    MEMBLOCK *mb = mb_list;
    unsigned int nmatches = 0;

    if (18 != get_match_count(mb_list)) {
        fprintf(stderr, "\n[-] Possibly inaccurate results!\n");
    }
    printf("[+] Omitting irrelevant results..\n");
    printf("[+] | ADDRESS         | HEX_VALUE  | size | value |\n");
    while (mb) {
        for (offset = 0; offset < mb->size; offset += mb->data_size) {
            if (IS_IN_SEARCH(mb, offset)) {
                uintptr_t val = peek(mb->hProc, mb->data_size, (uintptr_t) mb->addr + offset);
                if (8 == (((uintptr_t) mb->addr + offset) & 0x00000000F)) {
                    if (nmatches < 18) {
                        result_addr[nmatches] = ((uintptr_t) mb->addr + offset);
                    }
                    printf("[+] 0x%p: 0x%08x   {%d}    (%u)\r\n", mb->addr + offset, (int) val, data_size, (int) val);
                    nmatches++;
                }
            }
        }

        mb = mb->next;
    }

    printf("\n[+] Automatic scanner found %u possible matches.\n  Go back to the game and try shifting using your shifter.\n  If it doesn't work, press the '+' key and try again, until you find a working address.\n\n", nmatches);
    *max_index = nmatches;
}

MEMBLOCK *new_scan(DWORD pid) {
    MEMBLOCK *scan = NULL;

    while (1) {
        scan = create_scan((unsigned int) pid, sizeof(int));
        if (scan) break;
        printf("\r\n[-] Invalid scan");
    }

    return scan;
}

void scangear(MEMBLOCK *scan, DWORD pid, unsigned int gear) {
    printf("[*] Scanning, please wait..");
    fflush(stdout);
    update_scan(scan, COND_EQUALS, gear);
    printf("\r[+] %d matches found      \r\n\n", get_match_count(scan));
}
