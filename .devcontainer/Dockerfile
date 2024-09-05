FROM ipads/chcore_builder:v1.9.0

ARG USERNAME=stu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN sed -i "s/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g" /etc/apt/sources.list
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y sudo \
					binutils-dev \
					libgmp-dev \
					libmpfr-dev \
					curl \
					meson \
					python3 \
					python3-pip \
					python3-psutil \
					bear \
					cmake \
					git \
					pkgconf \
					clangd

# We need to install a latest gdb to support cppdbg.
RUN curl -SLO https://ftp.gnu.org/gnu/gdb/gdb-13.2.tar.xz && \
	tar -xf gdb-13.2.tar.xz && \
	cd gdb-13.2 && \
	./configure --prefix=/usr/local \
				--enable-targets=aarch64-linux-gnu,x86_64-linux-gnu \
				--enable-tui \
				--disable-sim \
				&& \
	make -j$(nproc) && \
	make install && \
	cd .. && \
	rm -rf gdb-13.2 gdb-13.2.tar.xz

# install tomli for .toml file required by qemu 9.1.x
RUN pip install tomli

# we need to install >=glib-2.66.8 to support qemu 9.1.x
RUN curl -SLO https://download.gnome.org/sources/glib/2.66/glib-2.66.8.tar.xz && \
	tar -xvf glib-2.66.8.tar.xz && \
	cd  glib-2.66.8 && \
	meson build --prefix /usr/local && \
	ninja -C build && \
	ninja -C build install && \
	cd .. && \
	rm -rf glib-2.66.8 glib-2.66.8.tar.xz

# install latest supported qemu supported by ubuntu-20.04.
RUN curl -SLO https://download.qemu.org/qemu-9.1.0.tar.xz && \
	tar -xf qemu-9.1.0.tar.xz && \
	cd qemu-9.1.0 && \
	./configure --prefix=/usr/local \
				--target-list=aarch64-softmmu,aarch64-linux-user && \
	make -j$(nproc) && \
	make install && \
	cd .. && \
	rm -rf qemu-9.1.0 qemu-9.1.0.tar.xz

RUN apt-get install -y libglib2.0-dev
RUN apt-get clean

RUN useradd -m -u $USER_UID -U -s /bin/bash stu
RUN usermod -aG sudo stu
RUN passwd -d stu
USER $USERNAME
CMD ["/bin/bash"]
