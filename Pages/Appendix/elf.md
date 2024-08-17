# ELF 文件格式

在Lab1中ChCore 的构建系统将会构建出 `build/kernel.img` 文件，该文件是一个 ELF 格式的“可执行目标文件”，和我们平常在 Linux 系统中见到的可执行文件如出一辙。ELF 可执行文件以 ELF 头部（ELF header）开始，后跟几个程序段（program segment），每个程序段都是一个连续的二进制块，其中又包含不同的分段（section），加载器（loader）将它们加载到指定地址的内存中并赋予指定的可读（R）、可写（W）、可执行（E）权限，并从入口地址（entry point）开始执行。

可以通过 `aarch64-linux-gnu-readelf` 命令查看 `build/kernel.img` 文件的 ELF 元信息（比如通过 `-h` 参数查看 ELF 头部、`-l` 参数查看程序头部、`-S` 参数查看分段头部等）：

```sh
$ aarch64-linux-gnu-readelf -h build/kernel.img
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           AArch64
  Version:                           0x1
  Entry point address:               0x80000
  Start of program headers:          64 (bytes into file)
  Start of section headers:          271736 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         4
  Size of section headers:           64 (bytes)
  Number of section headers:         15
  Section header string table index: 14
```

更多关于 ELF 格式的细节请参考 [ELF - OSDev Wiki](https://wiki.osdev.org/ELF)。

