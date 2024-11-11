#include "chcore_littlefs_defs.h"
#include <chcore/ipc.h>
#include <sched.h>
#include <fs_wrapper_defs.h>
#include <fs_vnode.h>

bool mounted;
bool using_page_cache;
struct server_entry *server_entrys[MAX_SERVER_ENTRY_NUM];
struct rb_root* fs_vnode_list;

#ifdef TEST_COUNT_PAGE_CACHE
struct test_count count;
#endif

int main(void) {
    int ret;
    init_littlefs();

    ret = ipc_register_server_with_destructor(fs_server_dispatch, DEFAULT_CLIENT_REGISTER_HANDLER, fs_server_destructor);
	printf("[LittleFS Driver] register server value = %d\n", ret);

	while(1) {
		sched_yield();
	}

    return 0;
}