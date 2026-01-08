#!/usr/bin/env python3
"""
DoIP Test Client for ISO 13400
Tests UDP vehicle discovery and TCP diagnostic communication
"""

import socket
import struct
import sys
import time
from typing import Optional, Tuple

# DoIP Protocol Constants
DOIP_PROTOCOL_VERSION = 0x03
DOIP_INVERSE_PROTOCOL_VERSION = 0xFC

# UDP Ports (per ISO 13400)
#  - DoIP Entity (server) listens on 13400 for requests
#  - Test Equipment (client) listens on 13401 for announcements/responses
DOIP_UDP_ENTITY_PORT = 13400
DOIP_UDP_TEST_EQUIPMENT_PORT = 13401

# Payload Types
DOIP_VEHICLE_IDENTIFICATION_REQUEST = 0x0001
DOIP_VEHICLE_IDENTIFICATION_RESPONSE_EID = 0x0004
DOIP_VEHICLE_ANNOUNCEMENT = 0x0004
DOIP_ROUTING_ACTIVATION_REQUEST = 0x0005
DOIP_ROUTING_ACTIVATION_RESPONSE = 0x0006
DOIP_DIAGNOSTIC_MESSAGE = 0x8001
DOIP_DIAGNOSTIC_MESSAGE_ACK = 0x8002
DOIP_DIAGNOSTIC_MESSAGE_NACK = 0x8003

# Routing Activation Types
ROUTING_ACTIVATION_DEFAULT = 0x00
ROUTING_ACTIVATION_WWH_OBD = 0x01

# UDS Service IDs
UDS_READ_DATA_BY_ID = 0x22

# Timeouts
# Announcements are often sent at multi-second intervals; use generous defaults
UDP_ANNOUNCEMENT_TIMEOUT = 5.0   # seconds
UDP_RESPONSE_TIMEOUT = 5.0       # seconds
TCP_RESPONSE_TIMEOUT = 5.0       # seconds


class DoIPHeader:
    """DoIP Generic Header"""
    FORMAT = "!BBHI"
    SIZE = 8

    def __init__(self, payload_type: int, payload_length: int):
        self.protocol_version = DOIP_PROTOCOL_VERSION
        self.inverse_protocol_version = DOIP_INVERSE_PROTOCOL_VERSION
        self.payload_type = payload_type
        self.payload_length = payload_length

    def pack(self) -> bytes:
        return struct.pack(
            self.FORMAT,
            self.protocol_version,
            self.inverse_protocol_version,
            self.payload_type,
            self.payload_length
        )

    @staticmethod
    def unpack(data: bytes) -> 'DoIPHeader':
        if len(data) < DoIPHeader.SIZE:
            raise ValueError("Data too short for DoIP header")

        proto_ver, inv_proto_ver, payload_type, payload_length = struct.unpack(
            DoIPHeader.FORMAT, data[:DoIPHeader.SIZE]
        )

        header = DoIPHeader(payload_type, payload_length)
        header.protocol_version = proto_ver
        header.inverse_protocol_version = inv_proto_ver
        return header


