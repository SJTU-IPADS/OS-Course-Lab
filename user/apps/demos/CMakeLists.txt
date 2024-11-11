# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

if(CHCORE_SERVER_GUI)
        add_subdirectory(black_square)
        # add_subdirectory(poker_slide)
        add_subdirectory(lvgl)
endif()

add_user_executable(capture_image_demo.bin capture_image_demo.c)
target_link_libraries(capture_image_demo.bin PRIVATE dm)

add_user_executable(car_demo.bin car_demo.c)
target_link_libraries(car_demo.bin PRIVATE dm)

add_user_executable(lidar_demo.bin lidar_demo.c)
target_link_libraries(lidar_demo.bin PRIVATE lidar_sdk)
chcore_copy_all_targets_to_ramdisk()