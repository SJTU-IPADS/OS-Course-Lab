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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lidar_sdk.h>
#include <serial_messenger.h>

struct dev_desc *lidar_connect(const char *dev_name)
{
        int ret;
        struct dev_desc *dev_desc;
        struct serial_ctrl_data serial_ctrl_data;

        if ((dev_desc = dm_open(dev_name, 0)) == NULL) {
                printf("Error occurs when openning device.\n");
                return NULL;
        }

        if ((ret = dm_ctrl(dev_desc, SERIAL_OPEN, NULL, 0))) {
                dm_close(dev_desc);
                return NULL;
        }

        serial_ctrl_data.set = 6;
        serial_ctrl_data.clear = 0;
        if ((ret = dm_ctrl(dev_desc, SERIAL_TIOCMSET, &serial_ctrl_data, 0))) {
                dm_close(dev_desc);
                return NULL;
        }

        serial_ctrl_data.set = 0;
        serial_ctrl_data.clear = 2;
        if ((ret = dm_ctrl(dev_desc, SERIAL_TIOCMSET, &serial_ctrl_data, 0))) {
                dm_close(dev_desc);
                return NULL;
        }

        checkMotorCtrlSupport(dev_desc);
        stopMotor(dev_desc);

        return dev_desc;
}

int lidar_disconnect(struct dev_desc *dev_desc)
{
        int ret;

        if ((ret = dm_close(dev_desc))) {
                printf("lidar_disconnect failed.\n");
                return ret;
        }

        return ret;
}

void setDTR(struct dev_desc *dev_desc)
{
        struct serial_ctrl_data serial_ctrl_data;
        serial_ctrl_data.set = 2;
        serial_ctrl_data.clear = 0;
        dm_ctrl(dev_desc, SERIAL_TIOCMSET, &serial_ctrl_data, 0);
}

void clearDTR(struct dev_desc *dev_desc)
{
        struct serial_ctrl_data serial_ctrl_data;
        serial_ctrl_data.set = 0;
        serial_ctrl_data.clear = 2;
        dm_ctrl(dev_desc, SERIAL_TIOCMSET, &serial_ctrl_data, 0);
}

int stopMotor(struct dev_desc *dev_desc)
{
        // RPLIDAR A1
        setDTR(dev_desc);
        return 0;
}

int startMotor(struct dev_desc *dev_desc)
{
        // RPLIDAR A1
        clearDTR(dev_desc);
        return 0;
}

int sendCommand(struct dev_desc *dev_desc, u8 cmd, const void *payload,
                size_t payloadsize)
{
        u8 pkt_header[10];
        rplidar_cmd_packet_t *header = (rplidar_cmd_packet_t *)pkt_header;
        u8 checksum = 0;

        if (payloadsize && payload) {
                if (payloadsize && payload) {
                        cmd |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
                }
        }
        header->syncByte = RPLIDAR_CMD_SYNC_BYTE;
        header->cmd_flag = cmd;

        // send header first
        dm_ctrl(dev_desc, SERIAL_WRITE, pkt_header, 2);

        // send payload if any
        if (cmd & RPLIDAR_CMDFLAG_HAS_PAYLOAD) {
                checksum ^= RPLIDAR_CMD_SYNC_BYTE;
                checksum ^= cmd;
                checksum ^= (payloadsize & 0xFF);

                // calc checksum
                for (size_t pos = 0; pos < payloadsize; ++pos) {
                        checksum ^= ((u8 *)payload)[pos];
                }

                // send size
                u8 sizebyte = payloadsize;
                dm_ctrl(dev_desc, SERIAL_WRITE, &sizebyte, 1);

                // send payload
                dm_ctrl(dev_desc, SERIAL_WRITE, (void *)payload, sizebyte);

                // send checksum
                dm_ctrl(dev_desc, SERIAL_WRITE, &checksum, 1);
        }

        return 0;
}

u32 checkMotorCtrlSupport(struct dev_desc *dev_desc)
{
        u32 flag = 0;
        sendCommand(
                dev_desc, RPLIDAR_CMD_GET_ACC_BOARD_FLAG, &flag, sizeof(flag));
        return 0;
}

