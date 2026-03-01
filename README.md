# ARM Cortex M1 firmware demo
This project uses a Gowin GW5AST FPGA on the Tang Mega 60k and the ARM Cortex M1
softcore IP provided by Gowin. Note that any of the Gowin supported FPGAs can be
used. I used the Gowin IDE to configure all settings

The FPGA project this code relies on can be found here: https://github.com/akielaries/mega_60k_test


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
[05:23:45] SystemCoreClock: 50 mHz
[05:23:45] magic: 0xDEADBEEF
[05:23:45] mfg_id: Gowin60K
[05:23:45] dev version: 0x00010000
[05:23:45] dev version: v1.0.0
[05:23:45] cheby version: v1.7.0
[05:23:45] DDR3 init
[05:23:45] DDR3 controller base: 0x88000000
[05:23:45] Pre-init: WR_EN=0 RD_EN=0 INIT=1
[05:23:45] Waiting for DDR3 initialization...
[05:23:45] SUCCESS: DDR3 initialized! INIT=0x00000001
[05:23:45] DDR3 init status: 1
[05:23:45] DDR3 test (16-byte aligned)
[05:23:45] Writing 6 blocks with 16-byte increment...
[05:23:45] Reading back...
[05:23:45] Results:
[05:23:45] Block 0: 0x91234567 0x89ABCDEF 0xFEDCBA98 0x76543210 OK
[05:23:45] Block 1: 0x66666666 0x88888888 0xEEEEEEEE 0xFFFFFFFF OK
[05:23:45] Block 2: 0x00000001 0x00000002 0x00000003 0x00000004 OK
[05:23:45] Block 3: 0x00000005 0x00000006 0x00000007 0x00000008 OK
[05:23:45] Block 4: 0x00000009 0x0000000A 0x0000000B 0x0000000C OK
[05:23:45] Block 5: 0x0000000D 0x0000000E 0x0000000F 0x00000010 OK
[05:23:45] initializing kernel...
[05:23:45] SysTick LOAD: 0x0000C34F
[05:23:45] SysTick CTRL: 0x00010007
[05:23:45] __StackLimit: 0x2000E000
[05:23:45] __StackTop:   0x20010000
[05:23:45] creating threads...
[05:23:45] system_time_ms before start: 39
[05:23:45] starting kernel...
[05:23:45] uptime: pin0
[05:23:45] pin1
[05:23:45] 0s
[05:23:45] pin0
[05:23:46] uptime: 1s
[05:23:46] pin0
[05:23:46] pin1
[05:23:46] pin0
[05:23:47] uptime: 2s
[05:23:47] pin0
[05:23:47] pin1
[05:23:47] pin0
[05:23:48] uptime: 3s
[05:23:48] pin0
[05:23:48] pin1
[05:23:48] pin0
[05:23:49] uptime: 4s
[05:23:49] pin0
[05:23:49] pin1
[05:23:49] pin0
```
