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

#include <arch/sync.h>
#include <irq/ipi.h>
#include <machine.h>
#include <common/kprint.h>

struct ipi_data {
        /* Grab this lock before writing to this ipi_data */
        struct lock lock;

        /* finish <- 1: the ipi_data (arguments) is handled */
        volatile u16 finish;

        /* The IPI vector */
        u32 vector;

        unsigned long args[IPI_DATA_ARG_NUM];
};

/* IPI data shared among all the CPUs */
static struct ipi_data ipi_data[PLAT_CPU_NUM];

/* Invoked once during the kernel boot */
void init_ipi_data(void)
{
        int i;

        for (i = 0; i < PLAT_CPU_NUM; ++i) {
                lock_init(&ipi_data[i].lock);
                ipi_data[i].finish = 1;
        }
}

/*
 * Interfaces for inter-cpu communication (named IPI_transaction).
 * IPI-based message sending.
 */

/* Lock the target buffer */
void prepare_ipi_tx(u32 target_cpu)
{
        struct ipi_data *data_target, *data_self;

        data_target = &(ipi_data[target_cpu]);
        data_self = &(ipi_data[smp_get_cpu_id()]);

        /*
         * Handle IPI tx while waiting to avoid deadlock.
         *
         * Deadlock example:
         * CPU 0: in prepare_ipi_tx(), waiting for ipi_data[1]->lock;
         * CPU 1: in prepare_ipi_tx(), waiting for ipi_data[0]->lock;
         * CPU 2: in wait_finish_ipi_tx(), holding ipi_data[0]->lock,
         *        waiting for CPU 0 to finish an IPI tx;
         * CPU 3: in wait_finish_ipi_tx(), holding ipi_data[1]->lock,
         *        waiting for CPU 1 to finish an IPI tx.
         */
        while (try_lock(&data_target->lock)) {
                if (!data_self->finish) {
                        handle_ipi();
                }
        }
}

/* Set argments */
void set_ipi_tx_arg(u32 target_cpu, u32 arg_index, unsigned long val)
{
        ipi_data[target_cpu].args[arg_index] = val;
}

/*
 * Start IPI-based transaction (tx).
 *
 * ipi_vector can be encoded into the physical interrupt (as IRQ number),
 * which can be used to implement some fast-path (simple) communication.
 *
 * Nevertheless, we support sending information with IPI.
 * So, actually, we can use one ipi_vector to distinguish different IPIs.
 */
void start_ipi_tx(u32 target_cpu, u32 ipi_vector)
{
        struct ipi_data *data;

        data = &(ipi_data[target_cpu]);

        /* Set ipi_vector */
        data->vector = ipi_vector;

        smp_mb();

        /* Mark the arguments are ready (set_ipi_tx_arg before) */
        data->finish = 0;

        smp_mb();

        /* Send physical IPI to interrupt the target CPU */
        arch_send_ipi(target_cpu, ipi_vector);
}

/* Wait and unlock */
void wait_finish_ipi_tx(u32 target_cpu)
{
        /*
         * It is possible that core-A is waiting for core-B to finish one IPI
         * while core-B is also waiting for core-A to finish one IPI.
         * So, this function will polling on the IPI data of both the target
         * core and the local core, namely data_target and data_self.
         */
        struct ipi_data *data_target, *data_self;

        data_target = &(ipi_data[target_cpu]);
        data_self = &(ipi_data[smp_get_cpu_id()]);

        /* Avoid dead lock by checking and handling ipi request while waiting */
        while (!data_target->finish) {
                if (!data_self->finish) {
                        handle_ipi();
                }
        }

        unlock(&(data_target->lock));
}

/* Send IPI tx without argument */
void send_ipi(u32 target_cpu, u32 ipi_vector)
{
        prepare_ipi_tx(target_cpu);
        start_ipi_tx(target_cpu, ipi_vector);
        wait_finish_ipi_tx(target_cpu);
}

/* Receiver side interfaces */

#define ipi_data_self (&ipi_data[smp_get_cpu_id()])

/* Get argments */
unsigned long get_ipi_tx_arg(u32 arg_index)
{
        return ipi_data_self->args[arg_index];
}

/* Handle IPI tx */
void handle_ipi(void)
{
        struct ipi_data *data = ipi_data_self;

        /* The IPI tx may have been completed in wait_finish_ipi_tx() */
        if (data->finish) {
                return;
        }

        arch_handle_ipi(data->vector);

        data->finish = 1;
}