int getDevInfo(struct dev_desc *dev_desc,
               rplidar_response_device_info_t *devinfo)
{
        rplidar_ans_header_t *response_header;
        u8 *buffer = malloc(32);
        u8 *dev_buffer;

        sendCommand(dev_desc, RPLIDAR_CMD_GET_DEVICE_INFO, NULL, 0);

        dm_ctrl(dev_desc, SERIAL_READ, buffer, 27);
        response_header = (rplidar_ans_header_t *)buffer;

        while (response_header->type != RPLIDAR_ANS_TYPE_DEVINFO) {
                // Give a second chance
                sendCommand(dev_desc, RPLIDAR_CMD_GET_DEVICE_INFO, NULL, 0);
                dm_ctrl(dev_desc, SERIAL_READ, buffer, 27);
                response_header = (rplidar_ans_header_t *)buffer;
        }

        // verify whether we got a correct header
        if (response_header->type != RPLIDAR_ANS_TYPE_DEVINFO) {
                printf("func: %s, line:%d get a wrong header\n",
                       __func__,
                       __LINE__);
                return -1;
        }

        u32 header_size = (response_header->size_q30_subtype
                           & RPLIDAR_ANS_HEADER_SIZE_MASK);
        if (header_size < sizeof(rplidar_response_device_info_t)) {
                printf("func: %s, line:%d header_size < sizeof(rplidar_response_device_info_t)\n",
                       __func__,
                       __LINE__);
                return -1;
        }

        dev_buffer = buffer + sizeof(rplidar_ans_header_t);
        for (int i = 0; i < sizeof(rplidar_response_device_info_t); ++i) {
                ((u8 *)devinfo)[i] = dev_buffer[i];
        }
        free(buffer);
        return 0;
}

int getHealth(struct dev_desc *dev_desc,
              rplidar_response_device_health_t *health_info)
{
        rplidar_ans_header_t *response_header;
        u8 *buffer = malloc(16);
        u8 *health_buffer;

        sendCommand(dev_desc, RPLIDAR_CMD_GET_DEVICE_HEALTH, NULL, 0);
        dm_ctrl(dev_desc, SERIAL_READ, buffer, 10);
        response_header = (rplidar_ans_header_t *)buffer;

        if (response_header->type != RPLIDAR_ANS_TYPE_DEVHEALTH) {
                // Give a second chance
                sendCommand(dev_desc, RPLIDAR_CMD_GET_DEVICE_HEALTH, NULL, 0);
                dm_ctrl(dev_desc, SERIAL_READ, buffer, 10);
                response_header = (rplidar_ans_header_t *)buffer;
        }

        // verify whether we got a correct header
        if (response_header->type != RPLIDAR_ANS_TYPE_DEVHEALTH) {
                printf("func: %s, line:%d get a wrong header\n",
                       __func__,
                       __LINE__);
                return -1;
        }

        u32 header_size = (response_header->size_q30_subtype
                           & RPLIDAR_ANS_HEADER_SIZE_MASK);
        if (header_size < sizeof(rplidar_response_device_health_t)) {
                printf("func: %s, line:%d header_size < sizeof(rplidar_response_device_info_t)\n",
                       __func__,
                       __LINE__);
                return -1;
        }

        health_buffer = buffer + sizeof(rplidar_ans_header_t);
        for (int i = 0; i < sizeof(rplidar_response_device_health_t); ++i) {
                ((u8 *)health_info)[i] = health_buffer[i];
        }
        free(buffer);
        return 0;
}

void stop(struct dev_desc *dev_desc)
{
        sendCommand(dev_desc, RPLIDAR_CMD_STOP, NULL, 0);
}

