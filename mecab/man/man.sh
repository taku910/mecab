#!/bin/sh
# Process this file with automake to produce Makefile.in
cmdarg=$1
if [ -n "$cmdarg" ]; then
  cmd=`basename $cmdarg .1`
else
  cmdarg="mecab"
  cmd="mecab"
fi
man="${cmd}.1"

# TOP PART while including DESCRIPTION contents
LANG=C help2man \
--no-info \
--name='Yet Another Part\-of\-Speech and Morphological Analyzer' \
--include="mecab1.ins" \
$cmd |\
sed -e '/^\.PP/,/^Copyright.*Nippon/d' |\
sed -e 's/(wakati,.*$/(SEE OUTPUT FORMAT)/' > $man

# OUTPUT FORMAT, DICTIONARY
cat mecab2.ins >> $man

# COPYRIGHT
echo '.SH "COPYRIGHT' >> $man
LANG=C help2man $cmdarg |grep -ie "^Copyright" |\
sed -e 's/Kudo/Kudo\n.br/' - >> $man

# SEE ALSO
cat mecab3.ins >> $man

