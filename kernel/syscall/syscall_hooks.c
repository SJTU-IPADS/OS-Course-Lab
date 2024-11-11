/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <common/types.h>
#include <object/thread.h>
#include <object/cap_group.h>
#include <object/memory.h>

/*
 * The syscall hooks can offer several features.
 * First, authorize the badge of the caller for the privileged syscalls.
 * Second, ease of debugging or export more information.
 */

int hook_sys_create_pmo(unsigned long size, pmo_type_t type, unsigned long val)
{
#ifdef CHCORE_KERNEL_DISALLOW_ACCESS_PHYMEM
        switch (type) {
        case PMO_DEVICE:
                /* This one is provided for user-level drivers only. */
                if (current_cap_group->badge >= APP_BADGE_START
                    || current_cap_group->badge < DRIVER_BADGE_START) {
                        kwarn("An unthorized process tries to create device pmo.\n");
                        return -EPERM;
                }
                break;
#ifdef CHCORE_OPENTRUSTEE
        case PMO_TZ_NS:
        case PMO_TZ_SHM:
                /* TODO: only gtask can create PMO_TZ_NS & PMO_TZ_SHM */
                return 0;
#endif /* CHCORE_OPENTRUSTEE */
        default:
                break;
        }
#endif
        return 0;
}

int hook_sys_get_phys_addr(vaddr_t va, paddr_t *pa_buf)
{
#ifdef CHCORE_KERNEL_DISALLOW_ACCESS_PHYMEM
        /* This one is only used in drivers and unit test now. */
        if (current_cap_group->badge >= APP_BADGE_START || current_cap_group->badge < DRIVER_BADGE_START) {
                kwarn("An unthorized process tries to get phys addr.\n");
                return -EPERM;
        }
#endif
        return 0;
}

int hook_sys_create_cap_group(unsigned long cap_group_args_p)
{
        if ((current_cap_group->badge != ROOT_CAP_GROUP_BADGE)
            && (current_cap_group->badge != FSM_BADGE)
            && (current_cap_group->badge != PROCMGR_BADGE)) {
                kwarn("An unthorized process tries to create cap_group.\n");
                return -EPERM;
        }
        return 0;
}

int hook_sys_register_recycle(cap_t notifc_cap, vaddr_t msg_buffer)
{
        if (current_cap_group->badge != PROCMGR_BADGE) {
                kwarn("A process (not the procmgr) tries to register recycle.\n");
                return -EPERM;
        }
        return 0;
}

int hook_sys_cap_group_recycle(cap_t cap_group_cap)
{
        if (current_cap_group->badge != PROCMGR_BADGE) {
                kwarn("A process (not the procmgr) tries to register recycle.\n");
                return -EPERM;
        }
        return 0;
}


int hook_sys_kill_group(cap_t proc_cap)
{
        if (current_cap_group->badge != PROCMGR_BADGE) {
                kwarn("A process (not the procmgr) tries to call kill.\n");
                return -EPERM;
        }
        return 0;
}
