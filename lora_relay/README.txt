Program description:

- Endpoint is the sensor node
- Relay is the relay node
- EcouteSeule monitors messages coming from the gateway or from endpoints, for debugging
- simpleExample is a simple communication examples, can be used as a starting point for new
communication software.

Modified files:
```
build/pkg/semtech-loramac/src/mac/LoRaMac.c: Radio.SetChannel(868100000);
build/pkg/semtech-loramac/src/mac/region/RegionEU868.c: uint32_t frequency = 868100000;//rxConfig->Frequency;
build/pkg/semtech-loramac/src/mac/region/RegionEU868.c: //*channel = enabledChannels[randr( 0, nbEnabledChannels - 1 )];
build/pkg/semtech-loramac/src/mac/region/RegionEU868.c: *channel = enabledChannels[1];
build/pkg/semtech-loramac/src/mac/region/RegionEU868.h:#define EU868_JOIN_ACCEPT_DELAY1 1000
build/pkg/semtech-loramac/src/mac/region/RegionEU868.h:#define EU868_JOIN_ACCEPT_DELAY2 15000
```
and in this file the constants
```
#define EU868_LC1, #define EU868_LC2 and #define EU868_LC3 
```
are all set to 868100000

