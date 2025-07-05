/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include "chcore/defs.h"
#include "chcore/memory.h"
#include "chcore/type.h"
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore-internal/procmgr_defs.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <chcore/container/list.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <cyaml/cyaml.h>

#include "proc_node.h"
#include "procmgr_dbg.h"
#include "srvmgr.h"
#include "libchcoreelf.h"
#include "liblaunch.h"
#include "loader.h"
#include "uthash.h"
#include "readyaml_conf.h"

/*
 * Note: This is not an isolated server. It is still a part of procmgr and
 * resides in the same address space. We just take the code related to server
 * management apart from procmgr.c and put it here. Code related to server
 * management like booting configurable servers or getting some info about a
 * server should be put in this file from now on.
 * The server configuration file is located at /sys_servers.yaml by default.
 */

static int nr_caps = 0;
static uint8_t initialized = false;

/*
 * NOTE: We do not initialize the data structure in procmgr to prevent procmgr
 * from registering client of itself. Otherwise, it will lead to memory leak
 * when procmgr calls printf. If put() do not send IPC msg, it can be optimized.
 */
cap_t __procmgr_server_cap;

static pthread_mutex_t read_elf_lock;
static pthread_rwlock_t service_map_lock;
static pthread_mutex_t wait_init_lock;
static pthread_cond_t wait_init_cond;
#ifdef CHCORE_ARCH_SPARC
static pthread_mutex_t cas_lock;
#endif

cap_t tmpfs_cap = -1;

#define LAZY_BOOT_STR            "lazy"
#define EAGER_BOOT_STR           "eager"
#define ACTIVE_REGISTRATION_STR  "active"
#define PASSIVE_REGISTRATION_STR "passive"
#define SYSTEM_SERVER_STR        "server"
#define SYSTEM_DRIVER_STR        "driver"

/* Struct service means a service that be provided, such as sd card driver or
 * tmpfs */
struct service {
        char name[SERVICE_NAME_LEN + 1];
#define CAP_INVALID (-1)
        cap_t cap;

/* Bagde is recorded only if the registration method is active*/
#define SERVICE_BADGE_NOT_SET (-1)
#define SERVICE_BADGE_WAITING (-2)
        int badge;
        struct system_server *server;
        UT_hash_handle hh;
};

#define FILENAME_MAX_LEN       128
#define MAX_SERVICE_PER_SERVER 32
/* Struct system_server means a process that provides services, such as
 * "circle.srv" */
struct system_server {
        char filename[FILENAME_MAX_LEN + 1];
        int nr_services;
        char services[MAX_SERVICE_PER_SERVER][SERVICE_NAME_LEN + 1];
        enum { EAGER_BOOT = 0,
               LAZY_BOOT = 1,
        } boot_time;

        enum { ACTIVE_REGISTRATION = 0,
               PASSIVE_REGISTRATION = 1,
        } registration_method;

        enum { BOOT_ERROR = -1,
               WAIT_LAZY_BOOT = 0,
               BOOTED = 1,
        } boot_flag;
        struct list_head list_node;
        int process_type;
};

static struct list_head system_servers;
static struct service *services = NULL;

/* Add services of a server to service list */
static void register_sys_server(struct system_server *server)
{
        struct service *service;
        if (server->boot_time == EAGER_BOOT) {
                server->boot_flag = BOOTED;
        } else {
                server->boot_flag = WAIT_LAZY_BOOT;
        }
        for (int i = 0; i < server->nr_services; i++) {
                service = malloc(sizeof(struct service));
                service->cap = CAP_INVALID;
                service->badge = SERVICE_BADGE_NOT_SET;
                strncpy(service->name, server->services[i], SERVICE_NAME_LEN);
                service->server = server;
                HASH_ADD_STR(services, name, service);
        }
}

/* For booting system servers. Only called in the initialize phase (single
 * thread) and in the get_service_cap. The lock that protect HASH_FIND_STR is
 * locked in get_service_cap. */
