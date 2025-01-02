# OTClientV8

## Contribution

If you add custom feature, make sure it's optional and can be enabled via g_game.enableFeature function, otherwise your pull request will be rejected.

## Compilation

### Windows

Use Visual Studio 2022, select backend (OpenGL, DirectX), platform (x86, x64) and just build, all required libraries will be installed for you.

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
