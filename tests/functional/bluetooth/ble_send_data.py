#!/usr/bin/env python3
"""
BLE Client for RoastABean ESP32 Device
Connects to the ESP32 and sends roast profile JSON data via Nordic UART Service (NUS)
"""

import asyncio
import json
import argparse
from datetime import datetime
from bleak import BleakClient, BleakScanner

# Nordic UART Service UUIDs (matching your ESP32 firmware)
NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # Write to ESP32
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # Receive from ESP32

DEVICE_NAME = "RoastABean"
MAX_MTU = 512  # Maximum bytes per BLE packet (will be negotiated)


async def scan_and_select_device(timeout: float = 5.0):
    """Scan for BLE devices and let user select one"""
    print(f"Scanning for BLE devices (timeout: {timeout}s)...\n")
    
    devices = await BleakScanner.discover(timeout=timeout)
    
    if not devices:
        print("✗ No BLE devices found!")
        return None
    
    # Filter out devices without names and sort by name
    named_devices = [d for d in devices if d.name]
    named_devices.sort(key=lambda x: x.name.lower())
    
    # Display available devices
    print(f"Found {len(named_devices)} device(s):\n")
    for i, device in enumerate(named_devices):
        rssi = f"RSSI: {device.rssi}" if hasattr(device, 'rssi') and device.rssi else ""
        print(f"  [{i}] {device.name:30s} ({device.address}) {rssi}")
    
    # Get user selection
    print("\nEnter device index to connect (or 'q' to quit): ", end='')
    try:
        user_input = input().strip()
        
        if user_input.lower() == 'q':
            print("Cancelled by user")
            return None
        
        index = int(user_input)
        
        if 0 <= index < len(named_devices):
            selected = named_devices[index]
            print(f"\n✓ Selected: {selected.name} ({selected.address})")
            return selected
        else:
            print(f"✗ Invalid index. Please enter a number between 0 and {len(named_devices)-1}")
            return None
            
    except ValueError:
        print("✗ Invalid input. Please enter a number or 'q'")
        return None
    except KeyboardInterrupt:
        print("\nCancelled by user")
        return None


async def send_json_data(address: str, json_data: dict, chunk_size: int = 244):
    """
    Connect to BLE device and send JSON data
    
    Args:
        address: BLE device address
        json_data: Dictionary to send as JSON
        chunk_size: Size of each chunk (default 244 bytes, safe for MTU 247)
    """
    json_string = json.dumps(json_data, separators=(',', ':'))
    json_bytes = json_string.encode('utf-8')
    
    print(f"\nJSON size: {len(json_bytes)} bytes")
    print(f"JSON preview: {json_string[:100]}...")
    
    async with BleakClient(address) as client:
        print(f"\n✓ Connected to {address}")
        
        # Check if services are available
        services = client.services
        nus_service = services.get_service(NUS_SERVICE_UUID)
        
        if not nus_service:
            print("✗ Nordic UART Service not found!")
            return False
        
        print("✓ Nordic UART Service found")
        
        # Get MTU size
        mtu = client.mtu_size
        print(f"✓ MTU size: {mtu} bytes")
        
        # Adjust chunk size based on MTU (leave 3 bytes for ATT overhead)
        actual_chunk_size = min(chunk_size, mtu - 3)
        
        # Split data into chunks if needed
        total_chunks = (len(json_bytes) + actual_chunk_size - 1) // actual_chunk_size
        
        print(f"\nSending data in {total_chunks} chunk(s) of max {actual_chunk_size} bytes each...")
        
        for i in range(0, len(json_bytes), actual_chunk_size):
            chunk = json_bytes[i:i + actual_chunk_size]
            chunk_num = (i // actual_chunk_size) + 1
            
            await client.write_gatt_char(NUS_RX_CHAR_UUID, chunk, response=False)
            print(f"  Chunk {chunk_num}/{total_chunks}: {len(chunk)} bytes sent")
            
            # Small delay between chunks to avoid overwhelming the ESP32
            if total_chunks > 1:
                await asyncio.sleep(0.05)
        
        print(f"\n✓ Successfully sent {len(json_bytes)} bytes")
        
        # Wait a bit for ESP32 to process
        await asyncio.sleep(0.5)
        
        return True


async def main():
    parser = argparse.ArgumentParser(
        description='Send JSON roast profile to RoastABean ESP32 via BLE',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Send data from JSON file:
  python ble_send_data.py --file roast_profile.json
  
  # Send default test data:
  python ble_send_data.py
  
  # Scan for devices with custom timeout:
  python ble_send_data.py --scan-timeout 20
  
  # Custom chunk size for transmission:
  python ble_send_data.py --chunk-size 128
        """
    )
    
    parser.add_argument(
        '-f', '--file',
        help='JSON file to send (if not specified, uses default test data)',
        type=str
    )
    
    parser.add_argument(
        '-t', '--scan-timeout',
        help='BLE scan timeout in seconds (default: 10)',
        default=5.0,
        type=float
    )
    
    parser.add_argument(
        '-c', '--chunk-size',
        help='Chunk size for data transmission in bytes (default: 244)',
        default=244,
        type=int
    )
    
    args = parser.parse_args()
    
    # Load JSON data
    if args.file:
        print(f"Loading JSON from file: {args.file}")
        try:
            with open(args.file, 'r') as f:
                json_data = json.load(f)
        except Exception as e:
            print(f"✗ Error loading JSON file: {e}")
            return
    else:
        print("Using default test data")
        # Default test data from your example
        json_data = {
            "temperatures": [
                0, 171, 144, 215, 265, 284, 232, 263, 269, 285, 225, 285,
                218, 173, 285, 240, 215, 206, 236, 281, 232, 212, 285, 285,
                274, 218, 247, 285, 200, 231, 285
            ],
            "pointsQuantity": 31,
            "isScheduled": False,
            "scheduledTime": "2026-01-10T18:25:00.000Z",
            "currentTime": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z",
            "curveName": "teste novo"
        }
    
    # Scan and let user select device
    device = await scan_and_select_device(timeout=args.scan_timeout)
    
    if not device:
        print("\nMake sure:")
        print("  1. ESP32 is powered on")
        print("  2. Bluetooth is enabled on your computer")
        print("  3. ESP32 is advertising (not already connected)")
        return
    
    # Send data
    try:
        success = await send_json_data(device.address, json_data, chunk_size=args.chunk_size)
        
        if success:
            print("\n✓ Operation completed successfully!")
        else:
            print("\n✗ Operation failed!")
    
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