static int boot_sys_server(struct system_server *server)
{
        int ret = 0;
        char *input_argv[PROC_REQ_ARGC_MAX];
        char *proc_name;
        struct proc_node *proc_node;
        struct service *service;
        if (server->registration_method == ACTIVE_REGISTRATION) {
                for (int i = 0; i < server->nr_services; i++) {
                        HASH_FIND_STR(services, server->services[i], service);
                        /* It is promised that service can be found. */
                        service->badge = SERVICE_BADGE_WAITING;
                }
        }

        input_argv[0] = malloc(FILENAME_MAX_LEN + 1);
        strncpy(input_argv[0], server->filename, FILENAME_MAX_LEN);
        input_argv[0][FILENAME_MAX_LEN] = '\0';
        proc_name = malloc(FILENAME_MAX_LEN + 1);
        strncpy(proc_name, server->filename, FILENAME_MAX_LEN);
        proc_name[FILENAME_MAX_LEN] = '\0';

        ret = procmgr_launch_process(1,
                                     input_argv,
                                     proc_name,
                                     false,
                                     0,
                                     NULL,
                                     server->process_type,
                                     &proc_node);
        if (ret < 0) {
                goto out;
        }

        if (server->registration_method == PASSIVE_REGISTRATION) {
                if (server->nr_services != 1) {
                        error("Passive registered server should"
                              " provide only one service\n");
                        error("Only the first service %s"
                              " will be registered\n",
                              server->services[0]);
                }
                HASH_FIND_STR(services, server->services[0], service);
                /* It is promised that service can be found. */
                service->cap = proc_node->proc_mt_cap;
        } else {
                for (int i = 0; i < server->nr_services; i++) {
                        HASH_FIND_STR(services, server->services[i], service);
                        /* It is promised that service can be found. */
                        service->badge = proc_node->badge;
                }
        }
        put_proc_node(proc_node);
out:
        return ret;
}

/* For booting system servers which are not recorded, fat32 and littlefs. */
static cap_t boot_sys_server_spec(char *srv_name, char *srv_path, int proc_type)
{
        int ret = 0;
        char *input_argv[PROC_REQ_ARGC_MAX];
        struct proc_node *proc_node;
        input_argv[0] = srv_path;
        ret = procmgr_launch_process(
                1, input_argv, srv_name, false, 0, NULL, proc_type, &proc_node);
        if (ret < 0) {
                goto out;
        }
        ret = proc_node->proc_mt_cap;
        put_proc_node(proc_node);
out:
        return ret;
}

void init_system_services(void)
{
        struct system_server *server;
        for_each_in_list (
                server, struct system_server, list_node, &system_servers) {
                register_sys_server(server);
        }
        for_each_in_list (
                server, struct system_server, list_node, &system_servers) {
                if (server->boot_time == EAGER_BOOT) {
                        if (boot_sys_server(server) < 0) {
                                server->boot_flag = BOOT_ERROR;
                        }
                }
        }
}

void set_tmpfs_cap(cap_t cap)
{
        tmpfs_cap = cap;
}

/* Return proc_node of new process */
static int do_launch_process(int argc, char **argv, char *name,
                             bool if_has_parent, badge_t parent_badge,
                             struct user_elf *user_elf, struct loader *loader,
                             struct new_process_args *np_args, int proc_type,
                             struct proc_node **out_proc)
{
        struct proc_node *parent_proc_node;
        struct proc_node *proc_node;
        cap_t caps[3];
        int ret;
        cap_t new_proc_cap;
        cap_t new_proc_mt_cap;
        struct launch_process_args lp_args;

        /* Get parent_proc_node */
        if (if_has_parent) {
                parent_proc_node = get_proc_node(parent_badge);
                if (!parent_proc_node) {
                        ret = -EINVAL;
                        goto out_ret;
                }
        } else {
                parent_proc_node = NULL;
        }

        debug("client: %p, cmd: %s\n", parent_proc_node, argv[1]);
        debug("fsm ipc_struct conn_cap=%d server_type=%d\n",
              fsm_ipc_struct->conn_cap,
              fsm_ipc_struct->server_id);

        // printf("[procmgr] Launching %s...\n", name);

        /* Init caps */
        caps[0] = __procmgr_server_cap;
        caps[1] = fsm_server_cap;
        caps[2] = lwip_server_cap;

