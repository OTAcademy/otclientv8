# OTClientV8

## Contribution

If you add custom feature, make sure it's optional and can be enabled via g_game.enableFeature function, otherwise your pull request will be rejected.

## Compilation

### Automatic

You can clone repoistory and use github action build-on-request workload.

### Windows

You need visual studio 2022 and vcpkg with commit `389e18e8380daab3884e7dc0710ad7f4a264def6` ([download](https://github.com/microsoft/vcpkg/archive/389e18e8380daab3884e7dc0710ad7f4a264def6.zip)).

Then you install vcpkg dependencies:

```bash
vcpkg install boost-iostreams:x86-windows-static boost-asio:x86-windows-static boost-beast:x86-windows-static boost-system:x86-windows-static boost-variant:x86-windows-static boost-lockfree:x86-windows-static boost-process:x86-windows-static boost-program-options:x86-windows-static luajit:x86-windows-static glew:x86-windows-static boost-filesystem:x86-windows-static boost-uuid:x86-windows-static physfs:x86-windows-static openal-soft:x86-windows-static libogg:x86-windows-static libvorbis:x86-windows-static zlib:x86-windows-static libzip:x86-windows-static openssl:x86-windows-static
```

and then you can compile static otcv8 version.

### Linux

on linux you need:

- vcpkg from commit `389e18e8380daab3884e7dc0710ad7f4a264def6`
- boost >=1.67 and libzip-dev, physfs >= 3
- gcc >=9

Then just run mkdir build && cd build && cmake .. -DUSE_STATIC_LIBS=OFF && make -j8

NOTICE: project comes with USE_STATIC_LIBS=ON set while libzip-dev is supplied with dynamic library only, so for linux build it is required to override USE_STATIC_LIBS to OFF

### Android

To compile on android you need to create `C:\android` with

- Android SDK 25
- Android NDK r21d
- Apache Ant 1.9
- Content of android_libs.7z (`C:\android\lib`, `C:\android\lib64`, `C:\android\include`)

SDK, NDK and Ant can be downloaded [here](https://drive.google.com/drive/folders/1jLnqB4zYqz3j3s9g3TraZdJQDOdlW7aM?usp=sharing)

Also install `Mobile development with C++` using Visual Studio Installer

Then open `android/otclientv8.sln`, open Tools -> Options -> Cross Platform -> C++ -> Android and:

- Set Android SDK to `C:\android\25`
- Set Android NDK to `C:\android\android-ndk-r21d`
- Set Apache Ant to `C:\android\apache-ant-1.9.16`
- Put data.zip in `android/otclientv8/assets`
- Select Release and ARM64
- Build `otclientv8` (the one with phone icon, not folder)

## Useful tips

- To run tests manually, unpack tests.7z and use command `otclient_debug.exe --test`
- To test mobile UI use command `otclient_debug.exe --mobile`
