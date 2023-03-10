#
# ReCaged - a Free Software, Futuristic, Racing Game
#
# Copyright (C) 2012, 2014, 2015 Mats Wahlberg
#
# This file is part of ReCaged.
#
# ReCaged is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ReCaged is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
#


#
#Custom macros for checking some libraries, config flags, and set up.
#



#
# RC_LIBS_INIT()
#
# Basic tests before starting
#

AC_DEFUN([RC_LIBS_INIT],
[

AC_MSG_CHECKING(if building for a terrible OS)
case "$target_os" in

mingw*|cygwin*)
	AC_MSG_RESULT([yes])
	RC_TARGET="w32"
	;;

*apple*|*darwin*)
	AC_MSG_RESULT([yes])
	RC_TARGET="osx"

	;;

*)
	AC_MSG_RESULT([probably not])
	RC_TARGET="good"
	;;

esac

#make this result available for automake (certain code alterations)
AM_CONDITIONAL([ON_W32], [test "$RC_TARGET" = "w32"])

#(note: the two "if/else" checks below prevents STATIC and CONSOLE from staying
#empty, to avoid using (IMHO) ugly looking "x$STATIC" = "xyes" tests later on)

#if w32, probably wanting static linking?
AC_ARG_ENABLE(
	[w32static],
	[AS_HELP_STRING([--enable-w32static],
			[Link libraries statically on W32 @<:@default=no@:>@])], [
		if test "x$enableval" != "xno"; then
			STATIC="yes"
		else
			STATIC="no"
		fi
		], [STATIC="no"] )

#and console output?
AC_ARG_ENABLE(
	[w32console],
	[AS_HELP_STRING([--enable-w32console],
			[Enable console output on W32 @<:@default=no@:>@])], [
		if test "x$enableval" != "xno"; then
			CONSOLE="yes"
		else
			CONSOLE="no"
		fi
		], [CONSOLE="no"] )

#need windres if on w32
if test "$RC_TARGET" = "w32"; then
	AC_PATH_TOOL([WINDRES], [windres])
	if test -z "$WINDRES"; then
		AC_MSG_ERROR([Program windres appears to be missing])
	fi
fi

])



#
# RC_CHECK_PROG(PROGRAM, FLAG_ARGS, LIB_ARGS, [ACTION-ON-FAILURE]
#
# Find and configure library by running PROGRAM with arguments FLAG_ARGS and
# LIB_ARGS, perform other action on failuer.
#

AC_DEFUN([RC_CHECK_PROG],
[

FAILED="yes"

if test -n "$1"; then

	#try running program for flags
	AC_MSG_CHECKING([for flags using $1 $2])

	#(these if statements check the return status, not returned string)
	if tmp_flags=$($1 $2 2>/dev/null); then
		#ok
		AC_MSG_RESULT([$tmp_flags])

		#now try running it for libs
		AC_MSG_CHECKING([for libraries using $1 $3])

		if tmp_libs=$($1 $3 2>/dev/null); then
			#both were ok, make sure not performing fallback
			FAILED="no"
			AC_MSG_RESULT([$tmp_libs])

			#safely provide these strings for building
			RC_FLAGS="$RC_FLAGS $tmp_flags"
			RC_LIBS="$RC_LIBS $tmp_libs"
		else
			AC_MSG_RESULT([failed])
		fi
	else
		AC_MSG_RESULT([failed])
	fi
fi

if test "$FAILED" = "yes"; then
	$4
fi

])



#
# RC_LIBS_CONFIG()
#
# Check and configure needed libraries
#

AC_DEFUN([RC_LIBS_CONFIG],
[

AC_REQUIRE([RC_LIBS_INIT])

#make sure only enabling w32 options on w32
if test "$RC_TARGET" = "w32"; then

	#console flag
	AC_MSG_CHECKING([if building with w32 console enabled])
	if test "$CONSOLE" = "yes"; then
		AC_MSG_RESULT([yes])

		RC_LIBS="$RC_LIBS -mconsole"
	else
		AC_MSG_RESULT([no])
	fi

	#static flag
	AC_MSG_CHECKING([if building static w32 binary])
	if test "$STATIC" = "yes"; then
		AC_MSG_RESULT([yes])

		RC_FLAGS="$RC_FLAGS -DGLEW_STATIC"
		#TODO: move static-lib* to LDFLAGS?
		RC_LIBS="$RC_LIBS -Wl,-Bstatic -static-libgcc -static-libstdc++"
		pkg_static="--static"
	else
		AC_MSG_RESULT([no])
	fi
fi


#
#actual library checks:
#

AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

#ODE:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags ode], [$pkg_static --libs ode],
[
	AC_PATH_TOOL([ODE_CONFIG], [ode-config])
	RC_CHECK_PROG([$ODE_CONFIG], [--cflags], [--libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for ODE using ac_check_* macros])
		AC_MSG_WARN([Don't know if using double or single precision!... Assuming double...])
		CPPFLAGS="$CPPFLAGS -DdDOUBLE" #hack: appends to user/global variable, but should be fine...?

		AC_CHECK_HEADER([ode/ode.h],, [ AC_MSG_ERROR([Header for ODE appears to be missing, install libode-dev or similar]) ])
		AC_CHECK_LIB([ode], [dInitODE2],
			[RC_LIBS="$RC_LIBS -lode"],
			[AC_MSG_ERROR([ODE library appears to be missing, install libode1 or similar])])
	])
])

#SDL:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags sdl], [$pkg_static --libs sdl],
[
	AC_PATH_TOOL([SDL_CONFIG], [sdl-config])

	#normally "--libs", but might change in certain situation (w32+static)
	if test "$RC_TARGET" = "w32" && test "$STATIC" = "yes"; then
		sdl_libs="--static-libs"
	else
		sdl_libs="--libs"
	fi

	RC_CHECK_PROG([$SDL_CONFIG], [--cflags], [$sdl_libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for SDL using ac_check_* macros])
		AC_CHECK_HEADER([SDL/SDL.h],, [ AC_MSG_ERROR([Header for SDL appears to be missing, install libsdl-dev or similar]) ])
		AC_CHECK_LIB([SDL], [SDL_Init],
			[RC_LIBS="$RC_LIBS -lSDL"],
			[AC_MSG_ERROR([SDL library appears to be missing, install libsdl-1.2 or similar])])
	])
])

