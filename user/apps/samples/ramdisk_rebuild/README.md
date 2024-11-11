## How to use chcore toolchain to build a project

#### Prepare

- run `./chbuild build` in chcore working directory to build chcore kernel 
- generate the `toolchain.cmake` configure file in the build directory `$cmake_build_dir`

#### Execution

1. set curretn directory as working directory
2. To use chcore toolchain, please run 
```
export CMAKE_TOOLCHAIN_FILE=${path to toolchain.cmake}
```
or set `-DCMAKE_TOOLCHAIN_FILE`flag when run cmake command
```
-DCMAKE_TOOLCHAIN_FILE=${path to toolchain.cmake}
``` 

3. run the following command to compile the source
```
mkdir build
cd build
cmake ..     // or use %cmake -DCMAKE_TOOLCHAIN_FILE=${path to toolchain.cmake} ..
make
```

## How to output the compiled objects into `chcore` to run

In `toolchain.cmake`, the variable `CHCORE_RAMDISK_INSTALL_PREFIX` is set to the path of the chcore ramdisk directory.

You can set the variable `EXECUTABLE_OUTPUT_PATH` in the project's CMake file to `CHCORE_RAMDISK_INSTALL_PREFIX`, or use the install command in CMake to specify the installation destination as `CHCORE_RAMDISK_INSTALL_PREFIX` to output the compiled objects to the chcore ramdisk folder.

#### Execution

1. In this sample project, we have already specified the installation destination as `CHCORE_RAMDISK_INSTALL_PREFIX` in cmake file, so you can directly run make install in the build folder.
```
make install
```
2. Go back to the chcore working directory, and run `./chbuild rambuild` to repack the ramdisk, and regenerate kernel image.
```
./chbuild rambuild
```
