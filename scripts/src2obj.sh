#!/bin/bash
# src2obj file.cpp
# output .o file

. $SCRIPTDIR/common.sh

[ "$1" ] || exitErr "usage: src2obj file.cpp (or file.c)"
name="${1%%.*}"
nameext="${1#*.}"

[ $nameext = "c" ] && compiler=gcc && . $SCRIPTDIR/options-gcc.txt
[ $nameext = "s" ] && compiler=gcc && . $SCRIPTDIR/options-gcc.txt
[ $nameext = "cpp" ] && compiler=g++ && . $SCRIPTDIR/options-g++.txt
[ $compiler ] || exitErr "no source file provided"

printInfo "COMPILE" "$1"
$ARM-$compiler $options -c "$1" -o $objdir/$name.o || exitErr "$1 compile failed"
$ARM-objdump -DzC $objdir/$name.o > $lssdir/$name.lss

