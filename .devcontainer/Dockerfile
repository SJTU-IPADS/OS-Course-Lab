FROM ipads/chcore_builder:v1.9.0

ARG USERNAME=stu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN sed -i "s/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g" /etc/apt/sources.list
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y sudo python3 python3-pip python3-psutil bear qemu-user qemu-system-aarch64 cmake git gdb-multiarch clangd
RUN useradd -m -u $USER_UID -U -s /bin/bash stu

USER $USERNAME
CMD ["/bin/bash"]
