#!/usr/bin/env bash

QT5_PWD=$(echo $(pwd))
QT5_ROOT=${QT5_PWD}/docker/qt5-src
QT5_LOCAL_ROOT=~/.local/qt5

if [ ! -d "${QT5_ROOT}" ]; then
  if [ ! -d "${QT5_LOCAL_ROOT}" ]; then
    git clone git://code.qt.io/qt/qt5.git ${QT5_LOCAL_ROOT} && cd ${QT5_LOCAL_ROOT} \
    && git checkout 5.15 && git submodule update --init --recursive && cp -R ${QT5_LOCAL_ROOT} ${QT5_ROOT}
  else
    cp -R ${QT5_LOCAL_ROOT} ${QT5_ROOT}
  fi
fi