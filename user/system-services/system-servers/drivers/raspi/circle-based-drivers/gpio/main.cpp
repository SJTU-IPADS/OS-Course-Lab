#include <circle/bcm2835.h>
#include <circle/gpiopin.h>
#include <circle/machineinfo.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/gpio_defs.h>
#include "../circle.hpp"

struct GPIODriver {
        CGPIOPin gpio[GPIO_PINS + 1];

        GPIODriver()
        {
                for (int i = 1; i <= GPIO_PINS; i++) {
                        gpio[i].AssignPin(i);
                }
        }

        static GPIODriver *get()
        {
                static GPIODriver *driver = nullptr;
                if (!driver) {
                        driver = new GPIODriver;
                }
                return driver;
        }
};

DEFINE_SERVER_HANDLER(ipc_dispatch)
{
        struct gpio_request *msg;
        int ret = 0;

        msg = (struct gpio_request *)ipc_get_msg_data(ipc_msg);
        if (msg->npin > GPIO_PINS) {
                ret = -1;
                printf("Incorrect pin number! Please retry!\n");
                goto out;
        }
        switch (msg->req) {
        case SET_MODE:
                GPIODriver::get()->gpio[msg->npin].SetMode((TGPIOMode)msg->opt);
                break;
        case SET_PULL_MODE:
                GPIODriver::get()->gpio[msg->npin].SetPullMode(
                        (TGPIOPullMode)msg->opt);
                break;
        case SET_PIN_LEVEL:
                GPIODriver::get()->gpio[msg->npin].Write(msg->opt);
                break;
        default:
                ret = -1;
                printf("Incorrect gpio req! Please retry!");
        }

out:
        ipc_return(ipc_msg, ret);
}

void *gpio(void *args)
{
        int *thread_ret = (int *)malloc(sizeof(int));
        int ret = ipc_register_server(
                reinterpret_cast<server_handler>(ipc_dispatch),
                register_cb_single);
        *thread_ret = ret;
        return thread_ret;
}
