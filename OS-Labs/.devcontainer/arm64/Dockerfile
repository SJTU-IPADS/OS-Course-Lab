# OS-Course-Lab toolchain image that builds natively on arm64 (and on amd64).
#
# This follows the build history stored in the image config of
# ipads/oslab:25.03 on Docker Hub (`docker history --no-trunc`):
# Ubuntu 20.04, gcc 9.4 (Ubuntu's gcc 9.3 package), cmake 3.16, gdb 13.2,
# glib 2.66.8 and QEMU 9.1.2 with the ChCore compatibility patch. The tools/ and patches/
# folders next to this file are byte-identical copies of /tools and
# /patches in that image.
#
# Differences from ipads/oslab:25.03:
#   - no platform pin, so `docker build` produces an image for the host;
#     on arm64 the native gcc 9.3 is already the aarch64-linux-gnu compiler;
#   - the apt mirror is a build argument instead of a fixed sed, and the
#     repo tool falls back to the Tsinghua mirror;
#   - build dependencies that the original image got indirectly are listed
#     explicitly (ncurses, expat, libffi, libmount, pixman ...);
#   - the riscv64 cross compiler and LLVM 16 are used when available; if a
#     package is missing it is skipped with a message, but a failed install
#     still fails the build;
#   - clangd comes from the same LLVM release as clang-tidy, and wget is not
#     purged at the end;
#   - two fixes so that Labs 3 to 6 also build in a folder bind-mounted
#     through Colima's virtiofs file sharing on macOS: GNU sed 4.8 is built
#     into /usr/local, and read_procmgr_elf_tool gets a mode for the file it
#     creates (see the two steps below).
#
# Build:  docker build -t oslab:25.03-native .
# China:  docker build --build-arg APT_MIRROR=mirrors.tuna.tsinghua.edu.cn -t oslab:25.03-native .

FROM ubuntu:20.04

ARG USERNAME=stu
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG APT_MIRROR=

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN if [ -n "$APT_MIRROR" ]; then \
        sed -i -e "s|archive.ubuntu.com|$APT_MIRROR|g" \
               -e "s|ports.ubuntu.com|$APT_MIRROR|g" /etc/apt/sources.list; \
    fi

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y ca-certificates wget gnupg curl

# Base toolchain, same pins as the original image.
RUN apt-get install -y \
        cmake=3.16.* cmake-curses-gui=3.16.* make ninja-build cpio bear \
        binutils binutils-aarch64-linux-gnu \
        gcc=4:9.3.* g++=4:9.3.* \
        sudo binutils-dev libgmp-dev libmpfr-dev meson \
        python3 python3-pip python3-pexpect python3-dev git pkgconf \
        xz-utils patch libncurses-dev libexpat1-dev \
        libffi-dev zlib1g-dev libpcre3-dev libmount-dev libpixman-1-dev && \
    arch="$(dpkg --print-architecture)" && \
    if [ "$arch" != arm64 ]; then \
        apt-get install -y gcc-aarch64-linux-gnu=4:9.3.* g++-aarch64-linux-gnu=4:9.3.*; \
    fi && \
    if apt-cache show gcc-riscv64-linux-gnu > /dev/null 2>&1; then \
        apt-get install -y binutils-riscv64-linux-gnu \
            gcc-riscv64-linux-gnu=4:9.3.* g++-riscv64-linux-gnu=4:9.3.*; \
    else \
        echo "riscv64 cross toolchain is not available on $arch, skipped"; \
    fi && \
    aarch64-linux-gnu-gcc --version && aarch64-linux-gnu-objcopy --version | head -n 1

# clang-tidy and clangd: LLVM 16 like the original image if apt.llvm.org still
# serves focal, otherwise the LLVM 12 packages from Ubuntu.
RUN if wget -qO /tmp/llvm.key https://apt.llvm.org/llvm-snapshot.gpg.key && \
       apt-key add /tmp/llvm.key && \
       echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-16 main" \
           > /etc/apt/sources.list.d/llvm-16.list && \
       apt-get update && apt-cache show clang-tidy-16 > /dev/null 2>&1; then \
        llvm=16; \
    else \
        echo "could not use apt.llvm.org for LLVM 16, using LLVM 12 from Ubuntu"; \
        rm -f /etc/apt/sources.list.d/llvm-16.list && apt-get update; \
        llvm=12; \
    fi && \
    rm -f /tmp/llvm.key && \
    apt-get install -y clang-tidy-$llvm clangd-$llvm && \
    ln -sf /usr/bin/clang-tidy-$llvm /usr/bin/clang-tidy && \
    ln -sf /usr/bin/clangd-$llvm /usr/bin/clangd

RUN ln -sf /usr/bin/make /usr/bin/gmake && pip3 install tomli

