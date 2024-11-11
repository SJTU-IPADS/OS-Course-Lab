#include "circle.hpp"
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>
#include <circle/bcm2835.h>
#include <circle/bcm2711.h>
#include <circle/sched/scheduler.h>

#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/pthread.h>
#include <chcore/error.h>

pthread_mutex_t *init_lock;

struct CircleBase {
        CInterruptSystem m_Interrupt;
        CTimer m_Timer;
        CLogger m_Logger;
        CDeviceNameService m_DeviceNameService;
        CMachineInfo m_MachineInfo;
#ifdef CHCORE_DRIVER_WLAN
        CScheduler scheduler;
#endif
        CKernelOptions m_KernelOptions;
        CircleBase(void);
        boolean Initialize(void);
};

CircleBase::CircleBase(void)
        : m_Timer(&m_Interrupt)
        , m_Logger(LogWarning, &m_Timer)
{
}

boolean CircleBase::Initialize(void)
{
        boolean bOK = TRUE;
        if (bOK) {
                bOK = m_Logger.Initialize(0);
        }

        if (bOK) {
                bOK = m_Interrupt.Initialize();
        }

        if (bOK) {
                bOK = m_Timer.Initialize();
        }

        return bOK;
}

static void init_io_mapping()
{
        int ret;
        cap_t io_pmo_cap = usys_create_device_pmo(
                ARM_IO_BASE, ROUND_UP(ARM_IO_END - ARM_IO_BASE, PAGE_SIZE));
        ret = usys_map_pmo(
                SELF_CAP, io_pmo_cap, ARM_IO_BASE, VM_READ | VM_WRITE);
        BUG_ON(ret < 0);
}


void bind_to_cpu(int cpu)
{
        int ret;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        assert(ret == 0);
        usys_yield();
}

/* Create a server thread, and register it in srvmgr. Servers are booted
 * serially. */
static pthread_t create_server_thread(const char *name, void *(*func)(void *),
                                      const char* service_name)
{
        struct proc_request *proc_req;
        struct ipc_msg *ipc_msg;
        pthread_t tid;
        cap_t thread_cap;
        int ret;

        /* Create server thread and get its thread cap. */
        void *thread_result;
        thread_cap = chcore_pthread_create(&tid, NULL, func, NULL);
        pthread_join(tid, &thread_result);
        ret = *(int *)thread_result;
        free(thread_result);
        if (ret) {
                error("%s server initialize failed with errno %d\n", name, ret);
                return -1;
        }

        /* Register the server in srvmgr. */
        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof(*proc_req));
        proc_req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        proc_req->req = PROC_REQ_SET_SERVICE_CAP;
        strncpy(proc_req->set_service_cap.service_name, service_name,
                SERVICE_NAME_LEN);
        ipc_set_msg_send_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, thread_cap);
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        if (ret < 0) {
                error("create %s server failed. error num is %d\n", name, ret);
        } else {
                info("%s server created with cap %d\n", name, thread_cap);
        }
        ipc_destroy_msg(ipc_msg);
        return tid;
}

int main(int argc, char *argv[])
{
        info("Drivers intializing\n");
        init_io_mapping();
        CircleBase *base = new CircleBase();
        base->Initialize();

#ifdef CIRCLE_AUDIO
        create_server_thread("audio", audio, SERVER_AUDIO);
#endif // CIRCLE_AUDIO

#ifdef CIRCLE_CPU
        create_server_thread("cpu driver", cpu, SERVER_CPU_DRIVER);
#endif // CIRCLE_CPU

#ifdef CIRCLE_GPIO
        create_server_thread("gpio", gpio, SERVER_GPIO);
#endif // CIRCLE_GPIO

#ifdef CIRCLE_SERIAL
        create_server_thread("serial", serial, SERVER_SERIAL);
#endif // CIRCLE_SERIAL

#ifdef CIRCLE_USB
        create_server_thread("usb mouse and keyboard",
                             usb_mouse_and_keyboard,
                             SERVER_USB_DEVMGR);
#endif // CIRCLE_USB

#ifdef CIRCLE_SD
        create_server_thread("sd", sdcard, SERVER_SD_CARD);
#endif // CIRCLE_SD

#ifdef CIRCLE_ETHERNET
        bcm54213_start();
#endif // CIRCLE_ETHERNET

#ifdef CIRCLE_WLAN
        wlan_start();
#endif
        info("Initialization finished! \n");

        fflush(stdout);
        usys_exit(0);
}