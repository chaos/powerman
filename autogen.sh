#!/bin/sh
##
# $Id: autogen.sh,v 1.1.1.1 2004/07/02 22:31:29 achu Exp $
##

echo "running aclocal ... "
aclocal -I config
echo "running libtoolize ... "
libtoolize --automake --copy 
echo "running autoheader ... "
autoheader
echo "running automake ... "
automake --copy --add-missing 
echo "running autoconf ... "
autoconf
echo "now run ./configure to configure powerman for your environment."
