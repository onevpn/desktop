FROM ubuntu:16.04

ENV MAKE_THREADS_CNT=-j8
ENV ONEVPN_DIR=/var/onevpn
ENV QT_DIR=/var/qt
ENV QT_DIR_PREFIX=/opt/qt
ENV OPENSSL_DIR=/var/openssl
ENV OPENSSL_DIR_PREFIX=/opt/openssl
ENV XKBCOMMON_DIR=/var/xkbcommon
ENV VCPKG_DIR=/var/vcpkg
ENV QT_VERSION_BRANCH=v5.12.5

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --fix-missing \
    build-essential clang cmake git wget autoconf xutils-dev libtool byacc bison flex gperf \
    ibgstreamer-plugins-base0.10-dev libasound2-dev libatkmm-1.6-dev libbz2-dev libcap-dev libcups2-dev \
    libdrm-dev libegl1-mesa-dev libfontconfig1-dev libfreetype6-dev libgcrypt11-dev libglu1-mesa-dev \
    libgstreamer0.10-dev libicu-dev libnss3-dev libpci-dev libpulse-dev libssl-dev libudev-dev libx11-dev \
    libx11-xcb-dev libxcb-composite0 libxcb-composite0-dev libxcb-cursor-dev libxcb-cursor0 libxcb-damage0 \
    libxcb-damage0-dev libxcb-dpms0 libxcb-dpms0-dev libxcb-dri2-0 libxcb-dri2-0-dev libxcb-dri3-0 libxcb-dri3-dev \
    libxcb-ewmh-dev libxcb-ewmh2 libxcb-glx0 libxcb-glx0-dev libxcb-icccm4 libxcb-icccm4-dev libxcb-image0 \
    libxcb-image0-dev libxcb-keysyms1 libxcb-keysyms1-dev libxcb-present-dev libxcb-present0 libxcb-randr0 \
    libxcb-randr0-dev libxcb-record0 libxcb-record0-dev libxcb-render-util0 libxcb-render-util0-dev libxcb-render0 \
    libxcb-render0-dev libxcb-res0 libxcb-res0-dev libxcb-screensaver0 libxcb-screensaver0-dev libxcb-shape0 \
    libxcb-shape0-dev libxcb-shm0 libxcb-shm0-dev libxcb-sync-dev libxcb-sync0-dev libxcb-sync1 libxcb-util-dev \
    libxcb-util0-dev libxcb-util1 libxcb-xevie0 libxcb-xevie0-dev libxcb-xf86dri0 libxcb-xf86dri0-dev libxcb-xfixes0 \
    libxcb-xfixes0-dev libxcb-xinerama0 libxcb-xinerama0-dev libxcb-xkb-dev libxcb-xkb1 libxcb-xprint0 \
    libxcb-xprint0-dev libxcb-xtest0 libxcb-xtest0-dev libxcb-xv0 libxcb-xv0-dev libxcb-xvmc0 libxcb-xvmc0-dev \
    libxcb1 libxcb1-dev libxcomposite-dev libxcursor-dev libxdamage-dev libxext-dev libxfixes-dev libxi-dev \
    libxrandr-dev libxrender-dev libxslt-dev libxss-dev libxtst-dev libdbus-1-3 libharfbuzz-bin perl python ruby \
    curl unzip tar software-properties-common

WORKDIR ${ONEVPN_DIR}

## Build openssl
RUN git clone https://github.com/openssl/openssl ${OPENSSL_DIR} \
    && cd ${OPENSSL_DIR} \
    && git checkout OpenSSL_1_1_1-stable \
    && ./config --prefix=${OPENSSL_DIR_PREFIX} -Wl,--enable-new-dtags,-rpath,'$(LIBRPATH)' \
    && make $MAKE_THREADS_CNT \
    && make install_sw \
    && echo '/opt/openssl/lib' >> /etc/ld.so.conf \
    && cat /etc/ld.so.conf \
    && ldconfig \
    && echo 'export LD_LIBRARY_PATH=/opt/openssl/lib' >> ~/.bash_profile && . ~/.bash_profile

## vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_DIR} && cd ${VCPKG_DIR} \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y && apt-get update -y && apt-get install g++-7 -y \
    && ./bootstrap-vcpkg.sh && ./vcpkg integrate install && ./vcpkg install harfbuzz pthread icu pcre2 glib

## Build QT
RUN git clone git://code.qt.io/qt/qt5.git ${QT_DIR} \
    && cd ${QT_DIR} && git checkout ${QT_VERSION_BRANCH} \
    && git submodule update --init --recursive \
    && cd ${QT_DIR} && ./configure -static -release -silent -prefix ${QT_DIR_PREFIX} -opensource \
    -confirm-license -opengl -nomake examples -nomake tests \
    -skip wayland -skip purchasing -skip serialbus -skip qtserialport -skip script -skip scxml -skip speech -skip qtwebengine \
    -plugindir ${QT_DIR}/qtbase/src/plugins/platformthemes/gtk3 -qt-pcre -qt-xcb -qt-libpng -qt-libjpeg -qt-zlib -qt-pcre \
    -qt-freetype -qt-harfbuzz -openssl-linked \
    OPENSSL_LIBS="-L${OPENSSL_DIR_PREFIX}/lib -lssl -lcrypto" -I ${OPENSSL_DIR_PREFIX}/include -L ${OPENSSL_DIR_PREFIX}/lib -pulseaudio -alsa \
    && make $MAKE_THREADS_CNT && make install

## Build project via cmake
RUN cd ${ONEVPN_DIR} && rm -rf build && mkdir build && cd build \
    && cmake --target install --config Release -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake -DQt5Core_DIR=${QT_DIR_PREFIX}/lib/cmake/Qt5Core -DQt5Network_DIR=${QT_DIR_PREFIX}/lib/cmake/Qt5Network -DQt5Widgets_DIR=${QT_DIR_PREFIX}/lib/cmake/Qt5Widgets ../ \
    && make $MAKE_THREADS_CNT

## Build project via qmake
RUN cd ${ONEVPN_DIR} && rm -rf qmake-build && mkdir qmake-build && cd qmake-build \
    ${QT_DIR_PREFIX}/bin/qmake ${ONEVPN_DIR}/src/OneVPN.pro && make $MAKE_THREADS_CNT

#-lpthread -licu -lpcre2 -lglib -lssl -lcrypto -lpng -lharfbuzz