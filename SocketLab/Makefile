#
# Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
# Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
# use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
# Mulan PSL v2 for more details.
#

.PHONY: clean build grade-part1 grade-part2 grade-part3 grade-part4 \
	grade prepare-part4 website middleware

build:
	cmake -S . -B build
	cmake --build build -- -j$(nproc)

clean:
	cmake --build build --target clean

grade-part1:
	python3 grade/client_test.py

grade-part2:
	python3 grade/server_test.py

grade-part3:
	python3 grade/epoll_test.py

grade-part4:
	$(MAKE) -C grade grade-nodejs

grade:
	python3 grade/grade.py

prepare-part4:
	$(MAKE) -C grade prepare

website:
	$(MAKE) -C frontend website

middleware:
	$(MAKE) -C frontend middleware