        for (; nr_caps < 3; nr_caps++) {
                if (caps[nr_caps] == 0)
                        break;
        }

        /* Create proc_node to get pid */
        ret = new_proc_node(parent_proc_node, name, proc_type, &proc_node);
        if (ret < 0) {
                goto out_put_parent;
        }

        if (np_args == NULL) {
                lp_args.stack_size = MAIN_THREAD_STACK_SIZE;
                lp_args.envp_argc = 0;
                lp_args.envp_argv = NULL;
#ifdef CHCORE_OPENTRUSTEE
                lp_args.puuid = NULL;
                lp_args.heap_size = (unsigned long)-1;
#endif /* CHCORE_OPENTRUSTEE */
        } else {
                lp_args.stack_size =
                        MIN(ROUND_UP(np_args->stack_size, PAGE_SIZE),
                            MAIN_THREAD_STACK_SIZE);
                lp_args.envp_argc = np_args->envp_argc;
                lp_args.envp_argv = np_args->envp_argv;
#ifdef CHCORE_OPENTRUSTEE
                lp_args.puuid = &np_args->puuid;
                lp_args.heap_size = np_args->heap_size;
#endif /* CHCORE_OPENTRUSTEE */
        }
        /* Init launch_process_args */
        lp_args.user_elf = user_elf;
        lp_args.child_process_cap = &new_proc_cap;
        lp_args.child_main_thread_cap = &new_proc_mt_cap;
        lp_args.pmo_map_reqs = NULL;
        lp_args.nr_pmo_map_reqs = 0;
        lp_args.caps = caps;
        lp_args.nr_caps = nr_caps;
        lp_args.cpuid = 0;
        lp_args.argc = argc;
        lp_args.argv = argv;
        lp_args.badge = proc_node->badge;
        lp_args.pid = proc_node->pid;
        lp_args.pcid = proc_node->pcid;
        lp_args.process_name = argv[0];
#ifdef CHCORE_OPENTRUSTEE
        if (!strcmp(argv[0], "/tarunner.elf")) {
                lp_args.process_name = name;
        }
#endif /* CHCORE_OPENTRUSTEE */
        lp_args.load_offset = 0;
        if (proc_type == TRACED_APP) {
                lp_args.is_traced = true;
        } else {
                lp_args.is_traced = false;
        }

#ifdef CHCORE_OPENTRUSTEE
        if (lp_args.puuid != NULL) {
                memcpy(&proc_node->puuid, lp_args.puuid, sizeof(spawn_uuid_t));
        } else {
                memset(&proc_node->puuid, 0, sizeof(spawn_uuid_t));
        }
#endif /* CHCORE_OPENTRUSTEE */
        proc_node->stack_size = lp_args.stack_size;

        /* Launch process */
        if (loader) {
                ret = launch_process_using_loader(loader, &lp_args);
        } else {
                ret = launch_process_with_pmos_caps(&lp_args);
        }

        if (ret != 0) {
                error("launch process failed\n");
                goto out_undo_new_proc;
        }

        /* Set proc_node properties */
        proc_node->state = PROC_STATE_RUNNING;
        proc_node->proc_cap = new_proc_cap;
        proc_node->proc_mt_cap = new_proc_mt_cap;

        debug("new_proc_node: pid=%d cmd=%s parent_pid=%d\n",
              proc_node->pid,
              proc_node->name,
              proc_node->parent ? proc_node->parent->pid : -1);

        if (out_proc) {
                *out_proc = proc_node;
        } else {
                put_proc_node(proc_node);
        }

        if (if_has_parent) {
                put_proc_node(parent_proc_node);
        }
        return 0;

out_undo_new_proc:
        del_proc_node(proc_node);
out_put_parent:
        if (if_has_parent) {
                put_proc_node(parent_proc_node);
        }
out_ret:
        return ret;
}

int procmgr_launch_process(int argc, char **argv, char *name,
                           bool if_has_parent, badge_t parent_badge,
                           struct new_process_args *np_args, int proc_type,
                           struct proc_node **out_proc)
{
        int ret;
        struct elf_header *elf_header;
        struct user_elf *user_elf = NULL;
        struct proc_node *node = NULL;
        struct loader *loader = NULL;

