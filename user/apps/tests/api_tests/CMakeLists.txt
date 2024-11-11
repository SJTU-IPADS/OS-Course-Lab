# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_custom_target(
    api_dir ALL
    WORKING_DIRECTORY ${CHCORE_RAMDISK_DIR}
    # add directory for api tests
    COMMAND [ ! -d "api_tests" ] && mkdir api_tests || true
)

add_subdirectory(memory_tests)
add_subdirectory(data_structure_tests)
add_subdirectory(process_tests)

chcore_copy_all_targets_to_ramdisk_api_tests()