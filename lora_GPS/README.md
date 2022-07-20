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

```
openocd -f interface/stlink-v2.cfg -f target/stm32l1.cfg
```
and

```
gdb-multiarch  bin/im880b/lora_GPS.elf
  target extended-remote localhost:3333
```

## Application

RTK GPS using UBlox ZED-F9P, one acting as base station (static transmitter) and one acting as
rover (mobile receiver). We have observed that for GPS and Galileo active, using only RAWX messages
in UBX format at 115200 bauds, the payload is about 1200 bytes long or 12000 bits every second.
That's about half the datarate of LoRa set to 500 kHz bandwidth, SF=7 and CR=5 according to
https://www.rfwireless-world.com/calculators/LoRa-Data-Rate-Calculator.html (CR=1 on the website).
We have observed that different settings must be used for transmitter and receiver, namely 
32-byte transmission to avoid communication overhead latencies, and 16-byte reception to avoid
ISR stack corruption. The output of the LoRa receiver (base station messages) on the rover and the 
second UBlox receiver (rover) are fed to ``rtknavi_qt`` set to Kinematic mode, receiving UBX
messages from ``/dev/ttyUSB0`` and ``/dev/ttyACM0``.

Centimetric fix measurement (green) following the float solution (blue):

<img src="pictures_logs/rtknavi_qt.png">

Evolution of the relative position:

<img src="pictures_logs/rtknavi_qt_rel0.png">
<img src="pictures_logs/rtknavi_qt_rel1.png">
<img src="pictures_logs/rtknavi_qt_rel2.png">

... and after much longer still consistent measurements

<img src="pictures_logs/rtknavi_qt_rel4.png">
<img src="pictures_logs/rtknavi_qt_rel5.png">
