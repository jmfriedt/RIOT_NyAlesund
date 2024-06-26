1. connect the U-Blox receiver and check the UART port with ``dmesg``
```bash
$ dmesg | tail -1
[XXXXX.YYYYYY] cdc_acm 3-2.4:1.0: ttyACM1: USB ACM device
```

2. check the UART is accessible from wine:
```bash
$ ls -l $HOME/.wine/dosdevices/ | grep ACM1
lrwxrwxrwx 1 jmfriedt jmfriedt 12 Jun 23 10:03 com7 -> /dev/ttyACM1
```

3. From the U-Center installation directory launch wine with the executable as argument:
```bash
~/.wine/drive_c/Program Files (x86)/u-blox/u-center_v22.02$ wine ./u-center.exe
```

4. ``Connect`` to the COM port identified earlier (First icon, second row below ``New'' File)

5. View -> Messages View -> NMEA right click -> Disable Child Messages and close Messages View window

6. View -> Messages View -> UBX right click -> Enable Child Messages and close Messages View window

7. View -> Messages View -> UBX-MON right click -> Disable Child Messages 

8. UBX-NAV right click -> Disable Child Messages

9. UBX-RXM-MEASX right click -> Disable Message. After that, only UBX-RXM-RAWX should be enabled

<img src="ucenter/ucenter_rawx.png">

10. UBX-CFG-MSG -> select 02-15 RXM-RAWX and enable UART1, Send (USB should
have been active already)

<img src="ucenter/ucenter_msg.png">

11. UBX-CFG-PRT -> Target 1-UART1: set Baudrate to 115200 and keep all other
settings (8N1), Protocol In UBX only and Protocol Out UBX only (disable NMEA), Send

<img src="ucenter/ucenter_ports.png">

12. UBX-CFG-GNSS -> Disable QZSS, SBAS, Beidou and GLONASS, keeping only GPS (L11C/A) and 
Galileo (E1) -- might not be necessary -- to reduce traffic, Send

<img src="ucenter/ucenter_gnss.png">

13. UBX-CFG-CFG -> Save Current Configuration to (click on) FLASH, Send

14. Disconnect, unplug USB, replug USB, reconnct and make sure the settings were saved (most
significantly PRT - 115200 bauds)

Since we have not managed to activate/deactivate GNSS constellation reception from U-Center, use ``gpsd``'s ``ubxtool`` to disable Galileo, GLONASS, Beidou, SBAS, and QZSS using 
```bash
ubxtool -d GLONASS
ubxtool -d BEIDOU
ubxtool -d QZSS
ubxtool -d SBAS
ubxtool -d GALILEO
```
While GPS remains active, we have not figured out how to disable L2C and keep only L1C/A, so we switch to a L1-only antenna on the station to reduce the amount of data transmitted.

When testing, u-center will no longer display the position but the pseudoranges are still
visible

<img src="ucenter/ucenter_pseudoranges.png">
