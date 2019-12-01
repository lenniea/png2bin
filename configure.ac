#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([png2bin], [1.0], [null@null.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/png2bin.c])
AC_CONFIG_HEADERS([config.h])

lib_error() {
	echo ""
	echo "  Library $1 was not found on this system."
	echo "  Please install it and re-run ./configure"
	echo ""
	exit 1
}

prog_error() {
	echo ""
	echo "  Program $1 was not found on this system."
	echo "  This program is part of $2."
	echo "  Please install it and re-run ./configure"
	echo ""
	exit 1
}

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([png_create_read_struct], [png], , [lib_error libpng], [-lz])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