void plot_histogram(rplidar_response_measurement_node_t *nodes, size_t count)
{
        const int BARCOUNT = 75;
        const int MAXBARHEIGHT = 20;
        const float ANGLESCALE = 360.0f / BARCOUNT;

        float histogram[BARCOUNT];
        for (int pos = 0; pos < _countof(histogram); ++pos) {
                histogram[pos] = 0.0f;
        }

        float max_val = 0;
        for (int pos = 0; pos < (int)count; ++pos) {
                int int_deg = (int)((nodes[pos].angle_q6_checkbit
                                     >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
                                    / 64.0f / ANGLESCALE);
                if (int_deg >= BARCOUNT)
                        int_deg = 0;
                float cachedd = histogram[int_deg];
                if (cachedd == 0.0f) {
                        cachedd = nodes[pos].distance_q2 / 4.0f;
                } else {
                        cachedd = (nodes[pos].distance_q2 / 4.0f + cachedd)
                                  / 2.0f;
                }

                if (cachedd > max_val)
                        max_val = cachedd;
                histogram[int_deg] = cachedd;
        }

        for (int height = 0; height < MAXBARHEIGHT; ++height) {
                float threshold_h =
                        (MAXBARHEIGHT - height - 1) * (max_val / MAXBARHEIGHT);
                for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
                        if (histogram[xpos] >= threshold_h) {
                                putc('*', stdout);
                        } else {
                                putc(' ', stdout);
                        }
                }
                printf("\n");
        }
        for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
                putc('-', stdout);
        }
        printf("\n");
}

int compareNode(const void *nodebuffer1, const void *nodebuffer2)
{
        nodebuffer1 = (rplidar_response_measurement_node_t *)nodebuffer1;
        nodebuffer2 = (rplidar_response_measurement_node_t *)nodebuffer2;
        return (getAngle(nodebuffer1) < getAngle(nodebuffer2)) ? -1 : 1;
}

int ascendScanData(rplidar_response_measurement_node_t *nodebuffer,
                   size_t count)
{
        // return 0;
        float inc_origin_angle = 360.0f / count;
        size_t i = 0;

        // Tune head
        for (i = 0; i < count; i++) {
                if (getDistanceQ2(&nodebuffer[i]) == 0) {
                        continue;
                } else {
                        while (i != 0) {
                                i--;
                                float expect_angle =
                                        getAngle(&nodebuffer[i + 1])
                                        - inc_origin_angle;
                                if (expect_angle < 0.0f)
                                        expect_angle = 0.0f;
                                if (expect_angle > 360.0f)
                                        printf("[warning] expect_angle > 360\n");
                                setAngle(&nodebuffer[i], expect_angle);
                        }
                        break;
                }
        }

        // all the data is invalid
        if (i == count)
                return -1;

        // Tune tail
        for (i = count - 1; i >= 0; i--) {
                if (getDistanceQ2(&nodebuffer[i]) == 0) {
                        continue;
                } else {
                        while (i != (count - 1)) {
                                i++;
                                float expect_angle =
                                        getAngle(&nodebuffer[i - 1])
                                        + inc_origin_angle;
                                if (expect_angle > 360.0f)
                                        expect_angle -= 360.0f;
                                if (expect_angle > 360.0f)
                                        printf("[warning] expect_angle > 360\n");
                                setAngle(&nodebuffer[i], expect_angle);
                        }
                        break;
                }
        }

        // Fill invalid angle in the scan
        float frontAngle = getAngle(&nodebuffer[0]);
        for (i = 1; i < count; i++) {
                if (getDistanceQ2(&nodebuffer[i]) == 0) {
                        float expect_angle = frontAngle + i * inc_origin_angle;
                        if (expect_angle > 360.0f)
                                expect_angle -= 360.0f;
                        if (expect_angle > 360.0f)
                                printf("[warning] expect_angle > 360\n");
                        setAngle(&nodebuffer[i], expect_angle);
                }
        }

        qsort(nodebuffer,
              count,
              sizeof(rplidar_response_measurement_node_t),
              compareNode);
        return 0;
}

