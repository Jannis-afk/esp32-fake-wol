#!/usr/bin/env python3
"""
Test script for ESP32 Captive Portal
This script helps verify that the captive portal is working correctly.
"""

import requests
import time
import sys

def test_captive_portal(esp32_ip="192.168.4.1"):
    """Test various captive portal endpoints"""
    
    print(f"Testing captive portal on ESP32 IP: {esp32_ip}")
    print("=" * 50)
    
    # Test endpoints that should redirect
    redirect_endpoints = [
        "/generate_204",
        "/fwlink", 
        "/hotspot-detect.html",
        "/ncsi.txt",
        "/connecttest.txt",
        "/redirect",
        "/portal",
        "/nonexistent_page"
    ]
    
    # Test endpoints that should return content
    content_endpoints = [
        "/",
        "/config", 
        "/test",
        "/success.txt"
    ]
    
    print("\nTesting redirect endpoints (should redirect to main page):")
    print("-" * 40)
    
    for endpoint in redirect_endpoints:
        try:
            response = requests.get(f"http://{esp32_ip}{endpoint}", 
                                 allow_redirects=False, timeout=5)
            if response.status_code == 302:
                location = response.headers.get('Location', 'No Location header')
                print(f"✓ {endpoint}: 302 redirect to {location}")
            else:
                print(f"✗ {endpoint}: {response.status_code} (expected 302)")
        except Exception as e:
            print(f"✗ {endpoint}: Error - {e}")
    
    print("\nTesting content endpoints (should return content):")
    print("-" * 40)
    
    for endpoint in content_endpoints:
        try:
            response = requests.get(f"http://{esp32_ip}{endpoint}", timeout=5)
            if response.status_code == 200:
                content_length = len(response.text)
                print(f"✓ {endpoint}: 200 OK ({content_length} chars)")
            else:
                print(f"✗ {endpoint}: {response.status_code} (expected 200)")
        except Exception as e:
            print(f"✗ {endpoint}: Error - {e}")
    
    print("\n" + "=" * 50)
    print("Captive Portal Test Complete!")
    print("\nTo test on your device:")
    print("1. Connect to ESP32-PC-Controller WiFi network")
    print("2. Open any website in your browser")
    print("3. You should be redirected to the ESP32 configuration page")
    print("4. If not, try visiting http://192.168.4.1 directly")

if __name__ == "__main__":
    # Allow custom IP address as command line argument
    esp32_ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.4.1"
    test_captive_portal(esp32_ip)
