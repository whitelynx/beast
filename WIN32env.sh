#!/bin/sh
export PS1="[W] $PS1"
export PATH=/c/Perl/bin:/e/programme/subversion/bin:$HOME/prog/win32/win32-dev/bin:$PATH
export PKG_CONFIG_PATH=$HOME/prog/win32/win32-dev/lib/pkgconfig
#:$PKG_CONFIG_PATH
export INTLTOOL_PERL=c:/perl/bin/perl.exe
export LIBZ="-L$HOME/prog/win32/win32-dev/lib -lz"
export LIBZ_CFLAGS="-I$HOME/prog/win32/win32-dev/include"
export CC='gcc -mms-bitfields'
export CXX='g++ -mms-bitfields'
export GUILE_LOAD_PATH="$HOME/prog/win32/win32-dev/share/guile/1.6" 
