### covered.m4 -- extra macros for configuring Covered		-*- Autoconf -*-

## COVERED_PATH_TCL_CONFIG
## ------------------
## Try finding tclConfig.sh in common library directories and their
## tcl$x.$y subdirectories.  Set shell variable r_cv_path_TCL_CONFIG
## to the entire path of the script if found, and leave it empty
## otherwise.
AC_DEFUN([COVERED_PATH_TCL_CONFIG],
[AC_MSG_CHECKING([for tclConfig.sh in library (sub)directories])
AC_CACHE_VAL([r_cv_path_TCL_CONFIG],
[for ldir in /opt/lib /sw/lib /usr/local/lib /usr/lib /lib /usr/lib64 ; do
  for dir in \
      ${ldir} \
      `ls -d ${ldir}/tcl[[8-9]].[[0-9]]* 2>/dev/null | sort -r`; do
    if test -f ${dir}/tclConfig.sh; then
      r_cv_path_TCL_CONFIG="${dir}/tclConfig.sh"
      break 2
    fi
  done
done])
if test -n "${r_cv_path_TCL_CONFIG}"; then
  AC_MSG_RESULT([${r_cv_path_TCL_CONFIG}])
else
  AC_MSG_RESULT([no])
fi
])# COVERED_PATH_TCL_CONFIG

## COVERED_PATH_TK_CONFIG
## ------------------
## Try finding tkConfig.sh in common library directories and their
## tk$x.$y subdirectories.  Set shell variable r_cv_path_TK_CONFIG
## to the entire path of the script if found, and leave it empty
## otherwise.
AC_DEFUN([COVERED_PATH_TK_CONFIG],
[AC_MSG_CHECKING([for tkConfig.sh in library (sub)directories])
AC_CACHE_VAL([r_cv_path_TK_CONFIG],
[for ldir in /opt/lib /sw/lib /usr/local/lib /usr/lib /lib /usr/lib64 ; do
  for dir in \
      ${ldir} \
      `ls -d ${ldir}/tk[[8-9]].[[0-9]]* 2>/dev/null | sort -r`; do
    if test -f ${dir}/tkConfig.sh; then
      r_cv_path_TK_CONFIG="${dir}/tkConfig.sh"
      break 2
    fi
  done
done])
if test -n "${r_cv_path_TK_CONFIG}"; then
  AC_MSG_RESULT([${r_cv_path_TK_CONFIG}])
else
  AC_MSG_RESULT([no])
fi
])# COVERED_PATH_TK_CONFIG

## COVERED_TCLTK_CONFIG
## ---------------
## Try finding the tclConfig.sh and tkConfig.sh scripts in PATH as well
## as in common library directories and their tcl/tk subdirectories.
## Set shell variables TCL_CONFIG and TK_CONFIG to the entire paths to
## the scripts if found and check that the corresponding Tcl/Tk versions
## are at least 8; if not, set shell variable have_tcltk to 'no'.
AC_DEFUN([COVERED_TCLTK_CONFIG],
[AC_PATH_PROGS(TCL_CONFIG, [${TCL_CONFIG} tclConfig.sh])
if test -z "${TCL_CONFIG}"; then
  COVERED_PATH_TCL_CONFIG
  if test -n "${r_cv_path_TCL_CONFIG}"; then
    TCL_CONFIG="${r_cv_path_TCL_CONFIG}"
  fi
fi
AC_PATH_PROGS(TK_CONFIG, [${TK_CONFIG} tkConfig.sh])
if test -z "${TK_CONFIG}"; then
  COVERED_PATH_TK_CONFIG
  if test -n "${r_cv_path_TK_CONFIG}"; then
    TK_CONFIG="${r_cv_path_TK_CONFIG}"
  fi
fi
if test -z "${TCLTK_CPPFLAGS}" \
    || test -z "${TCLTK_LIBS}"; then
  ## Check whether the versions found via the *Config.sh files are at
  ## least 8; otherwise, issue a warning and turn off Tcl/Tk support.
  ## Note that in theory a system could have outdated versions of the
  ## *Config.sh scripts and yet up-to-date installations of Tcl/Tk in
  ## standard places ...
  if test -n "${TCL_CONFIG}"; then
    . ${TCL_CONFIG}
    if test ${TCL_MAJOR_VERSION} -lt 8; then
      warn_tcltk_version="Tcl/Tk support requires Tcl version >= 8"
      AC_MSG_WARN([${warn_tcltk_version}])
      have_tcltk=no
    fi
  fi
  if test -n "${TK_CONFIG}" \
      && test -z "${warn_tcltk_version}"; then
    . ${TK_CONFIG}
    if test ${TK_MAJOR_VERSION} -lt 8; then
      warn_tcltk_version="Tcl/Tk support requires Tk version >= 8"
      AC_MSG_WARN([${warn_tcltk_version}])
      have_tcltk=no
    fi
  fi
  if test -n "${TCL_CONFIG}" \
      && test -n "${TK_CONFIG}" \
      && test -z "${warn_tcltk_version}"; then
    if test ${TCL_MAJOR_VERSION} -ne ${TK_MAJOR_VERSION} \
      || test ${TCL_MINOR_VERSION} -ne ${TK_MINOR_VERSION}; then
     warn_tcltk_version="Tcl and Tk major or minor versions disagree"
      AC_MSG_WARN([${warn_tcltk_version}])
      have_tcltk=no
    fi
  fi
fi
])# COVERED_TCLTK_CONFIG