class DoIPClient:
    def __init__(self, source_address: int = 0x0E80):
        self.source_address = source_address
        self.target_address: Optional[int] = None
        self.server_ip: Optional[str] = None
        self.tcp_port: int = 13400
        self.tcp_socket: Optional[socket.socket] = None

    def listen_for_announcement(self, timeout: float = UDP_ANNOUNCEMENT_TIMEOUT) -> bool:
        """Listen for DoIP vehicle announcements on UDP"""
        print(f"Listening for vehicle announcements (timeout: {timeout}s) on fixed port {DOIP_UDP_TEST_EQUIPMENT_PORT}...")

        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        if hasattr(socket, "SO_REUSEPORT"):
            try:
                udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            except OSError:
                pass

        try:
            # Announcements are sent by DoIP entities to port 13401
            udp_socket.bind(('', DOIP_UDP_TEST_EQUIPMENT_PORT))
            # Poll in short intervals up to the provided timeout
            deadline = time.time() + timeout
            udp_socket.settimeout(0.5)
            while time.time() < deadline:
                try:
                    data, addr = udp_socket.recvfrom(4096)
                except socket.timeout:
                    continue

                print(f"Received announcement from {addr[0]}:{addr[1]}")

                if self._parse_vehicle_announcement(data, addr[0]):
                    return True

        except socket.timeout:
            print("No announcement received within timeout")
            return False
        except Exception as e:
            print(f"Error listening for announcement: {e}")
            return False
        finally:
            udp_socket.close()

        return False

    def send_vehicle_identification_request(self, broadcast_addr: str = '255.255.255.255') -> bool:
        """Send vehicle identification request via UDP broadcast"""
        print("Sending vehicle identification request...")

        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Prefer binding to 13401 so responses arrive on the standard test equipment port
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        if hasattr(socket, "SO_REUSEPORT"):
            try:
                udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            except OSError:
                pass
        bound_to_13401 = False
        try:
            udp_socket.bind(('', DOIP_UDP_TEST_EQUIPMENT_PORT))
            bound_to_13401 = True
        except Exception as e:
            print(f"Warning: bind to UDP {DOIP_UDP_TEST_EQUIPMENT_PORT} failed ({e}); using ephemeral source port for send.")

        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        udp_socket.settimeout(UDP_RESPONSE_TIMEOUT)

        # Create a dedicated receive socket bound to 13401 if the send socket couldn't bind
        recv_socket = None
        if not bound_to_13401:
            try:
                recv_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                recv_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                if hasattr(socket, "SO_REUSEPORT"):
                    try:
                        recv_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
                    except OSError:
                        pass
                recv_socket.bind(('', DOIP_UDP_TEST_EQUIPMENT_PORT))
            except Exception as e:
                print(f"Error: unable to bind a receive socket to UDP {DOIP_UDP_TEST_EQUIPMENT_PORT} ({e}). Announcements/responses to {DOIP_UDP_TEST_EQUIPMENT_PORT} will not be received.")
                # Fall back to using the send socket for any responses (ephemeral port)
                recv_socket = udp_socket

        try:
            # Build vehicle identification request (no payload)
            header = DoIPHeader(DOIP_VEHICLE_IDENTIFICATION_REQUEST, 0)
            message = header.pack()

            # Requests go to the DoIP Entity port 13400
            udp_socket.sendto(message, (broadcast_addr, DOIP_UDP_ENTITY_PORT))
            print(f"Request sent to {broadcast_addr}:{DOIP_UDP_ENTITY_PORT}")

            # Wait for response(s) up to timeout window
            deadline = time.time() + UDP_RESPONSE_TIMEOUT
            (recv_socket or udp_socket).settimeout(0.5)
            while time.time() < deadline:
                try:
                    data, addr = (recv_socket or udp_socket).recvfrom(4096)
                except socket.timeout:
                    continue

                print(f"Received response from {addr[0]}:{addr[1]}")
                if self._parse_vehicle_announcement(data, addr[0]):
                    return True

        except socket.timeout:
            print("No response received to identification request")
            return False
        except Exception as e:
            print(f"Error during identification request: {e}")
            return False
        finally:
            udp_socket.close()
            if recv_socket and recv_socket is not udp_socket:
                recv_socket.close()

        return False

    def _parse_vehicle_announcement(self, data: bytes, server_ip: str) -> bool:
        """Parse vehicle announcement/identification response"""
        try:
            header = DoIPHeader.unpack(data)

            if header.payload_type not in [DOIP_VEHICLE_ANNOUNCEMENT, DOIP_VEHICLE_IDENTIFICATION_RESPONSE_EID]:
                print(f"Unexpected payload type: 0x{header.payload_type:04X}")
                return False

            payload = data[DoIPHeader.SIZE:]

            if len(payload) < 32:  # Minimum length for vehicle announcement
                print("Payload too short for vehicle announcement")
                return False

            # Parse VIN, Logical Address, EID, GID
            vin = payload[0:17].decode('ascii', errors='ignore')
            logical_address = struct.unpack("!H", payload[17:19])[0]
            eid = payload[19:25].hex()
            gid = payload[25:31].hex()

            print(f"  VIN: {vin}")
            print(f"  Logical Address: 0x{logical_address:04X}")
            print(f"  EID: {eid}")
            print(f"  GID: {gid}")

            # assert(vin == "WVWZZZ1JZ3W386752")
            # assert(eid == 123456)
            # assert(eid == 654321)

            self.server_ip = server_ip
            self.target_address = logical_address

            # Further Action Byte (if present)
            if len(payload) > 31:
                further_action = payload[31]
                print(f"  Further Action: 0x{further_action:02X}")

            return True

        except Exception as e:
            print(f"Error parsing vehicle announcement: {e}")
            return False

    def connect_tcp(self) -> bool:
        """Open TCP connection to DoIP server"""
        if not self.server_ip or not self.target_address:
            print("No server IP or target address available")
            return False

        print(f"\nConnecting to {self.server_ip}:{self.tcp_port}...")

        try:
            self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.tcp_socket.settimeout(TCP_RESPONSE_TIMEOUT)
            self.tcp_socket.connect((self.server_ip, self.tcp_port))
            print("TCP connection established")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False

    def send_routing_activation(self) -> bool:
        """Send routing activation request"""
        if not self.tcp_socket:
            print("No TCP connection")
            return False

        print("\nSending routing activation request...")

        try:
            # Payload: Source Address (2) + Activation Type (1) + Reserved (4)
            payload = struct.pack("!HBI", self.source_address, ROUTING_ACTIVATION_DEFAULT, 0)
            header = DoIPHeader(DOIP_ROUTING_ACTIVATION_REQUEST, len(payload))
            message = header.pack() + payload

            self.tcp_socket.sendall(message)

            # Wait for response
            response = self.tcp_socket.recv(4096)
            header = DoIPHeader.unpack(response)

            if header.payload_type != DOIP_ROUTING_ACTIVATION_RESPONSE:
                print(f"Unexpected response type: 0x{header.payload_type:04X}")
                return False

            payload = response[DoIPHeader.SIZE:]
            tester_addr, entity_addr, response_code = struct.unpack("!HHB", payload[:5])

            print(f"  Response Code: 0x{response_code:02X}")
            print(f"  Tester Address: 0x{tester_addr:04X}")
            print(f"  Entity Address: 0x{entity_addr:04X}")

            if response_code == 0x10:  # Successfully activated
                print("  Status: Successfully activated")
                return True
            else:
                print(f"  Status: Activation failed")
                return False

        except Exception as e:
            print(f"Error during routing activation: {e}")
            return False

    def send_uds_rdbi(self, did: int = 0xF190) -> bool:
        """Send UDS Read Data By Identifier request"""
        if not self.tcp_socket or not self.target_address:
            print("No TCP connection or target address")
            return False

        print(f"\nSending UDS RDBI request for DID 0x{did:04X}...")

        try:
            # UDS message: Service ID + DID
            uds_message = struct.pack("!BH", UDS_READ_DATA_BY_ID, did)

            # DoIP Diagnostic Message: Source Addr + Target Addr + UDS Data
            payload = struct.pack("!HH", self.source_address, self.target_address) + uds_message
            header = DoIPHeader(DOIP_DIAGNOSTIC_MESSAGE, len(payload))
            message = header.pack() + payload

            self.tcp_socket.sendall(message)

            # Wait for diagnostic message positive/negative acknowledgement
            response = self.tcp_socket.recv(4096)
            header = DoIPHeader.unpack(response)

            if header.payload_type == DOIP_DIAGNOSTIC_MESSAGE_NACK:
                payload = response[DoIPHeader.SIZE:]
                nack_code = payload[4] if len(payload) > 4 else 0
                print(f"  Diagnostic message NACK: 0x{nack_code:02X}")
                return False

            response = self.tcp_socket.recv(4096)
            header = DoIPHeader.unpack(response)

            # Wait for actual UDS response
            if header.payload_type != DOIP_DIAGNOSTIC_MESSAGE:
                response += self.tcp_socket.recv(4096)
                # Find diagnostic message in response
                if len(response) < DoIPHeader.SIZE:
                    print("Response too short")
                    return False
                header = DoIPHeader.unpack(response)

            if header.payload_type == DOIP_DIAGNOSTIC_MESSAGE:
                payload = response[DoIPHeader.SIZE:]
                source_addr, target_addr = struct.unpack("!HH", payload[:4])
                uds_response = payload[4:]

                print(f"  Source: 0x{source_addr:04X}, Target: 0x{target_addr:04X}")
                print(f"  UDS Response: {uds_response.hex()}")

                if len(uds_response) > 0:
                    service_id = uds_response[0]
                    if service_id == 0x62:  # Positive response
                        print(f"  Service: Positive Response (0x62)")
                        if len(uds_response) >= 3:
                            resp_did = struct.unpack("!H", uds_response[1:3])[0]
                            data = uds_response[3:]
                            print(f"  DID: 0x{resp_did:04X}")
                            print(f"  Data: {data.hex()}")
                            print(f"  Data (ASCII): {data.decode('ascii', errors='ignore')}")
                        return True
                    elif service_id == 0x7F:  # Negative response
                        print(f"  Service: Negative Response (0x7F)")
                        if len(uds_response) >= 3:
                            print(f"  NRC: 0x{uds_response[2]:02X}")
                        return False
            else:
                print(f"Unexpected response type: 0x{header.payload_type:04X}")
                return False

        except Exception as e:
            print(f"Error during UDS RDBI: {e}")
            return False

        return False

    def close_tcp(self):
        """Close TCP connection"""
        if self.tcp_socket:
            print("\nClosing TCP connection...")
            self.tcp_socket.close()
            self.tcp_socket = None


