#!/bin/bash

. $SCRIPTDIR/common.sh

printInfo "LINK" "project.elf"
[[ -e $SCRIPTDIR/../linker-script.txt ]] || exitErr "linker-script.txt not found"

$ARM-g++ -T$SCRIPTDIR/../linker-script.txt --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -Wl,--gc-sections -Wl,-Map=$bindir/project.map.txt -fno-exceptions -o bin/project.elf $objdir/*.o || exit 1

$ARM-objdump -DzC $bindir/project.elf > $lssdir/project.lss
$ARM-objcopy -O ihex $bindir/project.elf $bindir/project.hex
$ARM-objcopy -O binary $bindir/project.elf $bindir/project.bin
$ARM-size $bindir/project.elf > $bindir/size.txt

echo
cat $bindir/size.txt
echo
