##*****************************************************************************
## $Id: ac_powerman_module_dir.m4,v 1.4 2006/07/27 02:29:42 chu11 Exp $
##*****************************************************************************

AC_DEFUN([AC_POWERMAN_MODULE_DIR],
[
  if echo ${libdir} | grep 'lib64'; then
     LIBDIRTYPE=lib64
  else
     LIBDIRTYPE=lib
  fi

  if test "$prefix" = "NONE"; then
     POWERMAN_MODULE_DIR=${ac_default_prefix}/$LIBDIRTYPE/powerman
  else
     POWERMAN_MODULE_DIR=${prefix}/$LIBDIRTYPE/powerman
  fi

  AC_DEFINE_UNQUOTED([POWERMAN_MODULE_DIR], 
                     ["$POWERMAN_MODULE_DIR"], 
                     [Define default powerman module dir])
  AC_SUBST(POWERMAN_MODULE_DIR)
])
