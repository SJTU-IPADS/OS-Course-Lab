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

#include <lidar_sdk.h>
#include <stdio.h>

int main()
{
        int ret;
        struct dev_desc *dev_desc;
        rplidar_response_device_info_t devinfo;

        printf("Ultra simple LIDAR data grabber for RPLIDAR.\n");

        dev_desc = lidar_connect("CP210X-0");
        printf("successfully open CP210X-0\n");

        if (dev_desc == NULL) {
                printf("lidar_connect failed.\n");
                return 0;
        }

        ret = getDevInfo(dev_desc, &devinfo);
        if (ret) {
                printf("getDevInfo failed.\n");
                return 0;
        }

        // print out the device serial number, firmware and hardware version
        // number..
        printf("RPLIDAR S/N: ");
        for (int pos = 0; pos < 16; ++pos) {
                printf("%02X", devinfo.serialnum[pos]);
        }

        printf("\n"
               "Firmware Ver: %d.%02d\n"
               "Hardware Rev: %d\n",
               devinfo.firmware_version >> 8,
               devinfo.firmware_version & 0xFF,
               (int)devinfo.hardware_version);

        rplidar_response_device_health_t healthinfo = {0};
        ret = getHealth(dev_desc, &healthinfo);
        if (ret) {
                printf("getHealth failed.\n");
                return 0;
        }
        printf("RPLidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
                printf("Error, rplidar internal error detected. Please reboot the device to retry.\n");
                return -1;
        }

        startMotor(dev_desc);

        startScan(dev_desc, 0);
        stop(dev_desc);
        stopMotor(dev_desc);

        lidar_disconnect(dev_desc);
        return 0;
}
