
vcpkg install boost-iostreams:x86-windows-static boost-asio:x86-windows-static boost-beast:x86-windows-static boost-system:x86-windows-static boost-variant:x86-windows-static boost-lockfree:x86-windows-static boost-process:x86-windows-static boost-program-options:x86-windows-static luajit:x86-windows-static glew:x86-windows-static boost-filesystem:x86-windows-static boost-uuid:x86-windows-static physfs:x86-windows-static openal-soft:x86-windows-static libogg:x86-windows-static libvorbis:x86-windows-static zlib:x86-windows-static libzip:x86-windows-static openssl:x86-windows-static

Since version 1.1.0 OpenSSL have changed their library names from: libeay32.dll -> libcrypto.dll ssleay32.dll -> libssl.dll
So you need to change it in vc14/settings.props

on linux boost >=1.67 and libzip-dev, physfs >= 3
also gcc >=9, because otcv8 uses c++17
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install gcc-9 g++-9
Then just mkdir build && cd build && cmake .. && make -j8

To compile on android you need to create C:\android with
- android-ndk-r21b https://dl.google.com/android/repository/android-ndk-r21d-windows-x86_64.zip
- libs from http://otclient.ovh/android_libs.7z

Also install android extension for visual studio
In visual studio go to options -> cross platform -> c++ and set Android NDK to C:\android\android-ndk-r21b
Right click on otclientv8 -> proporties -> general and change target api level to android-25

Put data.zip in android/otclientv8/assets
You can use powershell script create_android_assets.ps1 to create them automaticly (won't be encrypted)

