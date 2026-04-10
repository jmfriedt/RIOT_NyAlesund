# uses https://pypi.org/project/meshcoredecoder/

from meshcoredecoder import MeshCoreDecoder
from meshcoredecoder.types.enums import PayloadType
from meshcoredecoder.utils.enum_names import get_route_type_name, get_payload_type_name, get_device_role_name
import json
from meshcoredecoder.crypto import MeshCoreKeyStore
from meshcoredecoder.types.crypto import DecryptionOptions
from meshcoredecoder.types.enums import PayloadType

# Decode a MeshCore packet
hex_data = '15001177c468dd68564b354b8a5fe04e7a8c07801451789940deb27bb88c6b1c57512a20d3'

packet = MeshCoreDecoder.decode(hex_data)

print(f"Route Type: {get_route_type_name(packet.route_type)}")
print(f"Payload Type: {get_payload_type_name(packet.payload_type)}")
print(f"Message Hash: {packet.message_hash}")

if packet.payload_type == PayloadType.Advert and packet.payload.get('decoded'):
    advert = packet.payload['decoded']
    print(f"Device Name: {advert.app_data.get('name')}")
    print(f"Device Role: {get_device_role_name(advert.app_data.get('device_role'))}")
    if advert.app_data.get('location'):
        location = advert.app_data['location']
        print(f"Location: {location['latitude']}, {location['longitude']}")


packet_dict = packet.to_dict()
print(json.dumps(packet_dict, indent=2, default=str))

# Create a key store with channel secret keys
key_store = MeshCoreKeyStore({
    'channel_secrets': [
        '8b3387e9c5cdea6ac9e5edbaa115cd72',  # Public channel (channel hash 11)
    ]
})

# Decode encrypted GroupText message
options = DecryptionOptions(key_store=key_store)
encrypted_packet = MeshCoreDecoder.decode(hex_data, options)

if encrypted_packet.payload_type == PayloadType.GroupText and encrypted_packet.payload.get('decoded'):
    group_text = encrypted_packet.payload['decoded']

    if group_text.decrypted:
        print(f"Sender: {group_text.decrypted.get('sender')}")
        print(f"Message: {group_text.decrypted.get('message')}")
        print(f"Timestamp: {group_text.decrypted.get('timestamp')}")
    else:
        print('Message encrypted (no key available)')