int waitScanData(struct dev_desc *dev_desc,
                 rplidar_response_measurement_node_t *nodebuffer, size_t count)
{
        size_t recvNodeCount = 0;
        int recvSize = 0;
        u8 *recvBuffer = malloc(256);
        u8 buffer[sizeof(rplidar_response_measurement_node_t)];

        while (recvNodeCount < count) {
                recvSize = dm_ctrl(dev_desc, SERIAL_READ, recvBuffer, 256);

                int recvPos = 0;
                for (size_t pos = 0; pos < recvSize; ++pos) {
                        u8 currentByte = recvBuffer[pos];
                        switch (recvPos) {
                        case 0: // expect the sync bit and its reverse in this
                                // byte
                        {
                                u8 tmp = (currentByte >> 1);
                                if ((tmp ^ currentByte) & 0x1) {
                                        // pass
                                } else {
                                        continue;
                                }

                        } break;
                        case 1: // expect the highest bit to be 1
                        {
                                if (currentByte
                                    & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) {
                                        // pass
                                } else {
                                        recvPos = 0;
                                        continue;
                                }
                        } break;
                        }
                        buffer[recvPos++] = currentByte;

                        if (recvPos
                            == sizeof(rplidar_response_measurement_node_t)) {
                                rplidar_response_measurement_node_t *node =
                                        (rplidar_response_measurement_node_t *)
                                                buffer;
                                if ((node->sync_quality
                                     >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT)
                                    >= 15) {
                                        nodebuffer[recvNodeCount] = *node;
                                        recvNodeCount++;
                                        if (recvNodeCount == count)
                                                return 0;
                                }
                                recvPos = 0;
                        }
                }
        }
        return 0;
}

int cacheScanData(struct dev_desc *dev_desc)
{
        rplidar_response_measurement_node_t test_buf[128];
        size_t count = 360;
        rplidar_response_measurement_node_t local_buf[count];

        // rplidar_response_measurement_node_hq_t local_scan[MAX_SCAN_NODES];
        // size_t scan_count = 0;
        // int ret;
        memset(test_buf, 0, sizeof(test_buf));
        memset(local_buf, 0, sizeof(local_buf));
        // memset(local_scan, 0, sizeof(local_scan));

        waitScanData(dev_desc,
                     test_buf,
                     128); // always discard the first data since it may be
                           // incomplete
        waitScanData(dev_desc, local_buf, count);

        printf("sizeof localbuf:%ld\n", sizeof(local_buf));
        for (int pos = 0; pos < count; ++pos) {
                if (getAngle(&local_buf[pos]) > 360.0f)
                        printf("find a angle > 360 before ascendScanData\n");
        }
        ascendScanData(local_buf, count);
        plot_histogram(local_buf, count);
        printf("Do you want to see all the data? (y/n) ");
        // int key = getchar();
        if (/*key == 'Y' || key == 'y'*/ 1) {
                printf("\n*************************************************\n");
                for (int pos = 0; pos < count; ++pos) {
                        printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
                               (local_buf[pos].sync_quality
                                & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ?
                                       "S " :
                                       "  ",
                               (local_buf[pos].angle_q6_checkbit
                                >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
                                       / 64.0f,
                               local_buf[pos].distance_q2 / 4.0f,
                               local_buf[pos].sync_quality
                                       >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
                }
        }

        return 0;
}

/// Start scan
///
/// \param force            Force the core system to output scan data regardless
/// whether the scanning motor is rotating or not. \param useTypicalScan   Use
/// lidar's typical scan mode or use the compatibility mode (2k sps) \param
/// options          Scan options (please use 0) \param outUsedScanMode  The
/// scan mode selected by lidar
int startScan(struct dev_desc *dev_desc, int force)
{
        u8 *buffer = malloc(64);
        memset(buffer, 0, 64);

        // stop(dev_desc); //force the previous operation to stop

        sendCommand(dev_desc,
                    force ? RPLIDAR_CMD_FORCE_SCAN : RPLIDAR_CMD_SCAN,
                    NULL,
                    0);

        // waiting for confirmation
        rplidar_ans_header_t *response_header;
        dm_ctrl(dev_desc, SERIAL_READ, buffer, 64);

        response_header = (rplidar_ans_header_t *)buffer;

        // verify whether we got a correct header
        if (response_header->type != RPLIDAR_ANS_TYPE_MEASUREMENT) {
                printf("func: %s, line:%d get a wrong header\n",
                       __func__,
                       __LINE__);
                return -1;
        }

        u32 header_size = (response_header->size_q30_subtype
                           & RPLIDAR_ANS_HEADER_SIZE_MASK);
        if (header_size < sizeof(rplidar_response_measurement_node_t)) {
                printf("func: %s, line:%d invalid data\n", __func__, __LINE__);
                return -1;
        }

        return cacheScanData(dev_desc);
}