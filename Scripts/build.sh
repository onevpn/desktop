#!/bin/sh -e

#brew install qt5
qt5path=($(brew --prefix qt5))
#qt5path="/Users/sergey/Qt5.6.1/5.6/clang_64/"
buildPath="$HOME/OneVPN-build"
sourceP=$(pwd)
sourcePath="$sourceP/.."

echo "Founded qmake by path: $qt5path"

pathToPro="/Users/sergey/onevpn-wrapper/OneVPN.pro"

mkdir "$buildPath"
cd "$buildPath"

$qt5path/bin/qmake "$pathToPro"
make
appPath="$buildPath/OneVPN.app"

$qt5path/bin/macdeployqt $appPath

cp "$sourcePath/Mac/Info.plist" "$buildPath/"

identity="3rd Party Mac Developer Application: Alexey Azanov (35NMW3MET5)"
codesign --deep "$appPath" -s "3rd Party Mac Developer Application: Alexey Azanov (35NMW3MET5)"


