# serial 100
# sngrep.m4: Custom autotools macros for sngrep
#
# @author Adam Duskett <aduskett@codeblue.com>
# @version 2017-05-25
# @license GNU General Public License 3.0
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# As a special exception, the you may copy, distribute and modify the
# configure scripts that are the output of Autoconf when processing
# the Macro.  You need not follow the terms of the GNU General Public
# License when using or distributing such scripts.
#

# SNGREP_CHECK_SCRIPT(LIBNAME, FUNCTION, DEFINE, CONFIG_SCRIPT, ELSE_PART)
AC_DEFUN([SNGREP_CHECK_SCRIPT],
[
   if test ! -z "m4_toupper($SNGREP_[$1]_CONFIG_SCRIPT)"; then
      # to be used to set the path to *-config when cross-compiling
      sngrep_config_script=$(m4_toupper($SNGREP_[$1]_CONFIG_SCRIPT) --libs 2> /dev/null)
   else
      sngrep_config_script=$([$4] --libs 2> /dev/null)
   fi
   sngrep_script_success=no
   sngrep_save_LDFLAGS="$LDFLAGS"
   if test ! "x$sngrep_config_script" = x; then
      LDFLAGS="$sngrep_config_script $LDFLAGS"
      AC_CHECK_LIB([$1], [$2], [
         AC_DEFINE([$3], 1, [The library is present.])
         LIBS="$sngrep_config_script $LIBS "
         sngrep_script_success=yes
      ], [])
      LDFLAGS="$save_LDFLAGS"
   fi
   if test "x$sngrep_script_success" = xno; then
      [$5]
   fi
])

# SNGREP_CHECK_LIB(LIBNAME, FUNCTION, DEFINE, ELSE_PART)
AC_DEFUN([SNGREP_CHECK_LIB],
[
   AC_CHECK_LIB([$1], [$2], [
      AC_DEFINE([$3], 1, [The library is present.])
      LIBS="-l[$1] $LIBS "
   ], [$4])
])
