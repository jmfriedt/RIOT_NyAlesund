## Compiling

```
CFLAGS="-DISR_STACK_SIZE=2048 -DSTDIO_UART_BAUDRATE=115200 -DSERIAL_BAUDRATE=115200" DRIVER=sx1272 BOARD=im880b make
```

## Flashing through serial port

```
stm32flash -w bin/im880b/lora_GPS.bin /dev/ttyUSB0
stm32flash -w bin/im880b/lora_GPS.bin /dev/ttyUSB1
```

## Flashing through STLink/v2

Using either a STLink/v2 interface (left half of the figure) OR a Discovery
Board fitted with the STLink connector (e.g. STM32F407G-Discovery) whose
upper half provides debugging capability **after opening the two jumpers**
for selecting either STLink or Discovery capability (see
page 17 of https://www.st.com/resource/en/user_manual/dm00039084-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf)

```
openocd -f interface/stlink-v2.cfg -f target/stm32l1.cfg
```
and

```
gdb-multiarch  bin/im880b/lora_GPS.elf
  target extended-remote localhost:3333
```

<img src="pinout.png">

<img src="setup.jpg">

