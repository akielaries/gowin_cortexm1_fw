# "bootloader"
It's not really loading anything, it's just the initialized ITCM (instruction tightly coupled memory)
when the Cortex M1 softcore is configured in the IP menu. In this case it just prints the uptime
in seconds and blinks some LEDs
