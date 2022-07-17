CFLAGS="-DISR_STACK_SIZE=2048 -DSTDIO_UART_BAUDRATE=115200 -DSERIAL_BAUDRATE=115200" DRIVER=sx1272 BOARD=im880b make
stm32flash -w bin/im880b/lora_GPS.bin /dev/ttyUSB0
stm32flash -w bin/im880b/lora_GPS.bin /dev/ttyUSB1


openocd -f interface/stlink-v2.cfg -f target/stm32l1.cfg
gdb-multiarch  bin/im880b/lora_GPS.elf
  target extended-remote localhost:3333
