Meshcore firmware commit 0679dbeffc504d562d2f09eb072fdc223f8ffc2a

Apply ``diff.patch``

When filling the broadcast packet with values from 0 to 9: the LoRaWAN gateway receives
```
2026-09-01T18:07:01+0200 : eu868/gateway/0016c001ff10d6dc/event/up : {"tst":"2026-09-01T18:07:01.093270+0200","topic":"eu868/gateway/0016c001ff10d6dc/event/up","qos":0,"retain":false,"payloadlen":327,"payload":"{\"phyPayload\":\"PQAAAQIDBAUGBwgJ\",\"txInfo\":{\"frequency\":868100000,\"modulation\":{\"lora\":{\"bandwidth\":125000,\"spreadingFactor\":7,\"codeRate\":\"CR_4_5\"}}},\"rxInfo\":{\"gatewayId\":\"0016c001ff10d6dc\",\"uplinkId\":27711,\"gwTime\":\"2026-09-01T16:07:01.087722849Z\",\"rssi\":-92,\"snr\":13.25,\"rfChain\":1,\"context\":\"bbJlnw==\",\"crcStatus\":\"CRC_OK\"}}"}
2026-09-01T18:07:21+0200 : eu868/gateway/0016c001ff10d6dc/event/up : {"tst":"2026-09-01T18:07:21.963919+0200","topic":"eu868/gateway/0016c001ff10d6dc/event/up","qos":0,"retain":false,"payloadlen":343,"payload":"{\"phyPayload\":\"AAAAAAABRQAAAI8uCP7/WBeosh9lD3jS\",\"txInfo\":{\"frequency\":868100000,\"modulation\":{\"lora\":{\"bandwidth\":125000,\"spreadingFactor\":7,\"codeRate\":\"CR_4_5\"}}},\"rxInfo\":{\"gatewayId\":\"0016c001ff10d6dc\",\"uplinkId\":11656,\"gwTime\":\"2026-09-01T16:07:21.958465396Z\",\"rssi\":-91,\"snr\":13.75,\"rfChain\":1,\"context\":\"bvDVbA==\",\"crcStatus\":\"CRC_OK\"}}"}
```

when the repeater indicates
```
>>> [REPEATER] Paquet recu ! Type: 15 | RSSI: -34 | Hops: 0 <<<

[AUTO-DISCOVERY] Voisins actuels : -none-

>>> [REPEATER] Paquet recu ! Type: 0 | RSSI: -106 | Hops: 0 <<<
JMF allow packet fwd

<<< [REPEATER] Paquet transmis (TX) ! Type: 0 <<<
```
and the sensor node
```
Temperature sent
```

The sensor to gateway packet is decoded as
```
echo "PQAAAQIDBAUGBwgJ" | base64 -d | xxd
00000000: 3d00 0001 0203 0405 0607 0809            =...........
```

but the packet forwarded ``AAAAAAABRQAAAI8uCP7/WBeosh9lD3jS`` in not understood.
