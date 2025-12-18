# https://github.com/pdxlocations/Meshtastic-Python-Examples
import paho.mqtt.client as mqtt
import base64
import struct
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from meshtastic.protobuf import mqtt_pb2, mesh_pb2
from meshtastic import protocols

KEY = "AQ=="
# https://docs.rs/meshtastic/latest/meshtastic/protobufs/struct.ChannelSettings.html
# Those bytes are mapped using the following scheme: 0 = No crypto 1 = The special “default” channel key: {0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59, 0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01}
# matching https://github.com/pdxlocations/Meshtastic-Python-Examples/blob/main/MQTT/receive-mqtt-packets.py#L14
KEY = "1PG7OiApB1nwvP+rz05pAQ==" if KEY == "AQ==" else KEY

def decrypt_packet(nonce,_payload, key):
    key_bytes = base64.b64decode(key.encode('ascii'))

    # Build the nonce from message ID and sender
    # nonce = nonce_packet_id + nonce_from_node
    print(f"nonce.hex: {nonce.hex()}")

    # Decrypt the encrypted payload
    cipher = Cipher(algorithms.AES(key_bytes), modes.CTR(nonce), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted_bytes = decryptor.update(_payload) + decryptor.finalize()

    # Parse the decrypted bytes into a Data object
    # see https://protobuf.dev/programming-guides/encoding/
    data = mesh_pb2.Data()
    data.ParseFromString(decrypted_bytes)
    print(data)

b64="/////wocFVBa0KYmYjEAKsVlwbSva6DiG0VM"
b64="/////ypNElCbTU1+YjEACrvS43+WzgkQhJm5NsCiNVQGWFcwBf7O80kR8Z6Oqlcb0+XKPpHDEpr3b+NqSdbyKzoeweVGC6s="
#b64="/////ypNElCq9bEuYjEACmGecRPgMrOyiFcoKWDfyx08vdkAgnxROfRZnRmiEUeCr00SmqcPN8Y7lwav/BqruhFsps7Cwwhakii3bb4="
raw=base64.b64decode(b64)
print(raw.hex())
 
# Organization https://meshtastic.org/docs/overview/mesh-algo/
# 4-byte dest, 4-byte source, 4-byte packet_id (LE), 4-byte others
_to=raw[0:4] 
_to=int.from_bytes(_to,byteorder='little')
print(f"dest=    {hex(_to)}")
_from=raw[4:8] 
_from=int.from_bytes(_from,byteorder='little')
print(f"source=  {hex(_from)}")
_id=raw[8:12] # 0x2eb1f5aa
_id=int.from_bytes(_id,byteorder='little')
print(f"packetid={hex(_id)}")
nonce=struct.pack("<q",_id)+struct.pack("<q",_from)  # <: little endian
_payload=raw[16:]
decrypt_packet(nonce,_payload,KEY)
