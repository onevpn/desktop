FROM ubuntu:16.04

ENV ONEVPN_DIR=/var/onevpn
ENV QT_DIR=/var/qt
ENV QT_VERSION_A=5.9
ENV QT_VERSION_B=5.9.4
ENV QT_VERSION_SCRIPT=594
# ENV XDG_RUNTIME_DIR=/run/user/1000

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --fix-missing \
    libfontconfig1-dev libfreetype6-dev libx11-dev xorg-dev libxrandr-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
    libxcb1 libxcb1-dev libx11-xcb-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev \
    libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0 libxcb-shape0-dev libxcb-randr0 libxcb-randr0-dev \
    libxcb-render-util0-dev openssl libcrypto++-dev libssl-dev libgl1-mesa-dev build-essential clang cmake \
    libjasper-dev libudev-dev xcb xvfb libxcb-xkb1 libxcb-xkb-dev libxcb-xtest0-dev \
    libxcb-xinerama0 libxcb-xinerama0-dev libdbus-1-3 libclang-dev perl python git wget

##ADD ./ ${ONEVPN_DIR}
WORKDIR ${ONEVPN_DIR}

##Первый вариант
## Get QT
RUN mkdir ${QT_DIR} && cd ${QT_DIR} \
    && wget https://download.qt.io/archive/qt/${QT_VERSION_A}/${QT_VERSION_B}/qt-opensource-linux-x64-${QT_VERSION_B}.run \
    && chmod +x qt-opensource-linux-x64-${QT_VERSION_B}.run
COPY docker/qt-noninteractive.qs ${QT_DIR}/qt-noninteractive.qs
RUN cd ${QT_DIR} ./qt-opensource-linux-x64-${QT_VERSION_B}.run --script qt-noninteractive.qs --platform minimal
#./qt-opensource-linux-x64-5.9.4.run --script qt-noninteractive.qs --platform minimal


## Второй вариант
## Get QT
RUN cd /var/onevpn && chmod +x docker/clone_qt.sh && ./docker/clone_qt.sh
#ADD ./docker/qt5-src /var/qt5
##Build
RUN cd /var/qt5 && \
   rm config.cache
   OPENSSL_LIBS="-llibssl -llibcrypto" ./configure -prefix /var/qt5-bin -release -opensource -static -static-runtime -confirm-license -openssl -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -nomake examples -nomake tests -skip qtwebengine
   rm config.cache \
   && ./configure -prefix /var/qt5-bin -release -opensource -static -static-runtime -confirm-license \
   && -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -qt-xcb -qt-xkbcommon -nomake examples -nomake tests -skip qtwebengine \
   && make && make install