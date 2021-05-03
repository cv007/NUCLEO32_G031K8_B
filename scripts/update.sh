#!/bin/bash

SCRIPTDIR=$(pwd)
export SCRIPTDIR

. ./common.sh

# to source folder one level below
cd ..

# do everything from this file

# compile each source file

shopt -s nullglob;
for i in *.cpp; do $SCRIPTDIR/src2obj.sh $i || exit 1; done
for i in *.c; do $SCRIPTDIR/src2obj.sh $i || exit 1; done

#create elf
$SCRIPTDIR/objs2elf.sh || exit 1
[[ -e $SCRIPTDIR/program.sh ]] && $SCRIPTDIR/program.sh

