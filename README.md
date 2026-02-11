# ARM Cortex M1 firmware demo
This project uses a Gowin GW5AST FPGA on the Tang Mega 60k and the ARM Cortex M1
softcore IP provided by Gowin. Note that any of the Gowin supported FPGAs can be
used. I used the Gowin IDE to configure all settings. The FPGA project can be
found here: https://github.com/akielaries/mega_60k_test


# Requirements
- A Gowin supported FPGA with the ARM Cortex M1 softcore configured with:
    - UART1
    - GPIO
    - Debug (Serial Wire)
- `arm-none-eabi-gcc` toolchain
- `gdb-multiarch`
- CMake

## Some notes
I prefer the blackmagic probe debugger and at the time of writing this
am using a fork I have of the project to properly detect and load ARM Cortex M1
devices. See here if interested: https://github.com/blackmagic-debug/blackmagic/issues/2190

The bulk of this code is generated from the Gowin MCU Designer (GMD). But much like
the STM32Cube and MX tools, it's not great so I modified the generated files and converted
the build to CMake.

# How to build
From the root directory:
```
cmake -S . -B build/
cmake --build build/
```
and that will produce the compile ELF under build/bin/softcore_fw_example.elf.

To load the firmware:
```
gdb-multiarch build/bin/softcore_fw_example.elf

# connect to your debugger
(gdb) target extended-remote /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v2.0.0-323-gf3b99649-dirty_8A8243AE-if00
Remote debugging using /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v2.0.0-323-gf3b99649-dirty_8A8243AE-if00

# scan for available SWD targets
(gdb) mon swd
Target voltage: 0.48V
Available Targets:
No. Att Driver
 1      Generic Cortex-M1 M1

# attach, load, start
(gdb) att 1
Attaching to program: /home/akiel/GMD_workspace/softcore_fw_example/build/bin/softcore_fw_example.elf, Remote target
⚠️ warning: while parsing target memory map (at line 1): Required element <memory> is missing
delay_ms (nms=50) at /home/akiel/GMD_workspace/softcore_fw_example/src/delay.c:89
89			while((temp & 0x01) && !(temp & (1 << 16)));
(gdb) load
Loading section .text, size 0x1cb4 lma 0x0
Loading section .ARM.extab, size 0x3c lma 0x1cb4
Loading section .ARM.exidx, size 0xd8 lma 0x1cf0
Loading section .data, size 0x10 lma 0x1dc8
Start address 0x000001cc, load size 7640
Transfer rate: 84 KB/sec, 694 bytes/write.
(gdb) c
Continuing.
```


# What it does
The code here will output a print message to the wired up serial console
```
$ tio -t -b 115200 /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0                                [12:38:50]
[12:38:51.993] tio 3.9
[12:38:51.993] Press ctrl-t q to quit
[12:38:52.003] Connected to /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
[12:38:55.858] �Hello from the cortex M1 soft core...
```

in addition to toggling GPIO pins 0 and 1. In my case they're blinking LEDs since I constrained
my FPGAs GPIO[0] and GPIO[1] pins to them.