## COVERED_HEADER_TCL
## -------------
## Set shell variable 'r_cv_header_tcl_h' to 'yes' if a recent enough
## 'tcl.h' is found, and to 'no' otherwise.
AC_DEFUN([COVERED_HEADER_TCL],
[AC_CACHE_CHECK([for tcl.h], [r_cv_header_tcl_h],
[AC_EGREP_CPP([yes],
[#include <tcl.h>
#if (TCL_MAJOR_VERSION >= 8)
  yes
#endif
],
             [r_cv_header_tcl_h=yes],
             [r_cv_header_tcl_h=no])])
])# COVERED_HEADER_TCL

## COVERED_HEADER_TK
## -------------
## Set shell variable 'r_cv_header_tk_h' to 'yes' if a recent enough
## 'tk.h' is found, and to 'no' otherwise.
AC_DEFUN([COVERED_HEADER_TK],
[AC_CACHE_CHECK([for tk.h], [r_cv_header_tk_h],
[AC_EGREP_CPP([yes],
[#include <tk.h>
#if (TK_MAJOR_VERSION >= 8)
  yes
#endif
],
             [r_cv_header_tk_h=yes],
             [r_cv_header_tk_h=no])])
])# COVERED_HEADER_TK

## COVERED_TCLTK_CPPFLAGS
## -----------------
## Need to ensure that we can find the tcl.h and tk.h headers, which
## may be in non-standard and/or version-dependent directories, such as
## on FreeBSD systems.
##
## The logic is as follows.  If TCLTK_CPPFLAGS was specified, then we
## do not investigate any further.  Otherwise, if we still think we
## have Tcl/Tk, then first try via the corresponding *Config.sh file,
## or else try the obvious.
AC_DEFUN([COVERED_TCLTK_CPPFLAGS],
[AC_REQUIRE([COVERED_TCLTK_CONFIG])
if test -z "${TCLTK_CPPFLAGS}"; then
  ## We have to do the work.
  if test "${have_tcltk}" = yes; then
    ## Part 1.  Check for tcl.h.
    found_tcl_h=no
    if test -n "${TCL_CONFIG}"; then
      . ${TCL_CONFIG}
      ## Look for tcl.h in
      ##   ${TCL_PREFIX}/include/tcl${TCL_VERSION}
      ##   ${TCL_PREFIX}/include
      ## Also look in
      ##   ${TCL_PREFIX}/include/tcl${TCL_VERSION}/generic
      ## to deal with current FreeBSD layouts.  These also link the real
      ## thing to the version subdir, but the link cannot be used as it
      ## fails to include 'tclDecls.h' which is not linked.  Hence we
      ## must look for the real thing first.  Argh ...
      for dir in \
          ${TCL_PREFIX}/include/tcl${TCL_VERSION}/generic \
          ${TCL_PREFIX}/include/tcl${TCL_VERSION} \
          ${TCL_PREFIX}/include; do 
        AC_CHECK_HEADER([${dir}/tcl.h],
                        [TCLTK_CPPFLAGS="-I${dir}"
                         found_tcl_h=yes
                         break])
      done
    fi
    if test "${found_tcl_h}" = no; then
      COVERED_HEADER_TCL
      if test "${r_cv_header_tcl_h}" = yes; then
        found_tcl_h=yes
      else
        have_tcltk=no
      fi
    fi
  fi
  if test "${have_tcltk}" = yes; then
    ## Part 2.  Check for tk.h.
    found_tk_h=no
    if test -n "${TK_CONFIG}"; then
      . ${TK_CONFIG}
      ## Look for tk.h in
      ##   ${TK_PREFIX}/include/tk${TK_VERSION}
      ##   ${TK_PREFIX}/include
      ## Also look in
      ##   ${TK_PREFIX}/include/tcl${TK_VERSION}
      ## to compensate for Debian madness ...
      ## Also look in
      ##   ${TK_PREFIX}/include/tk${TK_VERSION}/generic
      ## to deal with current FreeBSD layouts.  See above for details.
      ##
      ## As the AC_CHECK_HEADER test tries including the header file and
      ## tk.h includes tcl.h and X11/Xlib.h, we need to change CPPFLAGS
      ## for the check.
      r_save_CPPFLAGS="${CPPFLAGS}"
      CPPFLAGS="${CPPFLAGS} ${TK_XINCLUDES} ${TCLTK_CPPFLAGS}"
      for dir in \
          ${TK_PREFIX}/include/tk${TK_VERSION}/generic \
          ${TK_PREFIX}/include/tk${TK_VERSION} \
          ${TK_PREFIX}/include/tcl${TK_VERSION} \
          ${TK_PREFIX}/include; do 
        AC_CHECK_HEADER([${dir}/tk.h],
                        [TCLTK_CPPFLAGS="${TCLTK_CPPFLAGS} -I${dir}"
                         found_tk_h=yes
                         break])
      done
      CPPFLAGS="${r_save_CPPFLAGS}"
    fi
    if test "${found_tk_h}" = no; then
      COVERED_HEADER_TK
      if test "{r_cv_header_tk_h}" = yes; then
        found_tk_h=yes
      else
        have_tcltk=no
      fi
    fi
  fi
fi
if test "${have_tcltk}" = yes; then
  if test -n "${TK_XINCLUDES}"; then
    TCLTK_CPPFLAGS="${TCLTK_CPPFLAGS} ${TK_XINCLUDES}"
  else
    TCLTK_CPPFLAGS="${TCLTK_CPPFLAGS} ${X_CFLAGS}"
  fi
fi
])# COVERED_TCLTK_CPPFLAGS

