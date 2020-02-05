#!/bin/sh -e

#brew install qt5
#qt5path=($(brew --prefix qt5))
qt5path="/Users/sergey/Qt5.8.0/5.8/clang_64/"
sourceP=$(pwd)
buildPath="$sourceP/OneVPN-build"
sourcePath="$sourceP/../.."

echo "Founded qmake by path: $qt5path"

mkdir "$buildPath"
cd "$buildPath"

xc="$sourcePath/xcode/"
xcodebuild -project "$xc/OneVPN.xcodeproj"
appPath="$xc/Release/OneVPN.app"

cp -R "$sourcePath/Mac/Resources" "$appPath/Contents"
cp -R "$sourcePath/Mac/Library" "$appPath/Contents"
cp -R "$sourcePath/Mac/OneVPN.icns" "$appPath/Contents/Resources"

$qt5path/bin/macdeployqt $appPath

cp -R "$appPath" "$buildPath/"
cp "$sourcePath/Mac/Info.plist" "$buildPath/"

#identity="Developer ID Application: GOLDEN SOFTWARE INC. (U8KD48QDBK)"
#codesign --deep "$appPath" -s $identity

packagesbuild "$sourceP/installer/OneVPN.pkgproj" || exit 1

#productsign --sign "" "$buildPath/OneVpn.mpkg" "$buildPath/OneVPN1.mpkg"
#rm -r "$buildPath/OneVpn.mpkg"
mv "$buildPath/OneVPN1.mpkg" "$buildPath/OneVPN.mpkg"

# копирование appcast.xml 
# scp appcast.xml root@onevpn.co:/var/www/onevpn/data/www/onevpn.co/private
# ssh -i Downloads/onevpn-main.pem root@onevpn.co
