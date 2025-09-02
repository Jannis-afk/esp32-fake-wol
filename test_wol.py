#!/usr/bin/env python3
"""
Test script for ESP32 Wake-on-LAN functionality
This script sends WOL magic packets to test if the ESP32 detects them.
"""

import socket
import struct
import sys
import time

def create_magic_packet(mac_address):
    """Create a Wake-on-LAN magic packet"""
    # Remove any separators from MAC address
    mac = mac_address.replace(':', '').replace('-', '').replace('.', '')
    
    # Convert MAC address to bytes
    mac_bytes = bytes.fromhex(mac)
    
    # Magic packet structure: 6 bytes of 0xFF followed by 16 repetitions of MAC
    magic_packet = b'\xff' * 6 + mac_bytes * 16
    
    return magic_packet

def send_wol_packet(mac_address, broadcast_ip="255.255.255.255", port=9):
    """Send Wake-on-LAN magic packet"""
    magic_packet = create_magic_packet(mac_address)
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    try:
        # Send packet
        sock.sendto(magic_packet, (broadcast_ip, port))
        print(f"✓ WOL magic packet sent to {mac_address} via {broadcast_ip}:{port}")
        return True
    except Exception as e:
        print(f"✗ Failed to send WOL packet: {e}")
        return False
    finally:
        sock.close()

def test_wol_to_esp32(esp32_ip, target_mac):
    """Test WOL by sending packet to ESP32's network"""
    print(f"Testing WOL to ESP32 at {esp32_ip}")
    print(f"Target MAC: {target_mac}")
    print("=" * 50)
    
    # Get ESP32's network (assuming it's 192.168.4.x)
    network = ".".join(esp32_ip.split(".")[:-1]) + ".255"
    print(f"Broadcasting to network: {network}")
    
    # Send WOL packet
    success = send_wol_packet(target_mac, network, 9)
    
    if success:
        print("\nWOL packet sent successfully!")
        print("Check if:")
        print("1. ESP32 WOL LED is blinking")
        print("2. PC powers on (if connected)")
        print("3. Serial Monitor shows 'WOL magic packet detected'")
    else:
        print("\nFailed to send WOL packet")
    
    return success

def test_wol_broadcast(target_mac):
    """Test WOL with standard broadcast"""
    print(f"Testing WOL with standard broadcast")
    print(f"Target MAC: {target_mac}")
    print("=" * 50)
    
    # Send WOL packet to standard broadcast
    success = send_wol_packet(target_mac, "255.255.255.255", 9)
    
    if success:
        print("\nWOL packet sent successfully!")
        print("This should work if your network allows broadcast packets")
    else:
        print("\nFailed to send WOL packet")
    
    return success

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_wol.py <MAC_ADDRESS> [ESP32_IP]")
        print("Example: python test_wol.py AA:BB:CC:DD:EE:FF 192.168.4.1")
        print("\nMAC address formats supported:")
        print("  AA:BB:CC:DD:EE:FF")
        print("  AA-BB-CC-DD-EE-FF")
        print("  AABBCCDDEEFF")
        return
    
    mac_address = sys.argv[1]
    esp32_ip = sys.argv[2] if len(sys.argv) > 2 else "192.168.4.1"
    
    print("ESP32 Wake-on-LAN Test")
    print("=" * 50)
    
    # Test 1: Send WOL to ESP32's network
    print("\nTest 1: Sending WOL to ESP32's network")
    test_wol_to_esp32(esp32_ip, mac_address)
    
    # Wait a moment
    time.sleep(2)
    
    # Test 2: Standard broadcast
    print("\nTest 2: Standard broadcast WOL")
    test_wol_broadcast(mac_address)
    
    print("\n" + "=" * 50)
    print("WOL Test Complete!")
    print("\nTroubleshooting:")
    print("1. Ensure ESP32 is powered and connected to network")
    print("2. Check Serial Monitor for WOL detection messages")
    print("3. Verify MAC address is correctly configured in ESP32")
    print("4. Check if network allows UDP broadcast packets")
    print("5. Try visiting http://[ESP32_IP]/test_wol to manually trigger WOL")

if __name__ == "__main__":
    main()