## COVERED_TCLTK_LIBS
## -------------
## Find the tcl and tk libraries.
AC_DEFUN([COVERED_TCLTK_LIBS],
[AC_REQUIRE([AC_PATH_XTRA])
AC_REQUIRE([COVERED_TCLTK_CONFIG])
if test -z "${TCLTK_LIBS}"; then
  ## We have to do the work.
  if test "${have_tcltk}" = yes; then
    ## Part 1.  Try finding the tcl library.
    if test -n "${TCL_CONFIG}"; then
      . ${TCL_CONFIG}
      TCLTK_LIBS="${TCL_LIB_SPEC}"
    else
      AC_CHECK_LIB(tcl, Tcl_CreateInterp,
                   [TCLTK_LIBS=-ltcl],
                   [have_tcltk=no])
    fi
  fi
  if test "${have_tcltk}" = yes; then
    ## Part 2.  Try finding the tk library.
    if test -n "${TK_CONFIG}"; then
      . ${TK_CONFIG}
      TCLTK_LIBS="${TCLTK_LIBS} ${TK_LIB_SPEC} ${TK_LIBS}"
    else
      AC_CHECK_LIB(tk, Tk_Init, , , [${TCLTK_LIBS}])
      if test "${ac_cv_lib_tk_Tk_Init}" = no; then
	## Grr, simple -ltk does not work.
	## But maybe we simply need to add X11 libs.
        ## Note that we cannot simply repeat the above test with extra
        ## libs, because AC_CHECK_LIB uses the corresponding cache var
        ## (ac_cv_lib_tk_Tk_Init in our case) if set.  As using unset
        ## is not portable shell programming according to the Autoconf
        ## docs, we use Tk_SafeInit in the test with X11 libs added.
	AC_CHECK_LIB(tk, Tk_SafeInit,
                     [TCLTK_LIBS="${TCLTK_LIBS} -ltk ${X_LIBS}"],
	             [have_tcltk=no],
                     [${TCLTK_LIBS} ${X_LIBS}])
      fi
    fi
  fi
  ## Postprocessing for AIX.
  ## On AIX, the *_LIB_SPEC variables need to contain '-bI:' flags for
  ## the Tcl export file.  These are really flags for ld rather than the
  ## C/C++ compilers, and hence may need protection via '-Wl,'.
  ## We have two ways of doing that:
  ## * Recording whether '-Wl,' is needed for the C or C++ compilers,
  ##   and getting this info into the TCLTK_LIBS make variable ... mess!
  ## * Protecting all entries in TCLTK_LIBS that do not start with '-l'
  ##   or '-L' with '-Wl,' (hoping that all compilers understand this).
  ##   Easy, hence ...
  case "${host_os}" in
    aix*)
      orig_TCLTK_LIBS="${TCLTK_LIBS}"
      TCLTK_LIBS=
      for flag in ${orig_TCLTK_LIBS}; do
        case "${flag}" in
	  -l*|-L*|-Wl,*) ;;
	  *) flag="-Wl,${flag}" ;;
	esac
	TCLTK_LIBS="${TCLTK_LIBS} ${flag}"
      done
      ;;
  esac
  ## Force evaluation ('-ltcl8.3${TCL_DBGX}' and friends ...).
  eval "TCLTK_LIBS=\"${TCLTK_LIBS}\""
