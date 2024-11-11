#include <circle/machineinfo.h>
#include <circle/bcm2835.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/cputhrottle.h>

#include <SDCard/emmc.h>

#include <stdio.h>
#include <stdlib.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/cpu_driver_defs.h>

#include "../circle.hpp"

static void set_cpu_speed(ipc_msg_t *ipc_msg, struct cpu_driver_msg *req)
{
        int ret = 0;
        switch (req->set_speed_req.speed) {
        case CPU_MAXSPEED:
                if (CCPUThrottle::Get()->SetSpeed(CPUSpeedMaximum, TRUE)
                    == CPUSpeedUnknown) {
                        ret = -EAGAIN;
                }
                break;

        case CPU_LOWSPEED:
                if (CCPUThrottle::Get()->SetSpeed(CPUSpeedLow, TRUE)
                    == CPUSpeedUnknown) {
                        ret = -EAGAIN;
                }
                break;
        default:
                ret = -EINVAL;
                break;
        }
        ipc_return(ipc_msg, ret);
}

static void cpu_get_clockrate(ipc_msg_t *ipc_msg, struct cpu_driver_msg *req)
{
        int ret = 0;
        switch (req->get_clockrate_req.type) {
        case CLOCKRATE_CURRENT:
                if ((req->get_clockrate_req.clock_rate =
                             CCPUThrottle::Get()->GetClockRate())
                    == 0) {
                        ret = -EAGAIN;
                }
                break;
        case CLOCKRATE_MAX:
                req->get_clockrate_req.clock_rate =
                        CCPUThrottle::Get()->GetMaxClockRate();
                break;
        case CLOCKRATE_MIN:
                req->get_clockrate_req.clock_rate =
                        CCPUThrottle::Get()->GetMinClockRate();
                break;
        default:
                ret = -EINVAL;
                break;
        }
        ipc_return(ipc_msg, ret);
}

static void cpu_get_temperature(ipc_msg_t *ipc_msg, struct cpu_driver_msg *req)
{
        int ret = 0;
        switch (req->get_temperature_req.type) {
        case TEMPERATURE_CURRENT:
                if ((req->get_temperature_req.temperature =
                             CCPUThrottle::Get()->GetTemperature())
                    == 0) {
                        ret = -EAGAIN;
                }
                break;
        case TEMPERATURE_MAX:
                req->get_temperature_req.temperature =
                        CCPUThrottle::Get()->GetMaxTemperature();
                break;
        default:
                ret = -EINVAL;
                break;
        }
        ipc_return(ipc_msg, ret);
}

static void cpu_dump(ipc_msg_t *ipc_msg, struct cpu_driver_msg *req)
{
        CCPUThrottle::Get()->DumpStatus(true, req->dump_cpu_req.cpu_info);
        ipc_return(ipc_msg, 0);
}

DEFINE_SERVER_HANDLER(cpu_dispatcher)
{
        struct cpu_driver_msg *req =
                (struct cpu_driver_msg *)ipc_get_msg_data(ipc_msg);
        switch (req->req) {
        case CPU_SET_SPEED:
                set_cpu_speed(ipc_msg, req);
                break;
        case CPU_GET_CLOCKRATE:
                cpu_get_clockrate(ipc_msg, req);
                break;
        case CPU_GET_TEMPERATURE:
                cpu_get_temperature(ipc_msg, req);
                break;
        case DUMP_CPU_CURRENT_INFO:
                cpu_dump(ipc_msg, req);
                break;
        default:
                info("Bad CPU req %d, please retry.\n", req->req);
        }
        ipc_return(ipc_msg, -EINVAL);
}

void *cpu(void *args)
{
        int *thread_ret = (int *)malloc(sizeof(int));
        *thread_ret = 0;
        __attribute__((unused)) CCPUThrottle *m_CPUThrottle = new CCPUThrottle;
        *thread_ret = ipc_register_server(cpu_dispatcher,
                                          DEFAULT_CLIENT_REGISTER_HANDLER);
        return thread_ret;
}
