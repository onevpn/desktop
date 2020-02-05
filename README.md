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

https://wiki.qt.io/Building_Qt_5_from_Git
git clone git://code.qt.io/qt/qt5.git
cd qt5
git checkout v5.6.0
git submodule update --init --recursive

* cd "Путь к исходникам Qt"
./configure -static -prefix /*путь к Qt*/ -opensource -confirm-license -openssl-linked -qt-zlib -qt-libjpeg -qt-libpng -qt-xcb -qt-xkbcommon -qt-freetype -nomake examples -nomake tests -skip qtwebkit -skip qtwebkit-examples

* install qtcreator
sudo apt-get -y install openjdk-7-jre qtcreator build-essential

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

Ready to make package

* sudo chown -R root:root onevpn-pkg/onevpn_1.0
* sudo chmod 4775 onevpn-pkg/onevpn_1.0/usr/bin/onevpnhelper
* sudo vim ./onevpn-pkg/onevpn_1.0/DEBIAN/control
* dpkg-deb --build onevpn-pkg/onevpn_1.0

Mac codesign
codesign --deep --force --sign "Some certificate" ./OneVPN.app
productsign --sign "Some certificate" ./OneVPN.pkg ./OneVPN1.pkg // перед этим приложение должно быть подписано

чтобы проверить подпись pkg
pkgutil --check-signature ./OneVPN1.pkg 