        /**
         * Read ELF header first to detect dynamically linked programs.
         */
        ret = load_elf_header_from_fs(argv[0], &elf_header);
        if (ret < 0) {
                error("load_elf_header_from_fs failed, argv[0]=%s\n", argv[0]);
                goto out;
        }

        /**
         * For dynamically linked programs, launch it using CHCORE_LOADER,
         * otherwise load remaining ELF content and launch it directly.
         */
        if (elf_header->e_type == ET_DYN) {
                ret = find_loader(CHCORE_LOADER, &loader);
        } else {
                ret = load_elf_by_header_from_fs(
                        argv[0], elf_header, &user_elf);
        }

        if (ret < 0) {
                error("Error when launching: %s, ret = %d\n", argv[0], ret);
                goto out_free;
        }

        ret = do_launch_process(argc,
                                argv,
                                name,
                                if_has_parent,
                                parent_badge,
                                user_elf,
                                loader,
                                np_args,
                                proc_type,
                                &node);
        if (ret < 0) {
                goto out_free;
        }

        if (out_proc) {
                *out_proc = node;
        } else {
                put_proc_node(node);
        }
        ret = 0;

out_free:
        if (user_elf) {
                free_user_elf(user_elf);
        }
        free(elf_header);
out:
        return ret;
}

int procmgr_launch_basic_server(int input_argc, char **input_argv, char *name,
                                bool if_has_parent, badge_t parent_badge,
                                struct proc_node **out_proc)
{
        int ret;
        struct proc_node *node;
        struct user_elf *user_elf;

        if (strcmp(input_argv[0], "/tmpfs.srv") == 0) {
                pthread_mutex_lock(&read_elf_lock);
                ret = load_elf_from_mem(&__binary_tmpfs_elf_start, &user_elf);
                if (ret == 0) {
                        strncpy(user_elf->path, "/tmpfs.srv", ELF_PATH_LEN);
                }
                pthread_mutex_unlock(&read_elf_lock);
        } else if (strcmp(input_argv[0], "/fsm.srv") == 0) {
                pthread_mutex_lock(&read_elf_lock);
                ret = load_elf_from_mem(&__binary_fsm_elf_start, &user_elf);
                if (ret == 0) {
                        strncpy(user_elf->path, "/fsm.srv", ELF_PATH_LEN);
                }
                pthread_mutex_unlock(&read_elf_lock);

        } else {
                ret = -1;
        }
        if (ret < 0) {
                debug("Error args or parsing elf error!\n");
                goto out;
        }

        ret = do_launch_process(input_argc,
                                input_argv,
                                name,
                                if_has_parent,
                                parent_badge,
                                user_elf,
                                NULL,
                                NULL,
                                SYSTEM_SERVER,
                                &node);
        if (ret < 0) {
                goto out_free;
        }

        if (out_proc) {
                *out_proc = node;
        } else {
                put_proc_node(node);
        }
        ret = 0;

out_free:
        free_user_elf(user_elf);
out:
        return ret;
}

static void wait_for_init(void)
{
        pthread_mutex_lock(&wait_init_lock);
        while (!initialized) {
                pthread_cond_wait(&wait_init_cond, &wait_init_lock);
        }
        pthread_mutex_unlock(&wait_init_lock);
}

void handle_set_service_cap(ipc_msg_t *ipc_msg, badge_t badge,
                            struct proc_request *pr)
{
        int ret = 0;
        char service_name[SERVICE_NAME_LEN + 1];
        struct service *service = NULL;
        if (!initialized) {
                wait_for_init();
        }
        pthread_rwlock_wrlock(&service_map_lock);
        strncpy(service_name,
                pr->set_service_cap.service_name,
                SERVICE_NAME_LEN);
        service_name[SERVICE_NAME_LEN] = '\0';