# glib >= 2.66 is needed by QEMU 9.1; Ubuntu 20.04 ships 2.64.
RUN curl -fSLO https://download.gnome.org/sources/glib/2.66/glib-2.66.8.tar.xz && \
    tar -xf glib-2.66.8.tar.xz && cd glib-2.66.8 && \
    meson build --prefix /usr && ninja -C build && ninja -C build install && \
    cd .. && rm -rf glib-2.66.8 glib-2.66.8.tar.xz

COPY patches /opt/oslab/patches
RUN curl -fSLO https://download.qemu.org/qemu-9.1.2.tar.xz && \
    tar -xf qemu-9.1.2.tar.xz && cd qemu-9.1.2 && \
    patch -p1 < /opt/oslab/patches/qemu/0001-compat-chcore.patch && \
    ./configure --prefix=/usr --target-list=aarch64-softmmu,aarch64-linux-user && \
    make -j"$(nproc)" && make install && \
    cd .. && rm -rf qemu-9.1.2 qemu-9.1.2.tar.xz

RUN curl -fSLO https://ftp.gnu.org/gnu/gdb/gdb-13.2.tar.xz && \
    tar -xf gdb-13.2.tar.xz && cd gdb-13.2 && \
    ./configure --prefix=/usr \
        --enable-targets=aarch64-linux-gnu,x86_64-linux-gnu \
        --enable-tui --disable-sim && \
    make -j"$(nproc)" && make install && \
    cd .. && rm -rf gdb-13.2 gdb-13.2.tar.xz

# GNU sed 4.8. The sed 4.7 of Ubuntu 20.04 creates the temporary file of
# `sed -i` with mode 000 (sed sets umask 0700 while creating it; fixed in
# 4.8, whose only bug fix this is). Colima's virtiofs file sharing on macOS
# does not open such a file, so the libc patch step of Labs 3 to 5 and the
# `make` of Lab 6 stop with "couldn't open temporary file". /usr/local/bin
# comes before /usr/bin in PATH, so the build scripts use this sed.
RUN curl -fSLO --retry 3 https://ftp.gnu.org/gnu/sed/sed-4.8.tar.xz && \
    echo "f79b0cfea71b37a8eeec8490db6c5f7ae7719c35587f21edb0617f370eeff633  sed-4.8.tar.xz" | \
        sha256sum -c - && \
    tar -xf sed-4.8.tar.xz && cd sed-4.8 && \
    ./configure --prefix=/usr/local --disable-acl && \
    make -j"$(nproc)" && make install && \
    cd .. && rm -rf sed-4.8 sed-4.8.tar.xz

# read_procmgr_elf_tool calls open() with O_CREAT but no mode, so
# elf_info.temp gets whatever mode bits happen to be in a register: 000 in
# ipads/oslab and in an arm64 build of the same source. Colima's virtiofs
# file sharing does not open such a file, and the procmgr step of Labs 3 to
# 6 stops with "chmod: cannot access 'elf_info.temp'". The tool is built
# with the mode that the labs' CMake rule sets right afterwards with
# chmod 666; the copy in tools/ is unchanged.
COPY tools /opt/oslab/tools
RUN cp -r /opt/oslab/tools/read_procmgr_elf_tool /tmp/rpet && \
    sed -i 's/O_CREAT | O_RDWR);/O_CREAT | O_RDWR, 0666);/' /tmp/rpet/main.c && \
    grep -q 'O_CREAT | O_RDWR, 0666);' /tmp/rpet/main.c && \
    gcc /tmp/rpet/*.c -o /usr/bin/read_procmgr_elf_tool && rm -rf /tmp/rpet && \
    gcc -static /opt/oslab/tools/chcompress/*.c -o /usr/bin/chcompress

# repo: from Google, or from the Tsinghua mirror that Scripts/chbuild also
# uses, since storage.googleapis.com cannot be reached from mainland China.
RUN (curl -fSL https://storage.googleapis.com/git-repo-downloads/repo -o /usr/local/bin/repo || \
     curl -fSL https://mirrors.tuna.tsinghua.edu.cn/git/git-repo -o /usr/local/bin/repo) && \
    chmod +x /usr/local/bin/repo && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Same users as the original image: stu (no password, sudo) and runner
# (UID 1001, used by the CI workflows with `--user 1001`).
RUN groupadd -g $USER_GID $USERNAME && \
    useradd -m -u $USER_UID -g $USER_GID -s /bin/bash $USERNAME && \
    usermod -aG sudo $USERNAME && passwd -d $USERNAME && \
    useradd -m -u 1001 -U -s /bin/bash runner && usermod -aG sudo runner

CMD ["/bin/bash"]