fi
])# COVERED_TCLTK_LIBS

## COVERED_TCLTK_WORKS
## --------------
## Check whether compiling and linking code using Tcl/Tk works.
## Set shell variable r_cv_tcltk_works to 'yes' or 'no' accordingly.
AC_DEFUN([COVERED_TCLTK_WORKS],
[AC_CACHE_CHECK([whether compiling/linking Tcl/Tk code works],
                [r_cv_tcltk_works],
[AC_LANG_PUSH(C)
r_save_CPPFLAGS="${CPPFLAGS}"
r_save_LIBS="${LIBS}"
CPPFLAGS="${CPPFLAGS} ${TCLTK_CPPFLAGS}"
LIBS="${LIBS} ${TCLTK_LIBS}"
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <tcl.h>
#include <tk.h>
]],
[[static char * p1 = (char *) Tcl_Init;
static char * p2 = (char *) Tk_Init;
]])],
r_cv_tcltk_works=yes,
r_cv_tcltk_works=no)
CPPFLAGS="${r_save_CPPFLAGS}"
LIBS="${r_save_LIBS}"
AC_LANG_POP(C)])
])# COVERED_TCLTK_WORKS

## COVERED_TCLTK
## -------
AC_DEFUN([COVERED_TCLTK],
[if test "${want_tcltk}" = yes; then
  have_tcltk=yes
  ## (Note that the subsequent 3 macros assume that have_tcltk has been
  ## set appropriately.)
  COVERED_TCLTK_CONFIG
  COVERED_TCLTK_CPPFLAGS  
  COVERED_TCLTK_LIBS
  if test "${have_tcltk}" = yes; then
    COVERED_TCLTK_WORKS
    have_tcltk=${r_cv_tcltk_works}
  fi
else
  have_tcltk=no
  ## Just making sure.
  TCLTK_CPPFLAGS=
  TCLTK_LIBS=
fi
if test "${have_tcltk}" = yes; then
  AC_DEFINE(HAVE_TCLTK, 1,
            [Define if you have the Tcl/Tk headers and libraries and
	     want Tcl/Tk support to be built.])
  use_tcltk=yes
else
  use_tcltk=no
fi
AC_SUBST(TCLTK_CPPFLAGS)
AC_SUBST(TCLTK_LIBS)
AC_SUBST(use_tcltk)
])# COVERED_TCLTK