        HASH_FIND_STR(services, service_name, service);
        /* The new registered service. */
        if (service == NULL) {
                struct service *new_service = calloc(1, sizeof(struct service));
                strncpy(new_service->name, service_name, SERVICE_NAME_LEN);
                new_service->cap = ipc_get_msg_cap(ipc_msg, 0);
                new_service->badge = badge;
                new_service->server = NULL;
                HASH_ADD_STR(services, name, new_service);
        }
        /* The passive registered service's caps can only be set by srvmgr. */
        else if (service->server
                 && service->server->registration_method
                            == PASSIVE_REGISTRATION) {
                error("passive registered service's caps can only be set by srvmgr\n");
                ret = -EPERM;
        } else if (service->badge == SERVICE_BADGE_WAITING) {
                ret = -EIPCRETRY;
        }
        /* The setter process is not the creator process. */
        else if (service->badge != badge) {
                error("badge: %d, service->badge: %d\n", badge, service->badge);
                ret = -EPERM;
        } else {
                service->cap = ipc_get_msg_cap(ipc_msg, 0);
        }
        pthread_rwlock_unlock(&service_map_lock);
        ipc_return(ipc_msg, ret);
}

static int CAS(int *ptr, int oldval, int newval)
{
#ifdef CHCORE_ARCH_SPARC
        int ret;
        pthread_mutex_lock(&cas_lock);
        if (*ptr == oldval) {
                *ptr = newval;
                ret = oldval;
        } else {
                ret = *ptr;
        }
        pthread_mutex_unlock(&cas_lock);
        return ret;
#else
        return __sync_val_compare_and_swap(ptr, oldval, newval);
#endif
}

void handle_get_service_cap(ipc_msg_t *ipc_msg, struct proc_request *pr)
{
        int ret = 0;
        struct service *service;
        /* Boot a new FS server in each PROCMGR_REQ_GET_SERVICE_CAP */
        if (strncmp(pr->get_service_cap.service_name,
                    SERVER_FAT32_FS,
                    SERVICE_NAME_LEN)
            == 0) {
                ret = boot_sys_server_spec(
                        "fat32", "/fat32.srv", SYSTEM_SERVER);
                if (ret < 0) {
                        goto out;
                }
                ipc_set_msg_return_cap_num(ipc_msg, 1);
                ipc_set_msg_cap(ipc_msg, 0, ret);
                ret = 0;
                goto out;
        } else if (strncmp(pr->get_service_cap.service_name,
                           SERVER_LITTLEFS,
                           SERVICE_NAME_LEN)
                   == 0) {
                ret = boot_sys_server_spec(
                        "littlefs", "/littlefs.srv", SYSTEM_SERVER);
                if (ret < 0) {
                        goto out;
                }
                ipc_set_msg_return_cap_num(ipc_msg, 1);
                ipc_set_msg_cap(ipc_msg, 0, ret);
                ret = 0;
                goto out;
        } else if (strncmp(pr->get_service_cap.service_name,
                           SERVER_TMPFS,
                           SERVICE_NAME_LEN)
                   == 0) {
                if (tmpfs_cap < 0) {
                        ret = -EIPCRETRY;
                        goto out;
                }
                ipc_set_msg_return_cap_num(ipc_msg, 1);
                ipc_set_msg_cap(ipc_msg, 0, tmpfs_cap);
                ret = 0;
                goto out;
        }

        if (!initialized) {
                wait_for_init();
        }
        pthread_rwlock_rdlock(&service_map_lock);
        /* Check if server_id is valid */
        HASH_FIND_STR(services, pr->get_service_cap.service_name, service);
        pthread_rwlock_unlock(&service_map_lock);
        if (service == NULL) {
                error("Service %s not found\n",
                      pr->get_service_cap.service_name);
                ret = -EINVAL;
                goto out;
        }
        /* ptr to struct service to one sys server is never changed, and rw of
         * service->cap is atomic, so there is no need to protecting by the
         * lock here. */
        for (;;) {
                /* Server already booted */
                if (service->cap >= 0) {
                        ipc_set_msg_return_cap_num(ipc_msg, 1);
                        ipc_set_msg_cap(ipc_msg, 0, service->cap);
                        break;
                }
                /* If the server has no cap and is in the list, it is a system
                 * service, and its server pointer is not null.
                 * Only one thread can enter this branch. And the struct service
                 * with a LAZY_BOOT server can only be modified here, therefore
                 * we chould get write lock after releasing read lock. */
                else if (service->server
                         && CAS(&(service->server->boot_flag),
                                WAIT_LAZY_BOOT,
                                BOOTED)
                                    == WAIT_LAZY_BOOT) {
                        pthread_rwlock_wrlock(&service_map_lock);
                        /* Wait until server boots. */
                        ret = boot_sys_server(service->server);
                        pthread_rwlock_unlock(&service_map_lock);
                        if (ret < 0) {
                                service->server->boot_flag = BOOT_ERROR;
                                break;
                        }
                } else if (service->server
                           && service->server->boot_flag == BOOTED) {
                        usys_yield();
                }
                /* Each server has only one chance to boot. */
                else if (service->server
                         && service->server->boot_flag == BOOT_ERROR) {
                        error("Service %s boot failed\n",
                              pr->get_service_cap.service_name);
                        ret = -EINVAL;
                        break;
                } else {
                        ret = -1;
                        break;
                }
        }
out:
        if (ret == 0) {
                ipc_return_with_cap(ipc_msg, 0);
        } else {
                ipc_return(ipc_msg, ret);
        }
}

