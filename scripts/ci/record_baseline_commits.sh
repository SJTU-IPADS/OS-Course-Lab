#!/bin/sh
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

manifests_path=".repo/manifests"
kernel_path="kernel"
chcore_libs_path="user/chcore-libs"
system_services_path="user/system-services"
apps_path="user/apps"

cd $manifests_path
git pull
git checkout baseline_record
git pull
cd -
staros_commit=$(git log -n 1 | grep "commit")
cd $kernel_path
kernel_commit=$(git log -n 1 | grep "commit")
cd -
cd $chcore_libs_path
chcore_libs_commit=$(git log -n 1 | grep "commit")
cd -
cd $system_services_path
system_services_commit=$(git log -n 1 | grep "commit")
cd -
cd $apps_path
apps_commit=$(git log -n 1 | grep "commit")
cd -

filename=$(date +%Y%m%d)".xml"
cat>$filename<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <remote name="staros" fetch="."/>

  <default remote="staros"/>

  <project name="staros.git" path="." revision="${staros_commit#*commit }"/>
  <project name="starkernel.git" path="kernel" revision="${kernel_commit#*commit }"/>
  <project name="staros-apps.git" path="user/apps" revision="${apps_commit#*commit }"/>
  <project name="staros-libs.git" path="user/chcore-libs" revision="${chcore_libs_commit#*commit }"/>
  <project name="staros-system-services.git" path="user/system-services" revision="${system_services_commit#*commit }"/>
    <linkfile src="chcore-libc" dest="user/chcore-libc"/>
  </project>
</manifest>
EOF

mv $filename ./$manifests_path/$filename
cd $manifests_path
git add $filename
git commit -m "CI record baseline $(date +%Y%m%d)"
git push --set-upstream origin baseline_record 
