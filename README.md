# RIOT_tests
RIOT OS communictation tests involving timers and RS232 aimed at trasfering sentences from a U-Blox GPS receiver over sub-GHz LoRa link provided by the SX1272 of an im880b board.

## Compiling and executing

Store the repository in the ``tests/`` subdirectory of RIOT-OS. The ``readme.jmf`` text file holds the commands for compiling and transfering the program either through RS232 link (using ``stm32flash`` with GNU/Linux) or ST-Link/v2 with ``openocd``.

The ``application`` subdirectory provides a test-program to be executed on a PC connected to an im880b through RS232 for sending known data and assessing communication bandwidth or data loss.