/* Read yaml file with libcyaml */

int read_system_servers(void)
{
        yaml_server_t *servers;
        unsigned int server_count;
        cyaml_load_file(SYS_SERVER_CONFIGURE_FILE,
                        &cyaml_config,
                        &yaml_servers_schema,
                        (cyaml_data_t **)&servers,
                        &server_count);
        for (unsigned i = 0; i < server_count; i++) {
                struct system_server *server =
                        malloc(sizeof(struct system_server));
                strncpy(server->filename,
                        servers[i].filename,
                        FILENAME_MAX_LEN);
                if (strcmp(servers[i].boot_time, LAZY_BOOT_STR) == 0) {
                        server->boot_time = LAZY_BOOT;
                } else if (strcmp(servers[i].boot_time, EAGER_BOOT_STR) == 0) {
                        server->boot_time = EAGER_BOOT;
                } else {
                        error("Invalid boot time: %s\n", servers[i].boot_time);
                        continue;
                }
                if (strcmp(servers[i].registration_method,
                           ACTIVE_REGISTRATION_STR)
                    == 0) {
                        server->registration_method = ACTIVE_REGISTRATION;
                } else if (strcmp(servers[i].registration_method,
                                  PASSIVE_REGISTRATION_STR)
                           == 0) {
                        server->registration_method = PASSIVE_REGISTRATION;
                } else {
                        error("Invalid registration method: %s\n",
                              servers[i].registration_method);
                        continue;
                }
                if (strcmp(servers[i].type, SYSTEM_SERVER_STR) == 0) {
                        server->process_type = SYSTEM_SERVER;
                } else if (strcmp(servers[i].type, SYSTEM_DRIVER_STR) == 0) {
                        server->process_type = SYSTEM_DRIVER;
                } else {
                        error("Invalid server type: %s\n", servers[i].type);
                        continue;
                }
                server->nr_services = servers[i].services_count;
                for (unsigned j = 0; j < servers[i].services_count; j++) {
                        strncpy(server->services[j],
                                servers[i].services[j],
                                SERVICE_NAME_LEN);
                }
                list_add(&server->list_node, &system_servers);
        }
        cyaml_free(&cyaml_config, &yaml_servers_schema, servers, 0);
        return 0;
}

void boot_secondary_servers(void)
{
        read_system_servers();
        init_system_services();
        initialized = true;
        pthread_mutex_lock(&wait_init_lock);
        pthread_cond_broadcast(&wait_init_cond);
        pthread_mutex_unlock(&wait_init_lock);
}

void init_srvmgr(void)
{
        /* Init read_elf_lock */
        pthread_mutex_init(&read_elf_lock, NULL);
        pthread_rwlock_init(&service_map_lock, NULL);
        pthread_mutex_init(&wait_init_lock, NULL);
        pthread_cond_init(&wait_init_cond, NULL);
#ifdef CHCORE_ARCH_SPARC
        pthread_mutex_init(&cas_lock, NULL);
#endif
        /* Init servers list */
        init_list_head(&system_servers);
}
