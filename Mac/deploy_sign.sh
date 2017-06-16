#!/bin/bash
/Users/admin/Qt/5.5/clang_64/bin/macdeployqt /Users/admin/Documents/build-OneVPN-Desktop_Qt_5_5_1_clang_64bit-Release/OneVPN.app
codesign --deep /Users/admin/Documents/build-OneVPN-Desktop_Qt_5_5_1_clang_64bit-Release/OneVPN.app  -s "3rd Party Mac Developer Application: Alexey Azanov (35NMW3MET5)"