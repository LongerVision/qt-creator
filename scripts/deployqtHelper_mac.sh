#!/bin/bash

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt bin folder> <qt translations folder> <qt plugin folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

app_path="$1"
resource_path="$app_path/Contents/Resources"
libexec_path="$app_path/Contents/Resources/libexec"
bin_src="$2"
translation_src="$3"
plugin_src="$4"
quick2_src="$5"

echo "Deploying Qt"

# copy qtdiag
echo "- Copying qtdiag"
cp "$bin_src/qtdiag" "$app_path/Contents/MacOS/" || exit 1
# fix rpath if qtdiag was from binary Qt package
( xcrun install_name_tool -delete_rpath "@loader_path/../lib" "$app_path/Contents/MacOS/qtdiag" &&
  xcrun install_name_tool -add_rpath "@loader_path/../Frameworks" "$app_path/Contents/MacOS/qtdiag" ) || true


# collect designer plugins
designerDestDir="$app_path/Contents/PlugIns/designer"
if [ ! -d "$designerDestDir" ]; then
    echo "- Copying designer plugins"
    mkdir -p "$designerDestDir"
    for plugin in "$plugin_src"/designer/*.dylib; do
        cp "$plugin" "$designerDestDir"/ || exit 1
    done
fi

# collect 3d assetimporter plugins
assetimporterDestDir="$app_path/Contents/PlugIns/assetimporters"
assetimporterSrcDir="$plugin_src/assetimporters"
if [ -d "$assetimporterSrcDir" ]; then
    if [ ! -d "$assetimporterDestDir" ]; then
        echo "- Copying 3d assetimporter plugins"
        mkdir -p "$assetimporterDestDir"
        find "$assetimporterSrcDir" -iname "*.dylib" -maxdepth 1 -exec cp {} "$assetimporterDestDir"/ \;
    fi
fi

# copy Qt Quick 2 imports
imports2Dir="$app_path/Contents/Imports/qtquick2"
if [ -d "$quick2_src" ]; then
    if [ ! -d "$imports2Dir" ]; then
        echo "- Copying Qt Quick 2 imports"
        mkdir -p "$imports2Dir"
        cp -R "$quick2_src"/ "$imports2Dir"/
        find "$imports2Dir" -path "*.dylib.dSYM*" -delete
    fi
fi

# copy qt creator qt.conf
if [ ! -f "$resource_path/qt.conf" ]; then
    echo "- Copying qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qt.conf" "$resource_path/qt.conf" || exit 1
fi

# copy libexec tools' qt.conf
if [ ! -f "$libexec_path/qt.conf" ]; then
    echo "- Copying libexec/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/libexec_qt.conf" "$libexec_path/qt.conf" || exit 1
fi

# copy ios tools' qt.conf
if [ ! -f "$libexec_path/ios/qt.conf" ]; then
    echo "- Copying libexec/ios/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/ios_qt.conf" "$libexec_path/ios/qt.conf" || exit 1
fi

# copy qml2puppet's qt.conf
if [ ! -f "$libexec_path/qmldesigner/qt.conf" ]; then
    echo "- Copying libexec/qmldesigner/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmldesigner_qt.conf" "$libexec_path/qmldesigner/qt.conf" || exit 1
fi

# copy Qt translations
# check for known existing translation to avoid copying multiple times
if [ ! -f "$resource_path/translations/qt_de.qm" ]; then
    echo "- Copying Qt translations"
    cp "$translation_src"/*.qm "$resource_path/translations/" || exit 1
fi

# copy clang if needed
if [ $LLVM_INSTALL_DIR ]; then
    if [ "$LLVM_INSTALL_DIR"/bin/clangd -nt "$libexec_path"/clang/bin/clangd ]; then
        echo "- Copying clang"
        mkdir -p "$app_path/Contents/Frameworks" || exit 1
        # use recursive copy to make it copy symlinks as symlinks
        mkdir -p "$libexec_path/clang/bin"
        mkdir -p "$libexec_path/clang/lib"
        cp -Rf "$LLVM_INSTALL_DIR"/lib/clang "$libexec_path/clang/lib/" || exit 1
        cp -Rf "$LLVM_INSTALL_DIR"/lib/ClazyPlugin.dylib "$libexec_path/clang/lib/" || exit 1
        clangdsource="$LLVM_INSTALL_DIR"/bin/clangd
        cp -Rf "$clangdsource" "$libexec_path/clang/bin/" || exit 1
        clangtidysource="$LLVM_INSTALL_DIR"/bin/clang-tidy
        cp -Rf "$clangtidysource" "$libexec_path/clang/bin/" || exit 1
        clazysource="$LLVM_INSTALL_DIR"/bin/clazy-standalone
        cp -Rf "$clazysource" "$libexec_path/clang/bin/" || exit 1
        install_name_tool -add_rpath "@executable_path/../lib" "$libexec_path/clang/bin/clazy-standalone" 2> /dev/null
        install_name_tool -delete_rpath "/Users/qt/work/build/libclang/lib" "$libexec_path/clang/bin/clazy-standalone" 2> /dev/null
    fi
    clangbackendArgument="-executable=$libexec_path/clangbackend"
fi

#### macdeployqt

if [ ! -d "$app_path/Contents/Frameworks/QtCore.framework" ]; then

    qml2puppetapp="$libexec_path/qmldesigner/qml2puppet"
    if [ -f "$qml2puppetapp" ]; then
        qml2puppetArgument="-executable=$qml2puppetapp"
    fi

    qbsapp="$app_path/Contents/MacOS/qbs"
    if [ -f "$qbsapp" ]; then
        qbsArguments=("-executable=$qbsapp" \
        "-executable=$qbsapp-config" \
        "-executable=$qbsapp-config-ui" \
        "-executable=$qbsapp-setup-android" \
        "-executable=$qbsapp-setup-qt" \
        "-executable=$qbsapp-setup-toolchains" \
        "-executable=$qbsapp-create-project" \
        "-executable=$libexec_path/qbs_processlauncher")
    fi

    echo "- Running macdeployqt ($bin_src/macdeployqt)"

    "$bin_src/macdeployqt" "$app_path" \
        "-executable=$app_path/Contents/MacOS/qtdiag" \
        "-executable=$libexec_path/qtpromaker" \
        "-executable=$libexec_path/sdktool" \
        "-executable=$libexec_path/ios/iostool" \
        "-executable=$libexec_path/buildoutputparser" \
        "-executable=$libexec_path/cpaster" \
        "${qbsArguments[@]}" \
        "$qml2puppetArgument" \
        "$clangbackendArgument" || exit 1

fi

# clean up after macdeployqt
# it deploys some plugins (and libs for these) that interfere with what we want
echo "Cleaning up after macdeployqt..."
toRemove=(\
    "Contents/PlugIns/tls/libqopensslbackend.dylib" \
    "Contents/PlugIns/sqldrivers/libqsqlpsql.dylib" \
    "Contents/PlugIns/sqldrivers/libqsqlodbc.dylib" \
    "Contents/Frameworks/libpq.*dylib" \
    "Contents/Frameworks/libssl.*dylib" \
    "Contents/Frameworks/libcrypto.*dylib" \
)
for f in "${toRemove[@]}"; do
    echo "- removing \"$app_path/$f\""
    rm "$app_path"/$f 2> /dev/null; done
exit 0