def main():
    print("DoIP Test Client")
    print("=" * 50)

    # Check for loopback mode argument
    use_loopback = False
    if len(sys.argv) > 1 and sys.argv[1] in ['--loopback', '-l', 'loopback']:
        use_loopback = True
        print("Running in LOOPBACK mode (127.0.0.1)")
    else:
        print("Running in BROADCAST mode (255.255.255.255)")

    client = DoIPClient()

    # Step 1: Listen for vehicle announcements
    announcement_received = client.listen_for_announcement()

    # Step 2: If no announcement, send identification request
    if not announcement_received:
        print("\nNo announcement received, sending identification request...")
        broadcast_addr = '127.0.0.1' if use_loopback else '255.255.255.255'
        identification_received = client.send_vehicle_identification_request(broadcast_addr)

        if not identification_received:
            print("\nERROR: Neither announcement nor identification response received")
            sys.exit(1)

    print("\n" + "=" * 50)
    print("Vehicle discovered successfully!")
    print("=" * 50)

    # Step 3: Open TCP connection
    if not client.connect_tcp():
        print("\nERROR: Failed to establish TCP connection")
        sys.exit(1)

    # Step 4: Send routing activation request
    if not client.send_routing_activation():
        print("\nERROR: Routing activation failed")
        client.close_tcp()
        sys.exit(1)

    # Step 5: Send UDS RDBI request
    if not client.send_uds_rdbi(0xF190):
        print("\nWARNING: UDS RDBI request failed or returned negative response")

    # Step 6: Close TCP connection
    client.close_tcp()

    print("\n" + "=" * 50)
    print("Test completed successfully!")
    print("=" * 50)
    sys.exit(0)


if __name__ == "__main__":
    main()