## COVERED_PROG_BROWSER
## --------------
AC_DEFUN([COVERED_PROG_BROWSER],
[if test -z "${COVERED_BROWSER}"; then
  AC_PATH_PROGS(COVERED_BROWSER, [netscape mozilla galeon kfmclient opera gnome-moz-remote open])
fi
if test -z "${COVERED_BROWSER}"; then
  warn_browser="I could not determine a browser"
  AC_MSG_WARN([${warn_browser}])
else
  AC_MSG_RESULT([using default browser ... ${COVERED_BROWSER}])
fi
AC_DEFINE_UNQUOTED(COVERED_BROWSER, "$COVERED_BROWSER", [Browser to view help pages with])
])# COVERED_BROWSER

## COVERED_DEBUG_MODE
## ------------------
AC_DEFUN([COVERED_DEBUG],
[AC_ARG_ENABLE(debug,
               AC_HELP_STRING([--enable-debug],[Enables debugging output to be generated]),
               AC_SUBST([DEBUGDEF],["-DDEBUG_MODE"]))])

## COVERED_PROFILE_MODE
AC_DEFUN([COVERED_PROFILER],
[AC_ARG_ENABLE(profiling,
               AC_HELP_STRING([--enable-profiling],[Enables source code profiler]),
               AC_SUBST([PROFILEDEF],["-DPROFILER"]))])

# AX_LD_SHAREDLIB_OPTS
# --------------------
# linker options when building a shared library
AC_DEFUN([AX_LD_SHAREDLIB_OPTS],
[AC_MSG_CHECKING([for shared library link flag])
shared=-shared
case "${host}" in
     *-*-cygwin*)
        shared="-shared -Wl,--enable-auto-image-base"
        ;;
        
     *-*-hpux*)
        shared="-b"
        ;;

     *-*-darwin1.[0123])
        shared="-bundle -undefined suppress"
        ;; 

     *-*-darwin*)
        shared="-bundle -undefined suppress -flat_namespace"
        ;;
esac
AC_SUBST(shared)
AC_MSG_RESULT($shared)
])# AX_LD_SHAREDLIB_OPTS
    
# AX_C_PICFLAG
# ------------
# The -fPIC flag is used to tell the compiler to make position
# independent code. It is needed when making shared objects.
AC_DEFUN([AX_C_PICFLAG],
[AC_MSG_CHECKING([for flag to make position independent code])
PICFLAG=-fPIC
case "${host}" in

     *-*-cygwin*)
        PICFLAG=
        ;;

     *-*-hpux*)
        PICFLAG=+z
        ;;

esac
AC_SUBST(PICFLAG)
AC_MSG_RESULT($PICFLAG)
])# AX_C_PICFLAG

# AX_LD_RDYNAMIC
# --------------
# The -rdynamic flag is used by iverilog when compiling the target,
# to know how to export symbols of the main program to loadable modules
# that are brought in by -ldl
AC_DEFUN([AX_LD_RDYNAMIC],
[AC_MSG_CHECKING([for -rdynamic compiler flag])
rdynamic=-rdynamic
case "${host}" in

    *-*-netbsd*)
        rdynamic="-Wl,--export-dynamic"
        ;;

    *-*-openbsd*)
        rdynamic="-Wl,--export-dynamic"
        ;;

    *-*-solaris*)
        rdynamic=""
        ;;
    
    *-*-cygwin*)
        rdynamic=""
        ;;
    
    *-*-hpux*)
        rdynamic="-E"
        ;;
    
    *-*-darwin*)
        rdynamic="-Wl,-all_load"
        strip_dynamic="-SX"
        ;;

esac
AC_SUBST(rdynamic)
AC_MSG_RESULT($rdynamic)
AC_SUBST(strip_dynamic)
# since we didn't tell them we're "checking", no good place to tell the answer
# AC_MSG_RESULT($strip_dynamic)
])# AX_LD_RDYNAMIC

# AX_CPP_PRECOMP
# --------------
AC_DEFUN([AX_CPP_PRECOMP],
[# Darwin requires -no-cpp-precomp
case "${host}" in
    *-*-darwin*)
        CPPFLAGS="-no-cpp-precomp $CPPFLAGS"
        CFLAGS="-no-cpp-precomp $CFLAGS"
        ;;
esac
])# AX_CPP_PRECOMP

