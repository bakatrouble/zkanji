#!/bin/bash

PREFIX=$(pwd)/cmake-build-debug
cat zkanji.desktop | sed -e "s#\\{prefix\\}#$PREFIX#g" > zkanji-tmp.desktop
desktop-file-install --dir=$HOME/.local/share/applications zkanji-tmp.desktop
rm -f zkanji-tmp.desktop