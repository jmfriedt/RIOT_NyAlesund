# Meshtastic meshed wireless network over LoRaWAN

## Setup

* WifxL1 gateway
* Wio-E5 mini boards acting as endpoints

## Meshtastic firmware

https://github.com/meshtastic/firmware is a submodule of this project: apply
the ``meshtastic_diff.patch`` to change the syncword (from 0x2b, default for Meshtastic,
to 0x34 for LoRaWAN) and force the frequency to 868.{1,3} MHz in compliance
with the gateway channels

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

## Gateway configuration

Set the gateway messages to JSON to be readable (``marshaler="json"`` in 
``chirpstack-gateway-bridge/30-integration.toml``) assuming the UDP Semtech
forwarder was selected.

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
which is the correct payload.
