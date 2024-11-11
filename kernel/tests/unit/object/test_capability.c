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

#include <string.h>
#include <object/object.h>
#include <object/thread.h>
#include <object/cap_group.h>
#include "minunit_local.h"
#include "test_object.h"

u64 virt_to_phys(u64 x)
{
        return x;
}

u64 phys_to_virt(u64 x)
{
        return x;
}

#define BUG_MSG_ON(condition, format, arg...)  \
        do {                                   \
                if (condition) {               \
                        printf(format, ##arg); \
                        BUG();                 \
                }                              \
        } while (0)

#define UNUSED(x) ((void)x)

cap_t sys_create_cap_group(unsigned long cap_group_args_p);

struct thread_args {
        /* specify the cap_group in which the new thread will be created */
        cap_t cap_group_cap;
        vaddr_t stack;
        vaddr_t pc;
        unsigned long arg;
        vaddr_t tls;
        unsigned int prio;
        unsigned int type;
};

struct cap_group_args {
	badge_t badge;
	vaddr_t name;
	unsigned long name_len;
	unsigned long pcid;
};

cap_t sys_create_thread(unsigned long thread_args_p);
struct thread *create_user_thread_stub(struct cap_group *cap_group);

int sys_get_all_caps(cap_t cap_group_cap)
{
        struct cap_group *cap_group;
        struct slot_table *slot_table;
        cap_t i;

        cap_group = obj_get(current_cap_group, cap_group_cap, TYPE_CAP_GROUP);
        if (!cap_group)
                return -ECAPBILITY;
        printk("thread %p cap:\n", current_thread);

        slot_table = &cap_group->slot_table;
        for (i = 0; i < slot_table->slots_size; i++) {
                struct object_slot *slot = get_slot(cap_group, i);
                if (!slot)
                        continue;
                printk("slot_id:%d type:%d\n",
                       i,
                       slot_table->slots[i]->object->type);
        }

        obj_put(cap_group);
        return 0;
}

static void test_expand(struct cap_group *cap_group)
{
        int i;
        cap_t cap_tmp, cap_lowest;
        void *obj;

#define TOTAL_ALLOC 200
        for (i = 0; i < TOTAL_ALLOC; i++) {
                obj = obj_alloc(TYPE_TEST_OBJECT, sizeof(struct test_object));
                cap_tmp = cap_alloc(cap_group, obj);
                /* printf("\tinit multiple test_object cap = %d\n", cap_tmp); */
                if (i == 0) {
                        cap_lowest = cap_tmp;
                } else {
                        mu_assert_msg(cap_tmp == cap_lowest + i,
                                      "slot id not increasing sequentially\n");
                }
        }
        printf("alloc from %d to %d\n",
               cap_lowest,
               cap_lowest + TOTAL_ALLOC - 1);

#define FREE_OFFSET 60
#define TOTAL_FREE  30
        for (i = FREE_OFFSET; i < FREE_OFFSET + TOTAL_FREE; i++)
                cap_free(cap_group, i);
        printf("free from %d to %d\n",
               FREE_OFFSET,
               FREE_OFFSET + TOTAL_FREE - 1);

        for (i = 0; i < TOTAL_FREE; i++) {
                obj = obj_alloc(TYPE_TEST_OBJECT, sizeof(struct test_object));
                cap_tmp = cap_alloc(cap_group, obj);
                mu_assert_msg(cap_tmp == FREE_OFFSET + i,
                              "create test_object failed, ret = %d\n",
                              cap_tmp);
        }
        printf("alloc from %d to %d\n",
               FREE_OFFSET,
               FREE_OFFSET + TOTAL_FREE - 1);
}

MU_TEST(test_check)
{
        int ret;
        cap_t cap_child_cap_group, cap_thread_child;
        cap_t cap, cap_copied;
        struct thread *child_thread, *root_thread;
        struct cap_group *child_cap_group, *root_cap_group;
        struct test_object *test_object;
        struct object *object;
        void *obj;
        struct thread_args args = {0};
        struct cap_group_args cg_args = {0};

        /* create root cap_group */
        root_cap_group = create_root_cap_group("test", 4);
        mu_assert_msg(root_cap_group != NULL, "create_root_cap_group failed\n");

        /* create root thread */
        root_thread = create_user_thread_stub(root_cap_group);
        mu_assert_msg(root_thread != NULL, "create_user_thread failed\n");

        /* root_thread create child cap_group */
        current_thread = root_thread;
        cg_args.badge = 0x1024llu;
        cg_args.name = (vaddr_t)"test";
        cg_args.name_len = 4;
        cg_args.pcid = 0;
        ret = sys_create_cap_group((unsigned long)&cg_args);
        mu_assert_msg(ret >= 0, "sys_create_cap_group failed, ret = %d\n", ret);
        cap_child_cap_group = ret;
        child_cap_group =
                obj_get(root_cap_group, cap_child_cap_group, TYPE_CAP_GROUP);

        /* create thread child cap_group*/
        args.cap_group_cap = cap_child_cap_group;
        #define TYPE_USER 2
        args.type = TYPE_USER;
        #define MIN_PRIO 1
        args.prio = MIN_PRIO;
        ret = sys_create_thread((unsigned long)&args);
        mu_assert_msg(ret >= 0, "sys_create_thread failed, ret = %d\n", ret);
        /* ret: child thread cap in parent process */
        cap_thread_child = ret;
        child_thread = obj_get(root_cap_group, cap_thread_child, TYPE_THREAD);
        mu_assert_msg(child_thread != NULL, "obj_get child_thread failed\n");

        /* create object */
        obj = obj_alloc(TYPE_TEST_OBJECT, sizeof(struct test_object));
        ret = cap_alloc(root_cap_group, obj);
        mu_assert_msg(ret >= 0, "create test_object failed, ret = %d\n", ret);
        cap = ret;
        printf("init test_object cap = %d\n", cap);

        /* invoke SET */
        ret = sys_test_object_set_int(cap, 0xdeadbeef);
        mu_assert_msg(ret >= 0,
                      "invoke sys_test_object_set_int failed, ret = %d\n",
                      ret);

        /* copy */
        ret = cap_copy(root_cap_group,
                       root_cap_group,
                       cap,
                       CAP_RIGHT_NO_RIGHTS,
                       CAP_RIGHT_NO_RIGHTS);
        mu_assert_msg(ret >= 0, "copy cap failed, ret = %d\n", ret);
        cap_copied = ret;
        printf("copied cap = %d\n", cap_copied);

        /* test expanding slots */
        test_expand(root_cap_group);

        /* free old cap */
        ret = cap_free(root_cap_group, cap);
        mu_assert_msg(ret >= 0, "free cap failed, ret = %d\n", ret);

        /* invoke GET on old cap should failed */
        ret = sys_test_object_get_int(cap);
        mu_assert_msg(ret != 0,
                      "sys_test_object_get_int should failed but ret 0\n");

        /* invoke GET on copied cap */
        ret = sys_test_object_get_int(cap_copied);
        mu_assert_msg(
                ret >= 0, "sys_test_object_get_int failed, ret = %d\n", ret);
        cap = cap_copied;

        /* copy to child cap_group */
        cap_copied = cap_copy(root_cap_group,
                              child_cap_group,
                              cap,
                              CAP_RIGHT_NO_RIGHTS,
                              CAP_RIGHT_NO_RIGHTS);
        mu_assert_msg(
                cap_copied >= 0, "cap_copy failed, ret = %d\n", cap_copied);

        /* invoke GET on child copied cap */
        current_thread = child_thread;
        ret = sys_test_object_get_int(cap_copied);
        mu_assert_msg(
                ret >= 0, "sys_test_object_get_int failed, ret = %d\n", ret);

        test_object = obj_get(child_cap_group, cap_copied, TYPE_TEST_OBJECT);

        /* remove all the caps pointed to some object */
        ret = cap_free_all(child_cap_group, cap_copied);
        mu_assert_msg(ret >= 0, "cap_free_all failed, ret = %d\n", ret);
        object = container_of(test_object, struct object, opaque);
        mu_assert_msg(object->refcount == 1, "refcount of obj not 1\n");

        /* invoke on original cap should fail */
        current_thread = root_thread;
        ret = sys_test_object_get_int(cap);
        mu_assert_msg(ret != 0,
                      "sys_test_object_get_int should failed but ret 0\n");

        obj_put(test_object);
        obj = obj_alloc(TYPE_PMO, sizeof(struct test_object));
        ret = cap_alloc_with_rights(root_cap_group, obj, PMO_READ | PMO_WRITE);
        mu_assert_msg(ret >= 0, "cap_alloc_with_rights failed, ret = %d\n", ret);
        cap = ret;
        ret = cap_copy(root_cap_group, root_cap_group, cap, PMO_ALL_RIGHTS, PMO_READ);
        mu_assert_msg(ret >= 0, "copy cap may restrict failed, ret = %d\n", ret);
        cap_copied = ret;
        obj = obj_get_with_rights(root_cap_group, cap_copied, TYPE_PMO, PMO_READ, PMO_READ);
        mu_assert_msg(obj != NULL, "copy cap may restrict failed\n");
        obj_put(obj);
        obj = obj_get_with_rights(root_cap_group, cap_copied, TYPE_PMO, PMO_WRITE, PMO_WRITE);
        mu_assert_msg(obj == NULL, "copy cap may restrict failed\n");
        ret = cap_free(root_cap_group, cap);
        mu_assert_msg(ret >= 0, "free cap failed, ret = %d\n", ret);
        obj = obj_alloc(TYPE_PMO, sizeof(struct test_object));
        ret = cap_alloc_with_rights(root_cap_group, obj, PMO_READ);
        mu_assert_msg(ret >= 0, "cap_alloc_with_rights failed, ret = %d\n", ret);
        cap = ret;
        ret = cap_copy(root_cap_group, root_cap_group, cap, PMO_ALL_RIGHTS, PMO_READ | PMO_WRITE);
        mu_assert_msg(ret < 0, "copy cap may restrict should failed but ret %d\n", ret);
        ret = cap_free(root_cap_group, cap);
        mu_assert_msg(ret >= 0, "free cap failed, ret = %d\n", ret);

        /* test revoke all & copy */
        obj = obj_alloc(TYPE_TEST_OBJECT, sizeof(struct test_object));
        mu_assert_msg(obj != NULL, "obj_alloc failed\n");

        cap = cap_alloc(root_cap_group, obj);
        mu_assert_msg(cap >= 0, "cap_alloc failed, ret = %d\n", cap);

        /* revoke all */
        cap_copied = cap_copy(root_cap_group, root_cap_group, cap, CAP_RIGHT_REVOKE_ALL, CAP_RIGHT_NO_RIGHTS);
        mu_assert_msg(cap_copied >= 0, "cap_copy failed, ret = %d\n", cap_copied);

        ret = sys_revoke_cap(cap_copied, true);
        mu_assert_msg(ret == -ECAPBILITY, "sys_revoke_cap should return -ECAPBILITY, ret = %d\n", ret);

        ret = sys_revoke_cap(cap_copied, false);
        mu_assert_msg(ret == 0, "sys_revoke_cap failed, ret = %d\n", ret);

        /* copy */
        cap_copied = cap_copy(root_cap_group, root_cap_group, cap, CAP_RIGHT_COPY, CAP_RIGHT_NO_RIGHTS);
        mu_assert_msg(cap_copied >= 0, "cap_copy failed, ret = %d\n", cap_copied);

        ret = cap_copy(root_cap_group, root_cap_group, cap_copied, CAP_RIGHT_COPY, CAP_RIGHT_NO_RIGHTS);
        mu_assert_msg(ret == -ECAPBILITY, "cap_copy should return -ECAPBILITY, ret = %d\n", ret);

        ret = sys_revoke_cap(cap_copied, false);
        mu_assert_msg(ret == 0, "sys_revoke_cap failed, ret = %d\n", ret);

        ret = cap_free(root_cap_group, cap);
        mu_assert_msg(ret == 0, "free cap failed, ret = %d\n", ret);
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_check);
}

obj_deinit_func obj_test_deinit_tbl[TYPE_TEST_NR] = {
        [0 ... TYPE_TEST_NR - 1] = NULL,
};

static void prepare_obj_deinit_tbl(void)
{
        memcpy(obj_test_deinit_tbl, obj_deinit_tbl, sizeof(obj_deinit_tbl));
}

int main(void)
{
        prepare_obj_deinit_tbl();
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_fail;
}
