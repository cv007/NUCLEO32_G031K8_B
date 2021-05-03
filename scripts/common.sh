#!/bin/sh

# --- script name ---
script=$(basename $0)

# --- print functions ---
headerType(){ printf "[%-8s]" "$1"; }
headerFile(){ printf "[%-16s]" $script; }
printInfo(){
    [ "$2" ] || return
    headerFile
    headerType "$1"
    printf " %s\r\n" "$2"
}
exitErr(){ printInfo "ERROR" "$1"; exit 1; }

# --- set the gcc compiler full path+prefix ---
ARM=~/Programming/stm32/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi
export ARM

# --- folders to store things ---
objdir=$SCRIPTDIR/../obj
lssdir=$SCRIPTDIR/../lss
bindir=$SCRIPTDIR/../bin #elf,bin,hex
[ -e $objdir ] || mkdir $objdir
[ -e $lssdir ] || mkdir $lssdir
[ -e $bindir ] || mkdir $bindir
