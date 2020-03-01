#!/usr/bin/env bash

PROJECT_ROOT=/var/cpp
## some code, add this later
SFML_ROOT=/var/SFML
DOCKER_BUILD_PATH=${PROJECT_ROOT}/cmake-build-docker

if [ ! -d "${DOCKER_BUILD_PATH}" ]; then
  mkdir ${DOCKER_BUILD_PATH}
fi
cd ${DOCKER_BUILD_PATH}
cmake -DSFML_DIR=${SFML_ROOT}/install/lib/cmake/SFML -DSFML_INCLUDE_DIR=${SFML_ROOT}/install/include -DSFML_LIBRARY_DIR=${SFML_ROOT}/install/lib ..
make
make test

cd ${PROJECT_ROOT}
