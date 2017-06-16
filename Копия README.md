### OneVPN OpenVPN wrapper ###

## Windows ##

Download

* [VS Express 2015](https://www.microsoft.com/ru-ru/download/details.aspx?id=34673)
* [Qt 5.5.1 for Windows 32-bit (VS 2012, 747 MB)](http://download.qt.io/official_releases/qt/5.5/5.5.1/qt-opensource-windows-x86-msvc2012-5.5.1.exe.mirrorlist) 

Add project files and build  

## Linux ##

* [Download QT](http://www.qt.io/download-open-source/)

### How to build static libraries under linux ###


* sudo apt-get install libfontconfig1-dev libfreetype6-dev libx11-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libx11-xcb-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev openssl libssl-dev libgl1-mesa-dev build-essential libjasper-dev libxcb-xinerama0-dev
* cd "Путь к исходникам Qt"
* ./configure -static -prefix "Путь к исходникам Qt" -nomake examples -nomake tests
* убедиться что есть поддержка openssl и fontconfig, иначе доставить либы и перезапустить configure
* make -jn 
n - thread number
* Static libs are built
* Go to Tools => Options in QT Creator. Config to use static libs. Then => Build => Release

Binary file ready

### Helper compilation ###

* cd ./Linux/helper/
* gcc onevpnhelper.c 
onevpnhelper.c: In function ‘main’:
onevpnhelper.c:19:5: warning: incompatible implicit declaration of built-in function ‘sprintf’ [enabled by default]
     sprintf(cmd, "openvpn --config %s --management 127.0.0.1 %s --management-query-passwords", configPath, port);
* ./helper/a.out ./onevpn-pkg/onevpn_1.0/usr/bin/onevpnhelper


./configure -static -prefix ~/Qt/5.8/static -opensource -confirm-license -openssl-linked -qt-zlib -qt-libjpeg -qt-libpng -qt-xcb -qt-xkbcommon -qt-freetype -nomake examples -nomake tests -skip qtwebkit -skip qtserialport -skip

Ready to make package

* sudo chown -R root:root onevpn-pkg/onevpn_1.0
* sudo chmod 4775 onevpn-pkg/onevpn_1.0/usr/bin/onevpnhelper
* sudo vim ./onevpn-pkg/onevpn_1.0/DEBIAN/control
* dpkg-deb --build onevpn-pkg/onevpn_1.0