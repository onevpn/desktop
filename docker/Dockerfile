FROM ubuntu:16.04

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --fix-missing \
    libfontconfig1-dev libfreetype6-dev libx11-dev xorg-dev libxrandr-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
    libxcb1 libxcb1-dev libx11-xcb-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev \
    libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0 libxcb-shape0-dev libxcb-randr0 libxcb-randr0-dev \
    libxcb-render-util0-dev openssl libcrypto++-dev libssl-dev libgl1-mesa-dev build-essential clang cmake \
    libjasper-dev libudev-dev xcb xvfb libxcb-xkb1 libxcb-xkb-dev libxcb-xtest0-dev \
    libxcb-xinerama0 libxcb-xinerama0-dev perl python git

##ADD ./ /var/onevpn
WORKDIR /var/onevpn
## Get QT
RUN cd /var/onevpn && chmod +x docker/clone_qt.sh && ./docker/clone_qt.sh
##ADD ./docker/qt5-src /var/qt5
##Build

#RUN cd /var/qt5 && \
#    ./configure -static -prefix /var/qt5-bin -opensource -confirm-license -openssl-linked \
#    -qt-zlib -qt-libjpeg -qt-libpng -qt-xcb -qt-xkbcommon -qt-freetype -nomake examples \
#    -nomake tests -skip qtwebkit -skip qtwebkit-examples

#RUN cd /var/qt5 && \
#    OPENSSL_LIBS="-llibssl -llibcrypto" \
#    ./configure -prefix /var/qt5-bin -release -opensource -static -static-runtime -confirm-license \
#    -openssl-linked -qt-xcb -qt-xmbcommon  \
#    -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -nomake examples -nomake tests -skip qtwebengine
RUN cd /var/qt5 && ./configure -prefix /var/qt5-bin -release -opensource -static -static-runtime -confirm-license \
    -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -nomake examples -nomake tests -skip qtwebengine \
    make && make install
