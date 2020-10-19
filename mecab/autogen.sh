#!/bin/sh

test -f configure.in || {
  echo "autogen.sh: run this command only at the top of a MeCab source tree."
  exit 1
}

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  cat <<-'EOF'
	
	You must have autoconf installed to compile MeCab.
	Get ftp://ftp.gnu.org/pub/gnu/autoconf/autoconf-2.13.tar.gz
	(or a newer version if it is available)
	EOF
  DIE=1
  NO_AUTOCONF=yes
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  cat <<-'EOF'
	
	You must have automake installed to compile MeCab.
	Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.4.tar.gz
	(or a newer version if it is available)
	EOF
  DIE=1
  NO_AUTOMAKE=yes
}

# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  cat <<-'EOF'
	
	**Error**: Missing `aclocal'.  The version of `automake'
	installed doesn't appear recent enough.
	Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.4.tar.gz
	(or a newer version if it is available)
	EOF
  DIE=1
}

# if no autoconf, don't bother testing for autoheader
test -n "$NO_AUTOCONF" || (autoheader --version) < /dev/null > /dev/null 2>&1 || {
  cat <<-'EOF'
	
	**Error**: Missing `autoheader'.  The version of `autoheader'
	installed doesn't appear recent enough.
	Get ftp://ftp.gnu.org/pub/gnu/autoconf/autoconf-2.13.tar.gz
	(or a newer version if it is available)
	EOF
  DIE=1
}

(grep "^AM_PROG_LIBTOOL" configure.in >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
  cat <<-'EOF'
	
	**Error**: You must have `libtool' installed to compile Gnome.
	Get ftp://ftp.gnu.org/pub/gnu/libtool-1.3.tar.gz
	(or a newer version if it is available)
	EOF
    DIE=1
  }
}

case "$DIE" in (1) exit 1;; esac

case "$*" in ('')
  cat <<-EOF
	**Warning**: I am going to run \`configure' with no arguments.
	If you wish to pass any to it, please specify them on the
	\`$0' command line.
	
	EOF
esac

echo "Generating configure script and Makefiles for MeCab."

grep "^AM_PROG_LIBTOOL" configure.in >/dev/null && {
  echo "Running libtoolize..."
  libtoolize --force --copy
}
echo "Running aclocal ..."
aclocal -I .
echo "Running autoheader..."
autoheader
echo "Running automake ..."
automake --add-missing
echo "Running autoconf ..."
autoconf

echo "Configuring MeCab."
conf_flags="--enable-maintainer-mode"
echo Running $./configure $conf_flags "$@" ...
./configure $conf_flags "$@"
echo "Now type 'make' to compile MeCab."
