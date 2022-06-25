# STM32 NUCLEO G031K8 B
----------
#### This is a companion project to NUCLEO32_G031K8. In this case, the manufacturer header is used, along with the needed CMSIS headers. These extra headers are placed in a folder called cmsis, and the script gcc/g++ options are changed to -I(nclude) this folder. Now the ST mcu header can be used for both peripheral addresses and register structs, along with other defines. The cmsis headers allow using the common functions such as NVIC_EnableIRQ and so on.

#### You will see in the peripheral classes that the ST mcu header is put to use, and in the new Util.hpp header there can be seen the use of functions from the cmsis header. There may be more headers added over time.

#### Using these manufacurer and cmsis headers is a good compromise- we get to create C++ code and do not have to deal with creating our own register structs along with creating our own peripheral addresses. The register structs we create would be nicer because they would have bitfields, which eliminate mistakes in dealing with subsets of registers, and can still be done if wanted.

#### Also added a Printer class (initially) which used snprintf to do the work, and it would simply use the stack for buffer space on each use. Since it blocks on the virtual write function (hardware or software buffer is doing the blocking), the stack is a good place for the buffer. I then changed the Printer class to a cout style called Format, which requires no stdio functions and writes directly to the device/parent class via virtual put function. The simple encoder example in main.cpp compiles to about 3.6k with the Format version, but I do not recall what size the snprintf version was (may have been a little larger). The advantage of the Format class is you can have everything under your own control in a few hundred lines of header code without the need for stdio.h (which is quite complex in newlib).

