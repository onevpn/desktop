#!/bin/bash

##Variables
if [ "${OS}" == "Windows_NT" ]; then
    OS_TYPE='windows';
    DOCKER_COMPOSE_CMD='winpty docker-compose'
    DOCKER_CMD='winpty docker'
elif [ "${OSTYPE}" == "linux-gnu" ]; then
    OS_TYPE='linux';
    DOCKER_COMPOSE_CMD='docker-compose'
    DOCKER_CMD='docker'
else
    OS_TYPE='macos';
    DOCKER_COMPOSE_CMD='docker-compose'
    DOCKER_CMD='docker'
fi
## Constants
GREEN_OUTPUT='\033[0;32m'
RED_OUTPUT='\033[0;31m'
YELLOW_OUTPUT='\033[1;33m'
CLEAR_OUTPUT='\033[0m'
## Colored output
pc() {
    RES=""
    case "$1" in
        green)
            RES="${GREEN_OUTPUT}$2${CLEAR_OUTPUT}"
            ;;

        red)
            RES="${RED_OUTPUT}$2${CLEAR_OUTPUT}"
            ;;

        yellow)
            RES="${YELLOW_OUTPUT}$2${CLEAR_OUTPUT}"
            ;;

        *)
            RES="${CLEAR_OUTPUT}$2${CLEAR_OUTPUT}"
            ;;

    esac

    printf "${RES}"
}

fix_display_command() {
  if [ "${OS_TYPE}" == "linux" ]; then
    xhost +
  fi
}

forget_host_command() {
  if [ -f "~/.ssh/known_hosts" ]; then
    sed -i "/localhost/d" ~/.ssh/known_hosts
  fi
}

own_commands() {
    eval ${DOCKER_COMPOSE_CMD} exec dev $1 $2 $3 $4 $5 $6 $7 $8
}

general_help() {
  pc "green" "--build | -b   "
  pc "none" " - build docker container \n"
  pc "green" "--run | -r     "
  pc "none" " - start current docker container \n"
  pc "green" "--down | -d    "
  pc "none" " - stop current active docker container \n"
  pc "green" "--own | -o     "
  pc "none" " - run your own command in container \n"
  pc "green" "--forget-host  "
  pc "none" " - forget current docker host (only for UNIX based os or bash shell) \n"
  if [ "${OS_TYPE}" == "linux" ]; then
    pc "green" "--fix-display  "
    pc "none" " - try to change display output (only for X11) \n"
  fi
  pc "green" "--test | -t    "
  pc "none" " - run tests on docker container \n"
  pc "green" "--delete | -D  "
  pc "none" " - delete all docker container \n"
}

case "$1" in

    --build|-b)
        eval ${DOCKER_COMPOSE_CMD} up -d --build
        fix_display_command
        forget_host_command
        ;;
    --run|-r)
        eval ${DOCKER_COMPOSE_CMD} up -d
        fix_display_command
        ;;
    --down|-d)
        eval ${DOCKER_COMPOSE_CMD} down
        ;;
    --forget-host)
        forget_host_command
        ;;
    --fix-display)
        fix_display_command
        ;;
    --own|-o)
        own_commands $2 $3 $4 $5 $6 $7 $8
        ;;
    --test|-t)
        eval ${DOCKER_CMD} exec -it dev sh docker/do_test.sh
        ;;
    --delete|-D)
        read -p "All docker containers in the system will be deleted, are you sure? [Y/N] "  -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
          eval ${DOCKER_CMD} prune system -a
        fi
        ;;

    --help|-h)
        general_help $1
        ;;

    *)
        echo $"Usage: $0 {--build[-b]|--run[-r]|--down[-d]|--own[-o]|--test[-t]|--delete[-D]} {for help: -h/-help} CMD"
        exit 1

esac