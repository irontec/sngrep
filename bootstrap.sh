#!/bin/sh

check_for_app() {
        $1 --version 2>&1 >/dev/null
        if [ $? != 0 ]
        then
                echo "Please install $1 and run bootstrap.sh again!"
                exit 1
        fi
}

# On FreeBSD and OpenBSD, multiple autoconf/automake versions have different names.
# On Linux, environment variables tell which one to use.

case `uname -sr` in
        OpenBSD*)
                export AUTOCONF_VERSION=2.63
                export AUTOMAKE_VERSION=1.9
                ;;
        *)
                AUTOCONF_VERSION=2.60
                AUTOMAKE_VERSION=1.9
                export AUTOCONF_VERSION
                export AUTOMAKE_VERSION
                ;;
esac

check_for_app autoconf
check_for_app autoheader
check_for_app automake
check_for_app aclocal

echo "Generating the configure script ..."

aclocal
autoconf
autoheader
automake --add-missing --copy 2>/dev/null

exit 0

