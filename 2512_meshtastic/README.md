# Meshtastic meshed wireless network over LoRaWAN

## Setup

* WifxL1 or Kerlink iFemtoCell gateway
* Wio-E5 mini boards acting as endpoints

## Meshtastic firmware

https://github.com/meshtastic/firmware is a submodule of this project: apply
the ``meshtastic_diff.patch`` to change the syncword (from 0x2b, default for Meshtastic,
to 0x34 for LoRaWAN), force the frequency to 868.{1,3} MHz in compliance
with the gateway channels and shrink the preamble word length from 16 to 8 bits.

Force the endpoint configuration to match the gateway channel settings:
```
meshtastic --set lora.modem_preset LONG_MODERATE \
           --set lora.bandwidth 125 \
           --set lora.spreadFactor 9 \
           --set lora.region EU_868 --port /dev/ttyUSB1
```

Notice: the Wio-E5 mini does not allow setting the frequency with ``--set lora.override_frequency 868100000`` since this option leads to an error message ``error reason: NO_INTERFACE``

Once configured, send a message with
```
meshtastic --sendtext "hello" --port /dev/ttyUSB1 
```

## WifxL1 gateway configuration

Set the gateway messages to JSON to be readable (``marshaler="json"`` in 
``chirpstack-gateway-bridge/30-integration.toml``) assuming the UDP Semtech
forwarder was selected.

On the host computer, edit ``/etc/chirpstack/region_eu868.toml`` and add ``v4_migrate=true` (host Chirpstack v4
compatible with WifxL1 v3)

Probe the MQTT broker with
```
mosquitto_sub -v -h localhost -t "#"
```
and grab the Meshtastic message starting with ``/////`` (if broadcasting to all, i.e. destination = ffff ffff) looking like
```
/////wocFVBa0KYmYjEAKsVlwbSva6DiG0VM
```

The ``receive-mqtt-packets.py`` script, updated from https://github.com/pdxlocations/Meshtastic-Python-Examples/ under MQTT, decrypts (AES-128) and decodes (protobuf) the message to 
```
ffffffff0a1c15505ad0a6266231002ac565c1b4af6ba0e21b454c
dest=    0xffffffff
source=  0x50151c0a
packetid=0x26a6d05a
nonce.hex: 5ad0a626000000000a1c155000000000
portnum: TEXT_MESSAGE_APP
payload: "Hello"
bitfield: 0
```
which is the correct payload. However, reception of the Meshtastic is random, and activating the DEBUG log output indicates
that most (if not all) Meshtastic packet analysis fail with an erroneous CRC.

## iFemtoCell

No dedicated setting was selected on the iFemtoCell other than UDP broadcasting the packets to the network server compouter running the MQTT caster.

Probing the MQTT caster:
```
eu868/gateway/7276ff0039070055/event/up{"phyPayload":"/////zQ+E1C92njIYm4AO4toKlutR2UwsW3eu0HQVt+g","txInfo":{"frequency":867700000,"modulation":{"lora":{"bandwidth":125000,"spreadingFactor":11,"codeRate":"CR_4_8"}}},"rxInfo":{"gatewayId":"7276ff0039070055","uplinkId":14339,"gwTime":"2026-01-12T16:50:18.879929Z","rssi":-112,"snr":-17,"channel":3,"board":3,"context":"LH6Q5A==","crcStatus":"CRC_OK"}}
eu868/gateway/7276ff0039070055/event/up{"phyPayload":"/////zQ+E1AhI+eXYm4AOws9rOvN6v7MBNm7KpYwa5++","txInfo":{"frequency":867700000,"modulation":{"lora":{"bandwidth":125000,"spreadingFactor":11,"codeRate":"CR_4_8"}}},"rxInfo":{"gatewayId":"7276ff0039070055","uplinkId":16388,"gwTime":"2026-01-12T17:06:54.701878Z","rssi":-112,"snr":-14,"channel":3,"board":3,"context":"Z9maFA==","crcStatus":"CRC_OK"}}
eu868/gateway/7276ff0039070055/event/up{"phyPayload":"/////snEVAk4VMCY24AO02i0sij70veblKVJ1Z3hF11ExheurpiPuEZDdBjMtM4FvPR9N2niYhA8IKxiQMLxlDIIXOerdwAn/rk","txInfo":{"frequency":867700000,"modulation":{"lora":{"bandwidth":125000,"spreadingFactor":11,"codeRate":"CR_4_8"}}},"rxInfo":{"gatewayId":"7276ff0039070055","uplinkId":16644,"gwTime":"2026-01-12T17:06:57.688590Z","rssi":-114,"snr":-15.5,"channel":3,"board":3,"context":"aAcs7A==","crcStatus":"CRC_OK"}}
```
which are correctly decoded respectively as
```
portnum: TEXT_MESSAGE_APP
payload: "hello world"
portnum: TEXT_MESSAGE_APP
payload: "hello world"
portnum: TEXT_MESSAGE_APP
payload: "got msg \'hello world\' with rxSnr: 7.5 and hopLimit: 3"
```
since after the second message, a second endpoint was activated in ``--reply`` mode. All messages seem to be detected with this
hardware.
