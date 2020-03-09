cmake_minimum_required(VERSION 3.1.0)

project(OneVPN VERSION 1.0)

#set(CMAKE_PREFIX_PATH "/Users/s.kudaev/Qt5.9.0/5.9/clang_64/")
#set(CMAKE_PREFIX_PATH "../__SDK__/Qt/5.12.3/clang_64/")
#set(CMAKE_PREFIX_PATH "D:/Qt5.13/5.12.3/msvc2017_64/")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread -lssl -lcrypto -lpng")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# при генерации проекта для Xcode надо помнить, что нужно конфигурить cmake с параметром release (к примеру)
# cmake -G "Xcode" -DCMAKE_BUILD_TYPE=Release
# но сам xcode не использует при генерации схемы параметр CMAKE_BUILD_TYPE и продолжит собирать в Debug режиме
# по этому надо еще и собирать вот таким образом
# cmake --build ./build --target install --config Release
# либо же переключать Build Configuration в самом xcode

include(QtCommon)

set(PROJECT_NAME "OneVPN")
set(COMPANY "OneVPN")
SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
set(COPYRIGHT "Copyright (c) 2018 OneVPN. All rights reserved.")
set(IDENTIFIER "com.OneVPN.OneVPN")
set(RESOURCE_FILES "resources/mainwindow.qrc")

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")

include_directories(src)
fix_project_version()
add_project_meta(META_FILES_TO_INCLUDE)

file(GLOB SOURCES
            "src/*.h"
            "src/*.cpp"
            "src/Controllers/*.h"
            "src/Controllers/*.cpp"
            "src/DAOs/*.h"
            "src/DAOs/*.cpp"
            "src/Interfaces/*.h"
            "src/pubsub/*.h"
            "src/pubsub/*.cpp"
            "src/SingleApplication/*.h"
            "src/SingleApplication/*.cpp"
    )

if (APPLE)
    set(SOURCES ${SOURCES}
        src/Mac/OpenVPNConnector.mm
        src/Mac/OpenVPNConnectorQt.mm
        src/Mac/Connection.cpp
        src/Mac/platformutils.mm
        src/Mac/CrashlyticsImpl.mm
        src/Mac/Updater.mm
    )
    execute_process (
            COMMAND bash -c "(echo \"$(brew --prefix qt5)\")"
            OUTPUT_VARIABLE QT5PATH
    )
    if (QT5PATH)
        # Homebrew installs Qt5 in /usr/local/qt5.
        list(APPEND CMAKE_PREFIX_PATH "${QT5PATH}")
    else()
        message(WARNING "Required dependence \"Qt5Core\" not found on the path /usr/local/opt/qt5")
    endif()
elseif(WIN32)
     set(SOURCES ${SOURCES}
        src/Windows/platformutils.cpp
        src/Windows/connection.h
        src/Windows/connection.cpp
        src/Windows/openvpnconnectorqt.h
        src/Windows/openvpnconnectorqt.cpp
    )
endif()

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

include_directories(include)
add_executable(OneVPN ${OS_BUNDLE} ${SOURCES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES})
if (WIN32)
    add_executable(OneVPNService "WindowsOpenVPNService/main.cpp")
    target_link_libraries(OneVPNService "advapi32.lib")
endif()

find_package(harfbuzz CONFIG REQUIRED)
if(NOT harfbuzz_FOUND)
    message(STATUS "harfbuzz:    NO")
else()
    message(STATUS "harfbuzz:    YES")
endif()
find_package(unofficial-glib CONFIG REQUIRED)
if(NOT unofficial-glib_FOUND)
    message(STATUS "unofficial-glib:    NO")
else()
    message(STATUS "unofficial-glib:    YES")
endif()

find_package(ZLIB)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

target_link_libraries(OneVPN PRIVATE Qt5::Widgets Qt5::Core Qt5::Network harfbuzz::harfbuzz unofficial::glib::gio unofficial::glib::glib unofficial::glib::gmodule unofficial::glib::gobject ${ZLIB_LIBRARIES})
fix_win_compiler()

if(APPLE)
    target_link_libraries(OneVPN stdc++ "-framework CoreFoundation" "-framework CoreServices" "-framework Security" "-framework SystemConfiguration" "-framework AppKit" "-framework ServiceManagement" objc)

 #   set(BUNDLE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.app")
