# OTClientV8

## Compilation

### Linux

#### Docker (Via Ubuntu 22.04)

```
make compile
```

#### Ubuntu 22.04

```
sudo apt update
sudo apt install git curl build-essential cmake gcc g++ pkg-config autoconf libtool libglew-dev -y
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh && cd ..
git clone https://github.com/tibia-oce/otclientv8.git
cd otclientv8 && mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake .. && make -j$(nproc)
cp otclient ../otclient && cd ..
./otclient
```

### Windows

#### Visual Studio 2022

Install vcpkg

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg.exe integrate install
```

Use Visual Studio 2022, select backend (OpenGL, DirectX), platform (x86, x64) and just build, all required libraries will be installed for you.
