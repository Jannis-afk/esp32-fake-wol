#!/usr/bin/env python3
"""
ESP32 Smart PC Power Controller - Test Script

This script tests all the API endpoints of your ESP32 controller
to ensure everything is working correctly.

Usage:
    python test_controller.py [ESP32_IP_ADDRESS]

Example:
    python test_controller.py 192.168.1.100
"""

import requests
import json
import sys
import time
from typing import Optional

class ESP32ControllerTester:
    def __init__(self, base_url: str):
        """Initialize the tester with the ESP32's base URL."""
        self.base_url = base_url.rstrip('/')
        self.session = requests.Session()
        self.session.timeout = 10
        
    def test_connection(self) -> bool:
        """Test basic connectivity to the ESP32."""
        try:
            response = self.session.get(f"{self.base_url}/")
            if response.status_code == 200:
                print("✅ Connection successful - Web interface accessible")
                return True
            else:
                print(f"❌ Connection failed - Status code: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"❌ Connection failed - Error: {e}")
            return False
    
    def get_status(self) -> Optional[dict]:
        """Get current PC status."""
        try:
            response = self.session.get(f"{self.base_url}/status")
            if response.status_code == 200:
                status = response.json()
                print(f"✅ Status retrieved: {json.dumps(status, indent=2)}")
                return status
            else:
                print(f"❌ Status request failed - Status code: {response.status_code}")
                return None
        except requests.exceptions.RequestException as e:
            print(f"❌ Status request failed - Error: {e}")
            return None
        except json.JSONDecodeError as e:
            print(f"❌ Invalid JSON response - Error: {e}")
            return None
    
    def test_power_on(self) -> bool:
        """Test power on functionality."""
        try:
            print("🔄 Testing power on...")
            response = self.session.post(f"{self.base_url}/on")
            if response.status_code == 200:
                print("✅ Power on command sent successfully")
                return True
            else:
                print(f"❌ Power on failed - Status code: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"❌ Power on failed - Error: {e}")
            return False
    
    def test_shutdown(self) -> bool:
        """Test shutdown functionality."""
        try:
            print("🔄 Testing shutdown...")
            response = self.session.post(f"{self.base_url}/off")
            if response.status_code == 200:
                print("✅ Shutdown command sent successfully")
                return True
            else:
                print(f"❌ Shutdown failed - Status code: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"❌ Shutdown failed - Error: {e}")
            return False
    
    def test_force_off(self) -> bool:
        """Test force off functionality."""
        try:
            print("🔄 Testing force off...")
            response = self.session.post(f"{self.base_url}/forceoff")
            if response.status_code == 200:
                print("✅ Force off command sent successfully")
                return True
            else:
                print(f"❌ Force off failed - Status code: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"❌ Force off failed - Error: {e}")
            return False
    
    def get_config(self) -> Optional[dict]:
        """Get current configuration."""
        try:
            response = self.session.get(f"{self.base_url}/get_config")
            if response.status_code == 200:
                config = response.json()
                print(f"✅ Configuration retrieved: {json.dumps(config, indent=2)}")
                return config
            else:
                print(f"❌ Config request failed - Status code: {response.status_code}")
                return None
        except requests.exceptions.RequestException as e:
            print(f"❌ Config request failed - Error: {e}")
            return None
        except json.JSONDecodeError as e:
            print(f"❌ Invalid JSON response - Error: {e}")
            return None
    
    def run_full_test(self) -> bool:
        """Run all tests and return overall success status."""
        print("🚀 Starting ESP32 Controller Test Suite")
        print("=" * 50)
        
        # Test basic connectivity
        if not self.test_connection():
            return False
        
        print()
        
        # Get initial status
        initial_status = self.get_status()
        if initial_status is None:
            return False
        
        print()
        
        # Get configuration
        config = self.get_config()
        if config is None:
            return False
        
        print()
        
        # Test power control functions (only if PC is off)
        if initial_status.get('state') == 'off':
            print("💡 PC is currently off - testing power on...")
            if not self.test_power_on():
                return False
            
            # Wait a bit for the command to take effect
            time.sleep(2)
            
            # Check new status
            new_status = self.get_status()
            if new_status:
                print(f"📊 Status after power on: {new_status.get('stateText', 'Unknown')}")
        
        elif initial_status.get('state') == 'on':
            print("💡 PC is currently on - testing shutdown...")
            if not self.test_shutdown():
                return False
            
            # Wait a bit for the command to take effect
            time.sleep(2)
            
            # Check new status
            new_status = self.get_status()
            if new_status:
                print(f"📊 Status after shutdown: {new_status.get('stateText', 'Unknown')}")
        
        print()
        print("✅ All tests completed successfully!")
        return True

def main():
    """Main function to run the test suite."""
    if len(sys.argv) != 2:
        print("Usage: python test_controller.py [ESP32_IP_ADDRESS]")
        print("Example: python test_controller.py 192.168.1.100")
        sys.exit(1)
    
    esp32_ip = sys.argv[1]
    
    # Validate IP address format (basic)
    if not esp32_ip.replace('.', '').isdigit():
        print("❌ Invalid IP address format")
        sys.exit(1)
    
    # Create tester instance
    tester = ESP32ControllerTester(f"http://{esp32_ip}")
    
    # Run tests
    try:
        success = tester.run_full_test()
        if success:
            print("\n🎉 All tests passed! Your ESP32 controller is working correctly.")
            sys.exit(0)
        else:
            print("\n❌ Some tests failed. Check the output above for details.")
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\n⏹️  Testing interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n💥 Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