#    foreach(RES ${RESOURCES})
 #       install(DIRECTORY ${RES} DESTINATION "${BUNDLE_PATH}/Contents/MacOS/")
  #  endforeach()

   # install(CODE "
   #         execute_process(COMMAND ${MACDEPLOYQT_PATH}/macdeployqt \"$<TARGET_FILE_DIR:OneVPN>\" -verbose=2)
   #         message(STATUS \"Creating dmg of the app\")
   #         message(\"$<TARGET_FILE_DIR:OneVPN>\")
   #         #execute_process(COMMAND hdiutil create -fs HFS+ -volname ${PROJECT_NAME} -srcfolder \"${BUNDLE_PATH}\" -ov -format UDZO \"${PROJECT_NAME}.dmg\")
   #         " COMPONENT Runtime
   # )

    find_path(MACDEPLOYQT_PATH macdeployqt PATH_SUFFIXES bin)
    if(NOT MACDEPLOYQT_PATH)
        message(FATAL_ERROR "Could not find macdeployqt for OSX bundling. You can point MACDEPLOYQT_PATH to it's path.")
    endif()

    add_custom_command(TARGET OneVPN POST_BUILD
        COMMAND ${MACDEPLOYQT_PATH}/macdeployqt \"$<TARGET_FILE_DIR:OneVPN>/../..\" -verbose=2
        COMMENT "Deploying Qt..."
    )

    install(PROGRAMS "$<TARGET_FILE_DIR:OneVPN>/../../../" DESTINATION .)

    file(GLOB DEPLOY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Resources/*")
    foreach(ITEM ${DEPLOY_FILES})
       if(IS_DIRECTORY "${ITEM}")
          list(APPEND DIRS_TO_DEPLOY "${ITEM}")
       else()
          list(APPEND FILES_TO_DEPLOY "${ITEM}")
       endif()
    endforeach()

    install(FILES ${FILES_TO_DEPLOY} DESTINATION "./OneVPN.app/Contents/Resources")
    install(DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION "./OneVPN.app/Contents/Resources")
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Resources/openvpn" DESTINATION "./OneVPN.app/Contents/Resources")

    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Resources/client.down.sh" DESTINATION "./OneVPN.app/Contents/Resources")
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Resources/client.up.sh" DESTINATION "./OneVPN.app/Contents/Resources")

    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Library/LaunchServices/com.aaa.onevpn.OVPNHelper" DESTINATION "./OneVPN.app/Contents/Library/LaunchServices")

    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Mac/Library/LaunchServices/com.aaa.onevpn.OVPNHelper" DESTINATION .)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Mac/OVPNClientSrc/OVPNClient/OVPNHelper/com.aaa.onevpn.OVPNHelper.plist" DESTINATION .)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Mac/OVPNClientSrc/OVPNClient/OVPNHelper/com.aaa.OneVPN.starter.plist" DESTINATION .)

    add_custom_target(installer
                        cmake -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/mac/BuildScripts/installer/" "${CMAKE_CURRENT_BINARY_DIR}/install/distrib"
                        COMMAND packagesbuild "${CMAKE_CURRENT_BINARY_DIR}/install/distrib/OneVpn.pkgproj"
                        VERBATIM)

elseif(WIN32)
    target_link_libraries(OneVPN "Ws2_32.lib" "Advapi32.lib" "Iphlpapi.lib" "Wininet.lib")
    get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    add_custom_command(TARGET OneVPN POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
            env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                \"$<TARGET_FILE:OneVPN>\"
        COMMENT "Deploying Qt..."
    )

    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)
    install(DIRECTORY "$<TARGET_FILE_DIR:OneVPN>/" DESTINATION .)
    install(DIRECTORY "$<TARGET_FILE_DIR:OneVPNService>/" DESTINATION .)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Windows/" DESTINATION .)
endif()

#Cpack
if (WIN32)
    find_program(ISS_COMPILER NAMES iscc ISCC
               HINTS "C:/Program Files (x86)/Inno Setup 5" "C:/Program Files/Inno Setup 5")

    if(NOT ISS_COMPILER)
        message(WARNING "Inno Setup Compiler not found. You won't be able to build setup files.")
    else()
        message(STATUS "Using Inno Setup Compiler from: ${ISS_COMPILER}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/win_installer/"
             DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/distrib")
             
        add_custom_target(installer
                          COMMAND ${ISS_COMPILER} "${CMAKE_CURRENT_BINARY_DIR}/distrib/onevpn_installer.iss"
                          VERBATIM)
    endif()
endif()