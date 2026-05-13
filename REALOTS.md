# RealOTS Client Configuration

This checkout is configured for the local Tibia stack at:

```text
Login: 192.168.68.109:7171
Game:  192.168.68.109:7172
Version: 770
Account: 111111
Password: tibia
```

Before login, add Tibia 7.7 client assets:

```text
data/things/770/Tibia.dat
data/things/770/Tibia.spr
```

The client has been configured with the RSA public key matching
`tibia-stack/game/tibia.pem`.

## Build on Zorin/Ubuntu

```sh
sudo apt update
sudo apt install git curl build-essential cmake gcc g++ pkg-config autoconf libtool libglew-dev -y
cd /home/zakaria
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd /home/zakaria/Documents/RealOTS/otclientv8
/home/zakaria/vcpkg/vcpkg install
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/home/zakaria/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j"$(nproc)"
cp build/otclient ./otclient
./otclient
```

## Build on Windows with vcpkg

Install Visual Studio 2022 or Build Tools 2022 with:

- Desktop development with C++
- MSVC v143 toolset
- Windows 10/11 SDK

Then open PowerShell in this directory and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build-windows-vcpkg.ps1 -Configuration OpenGL -Platform x64
```

To build both OpenGL and DirectX:

```powershell
.\build-windows-vcpkg.ps1 -Configuration Both -Platform x64
```

The packaged client will be created at:

```text
dist\realots-client-windows
```

Choose the `RealOTS` server entry, or manually use:

```text
192.168.68.109:7171:770
```
