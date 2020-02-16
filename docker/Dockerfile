FROM ubuntu:16.04

RUN apt-get update \
    #&& apt-get upgrade -y \
    && apt-get install -y --fix-missing \
    libfontconfig1-dev libfreetype6-dev libx11-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
    libxcb1-dev libx11-xcb-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev \
    libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev \
    libxcb-render-util0-dev openssl libssl-dev libgl1-mesa-dev build-essential clang cmake \
    #libjasper-dev \
    libxcb-xinerama0-dev perl python git

ADD ./ /var/onevpn
WORKDIR /var/onevpn
# Get QT
#RUN cd /var/openvpn && chmod +x docker/clone_qt.sh
RUN cd /var/onevpn && chmod +x docker/clone_qt.sh && ./docker/clone_qt.sh
ADD ./docker/qt5-src /var/qt5
#Build
RUN cd /var/onevpn && \
    ./configure -static -prefix /var/qt5 -opensource -confirm-license -openssl-linked \
    -qt-zlib -qt-libjpeg -qt-libpng -qt-xcb -qt-xkbcommon -qt-freetype -nomake examples \
    -nomake tests -skip qtwebkit -skip qtwebkit-examples