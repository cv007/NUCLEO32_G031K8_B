#!/bin/bash
#program nuvleo32 via its virtual drive

. $SCRIPTDIR/common.sh

printInfo "PROGRAM" "project.bin"

[[ -e $bindir/project.bin ]] &&
    [[ -e /media/owner/NOD_G031K8/ ]] &&
    cp $bindir/project.bin /media/owner/NOD_G031K8/ ||
    exitErr "nucleo32 virtual drive not present"