#LIBPNG:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags libpng], [$pkg_static --libs libpng],
[
	AC_PATH_TOOL([PNG_CONFIG], [libpng-config])

	#add pkg_static for the potential "--static" flag
	RC_CHECK_PROG([$PNG_CONFIG], [--cflags], [$pkg_static --ldflags],
	[
		AC_MSG_WARN([Attempting to guess configuration for LIBPNG using ac_check_* macros])
		AC_CHECK_HEADER([png.h],, [ AC_MSG_ERROR([Headers for LIBPNG appears to be missing, install libpng-dev or similar]) ])
		AC_CHECK_LIB([png], [png_destroy_write_struct],
			[RC_LIBS="$RC_LIBS -lpng"],
			[AC_MSG_ERROR([LIBPNG library appears to be missing, install libpng or similar])])

		#static linking on windows requires zlib
		if test "$RC_TARGET" = "w32" && test "$STATIC" = "yes"; then
			AC_CHECK_LIB([z], [gzread],
				[RC_LIBS="$RC_LIBS -lz"],
				[AC_MSG_ERROR([ZLIB library appears to be missing, install zlib or similar])])
		fi
	])
])

#LIBJPEG:
#note: Have never seen a libjpeg.pc in the wild, but this can't harm... right?
RC_CHECK_PROG([$PKG_CONFIG], [--cflags libjpeg], [$pkg_static --libs libjpeg],
[
	#okay, most likely outcome: check for existence directly
	AC_MSG_WARN([Attempting to guess configuration for LIBJPEG using ac_check_* macros])
	AC_CHECK_HEADER([jpeglib.h],, [ AC_MSG_ERROR([Headers for LIBJPEG appears to be missing, install libjpeg-dev or similar]) ])
	AC_CHECK_LIB([jpeg], [jpeg_destroy_decompress],
		[RC_LIBS="$RC_LIBS -ljpeg"],
		[AC_MSG_ERROR([LIBJPEG library appears to be missing, install libjpeg or similar])])
])

#GLEW:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags glew], [$pkg_static --libs glew],
[
	AC_MSG_WARN([Attempting to guess configuration for GLEW using ac_check_* macros])
	AC_CHECK_HEADER([GL/glew.h],, [ AC_MSG_ERROR([Header for GLEW appears to be missing, install libglew-dev or similar]) ])

	#w32 uses (of course...) a different name
	if test "$RC_TARGET" = "w32"; then
		AC_CHECK_LIB([glew32], [main],
			[RC_LIBS="$RC_LIBS -lglew32"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	else
		AC_CHECK_LIB([GLEW], [main],
			[RC_LIBS="$RC_LIBS -lGLEW"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	fi

])

#static stop (if enabled and on w32)
if test "$RC_TARGET" = "w32" && test "$STATIC" = "yes"; then
	RC_LIBS="$RC_LIBS -Wl,-Bdynamic"
fi

#GL (never static):
RC_CHECK_PROG([$PKG_CONFIG], [--cflags gl], [--libs gl],
[
	AC_MSG_WARN([Attempting to guess configuration for GL using ac_check_* macros])

	#here's a fun part: both w32 and osx likes to brake naming conventions for opengl...
	if test "$RC_TARGET" = "osx"; then
		AC_CHECK_HEADER([OpenGL/gl.h],, [ AC_MSG_ERROR([Header for GL appears to be missing, you go and figure this one out]) ])
	else
		AC_CHECK_HEADER([GL/gl.h],, [ AC_MSG_ERROR([Header for GL appears to be missing, install libgl1-mesa-dev or similar]) ])
	fi

	#also GL libraries got unreliable symbols (so just check for dummy main)
	case "$RC_TARGET" in

	w32)
		AC_CHECK_LIB([opengl32], [main],
			[RC_LIBS="$RC_LIBS -lopengl32"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
		;;

	osx)
		AC_MSG_WARN([Just guessing "-framework OpenGL" can be used for linking gl library, no guarantees!])
		RC_LIBS="$RC_LIBS -framework OpenGL"
		;;
	
	*)
		AC_CHECK_LIB([GL], [main],
			[RC_LIBS="$RC_LIBS -lGL"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
		;;
	esac

])

#make available
AC_SUBST(RC_FLAGS)
AC_SUBST(RC_LIBS)

])
