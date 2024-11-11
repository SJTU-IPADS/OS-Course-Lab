#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>

int main(int argc,char *argv[]){
    if(argc != 2){
        printf("Usage: test_dynamic_pmo [open][close]\n");
    }
    if(strcpy(argv[1], "close") == 0){
        cap_t pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        char* addr = chcore_alloc_vaddr(2 * PAGE_SIZE);
        usys_map_pmo(SELF_CAP, pmo, addr, VM_READ | VM_WRITE);
        addr[PAGE_SIZE + 5] = 'd';
        printf("%s", addr[PAGE_SIZE + 5]);
    }
}