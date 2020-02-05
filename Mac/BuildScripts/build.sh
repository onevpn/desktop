#!/bin/sh -e

#brew install qt5
#qt5path=($(brew --prefix qt5))
qt5path="/Users/$USER/Qt5.9.0/5.9/clang_64/"
sourceP=$(pwd)
buildPath="$sourceP/OneVPN-build"
sourcePath="$sourceP/../.."

echo "Founded qmake by path: $qt5path"

pathToPro="$sourcePath/OneVPN.pro"

mkdir "$buildPath"
cd "$buildPath"

$qt5path/bin/qmake "$pathToPro"
make
appPath="$buildPath/OneVPN.app"

cp -R "$sourcePath/Mac/Resources" "$appPath/Contents"
cp -R "$sourcePath/Mac/Library" "$appPath/Contents"

$qt5path/bin/macdeployqt $appPath

cp "$sourcePath/Mac/Info.plist" "$buildPath/"

identity="Developer ID Application: GOLDEN SOFTWARE INC. (U8KD48QDBK)"
#codesign --deep "$appPath" -s $identity

echo "$sourceP/installer/OneVPN.pkgproj"
packagesbuild "$sourceP/installer/OneVPN.pkgproj" || exit 1

#productsign --sign "Developer ID Application: GOLDEN SOFTWARE INC. (U8KD48QDBK)" "$buildPath/OneVpn.mpkg" "$buildPath/OneVPN1.mpkg"
#rm -r "$buildPath/OneVpn.mpkg"
mv "$buildPath/OneVPN1.mpkg" "$buildPath/OneVPN.mpkg"
