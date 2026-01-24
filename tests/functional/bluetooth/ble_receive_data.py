#!/usr/bin/env python3
"""
BLE Client for RoastABean ESP32 Device - Receive Feedback Test
Connects to the ESP32 and listens for feedback data sent via Nordic UART Service (NUS)
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

# Global variable to accumulate received data
received_data_buffer = bytearray()
received_messages = []


def validate_feedback_json(json_data: dict) -> tuple[bool, list[str]]:
    """
    Validate if received JSON matches the expected feedback template.
    
    Expected format:
    {
        "temperatures": [array of floats],
        "finishTime": "2026-01-15T14:30:45.000Z",
        "curveName": "string"
    }
    
    Returns:
        Tuple of (is_valid, list of error messages)
    """
    errors = []
    
    # Check required fields
    required_fields = ["temperatures", "finishTime", "curveName"]
    for field in required_fields:
        if field not in json_data:
            errors.append(f"Missing required field: '{field}'")
    
    # Validate temperatures field
    if "temperatures" in json_data:
        temps = json_data["temperatures"]
        if not isinstance(temps, list):
            errors.append("'temperatures' must be an array")
        elif len(temps) == 0:
            errors.append("'temperatures' array is empty")
        else:
            # Check if all elements are numbers
            non_numeric = [i for i, t in enumerate(temps) if not isinstance(t, (int, float))]
            if non_numeric:
                errors.append(f"'temperatures' contains non-numeric values at indices: {non_numeric[:5]}")
    
    # Validate finishTime field
    if "finishTime" in json_data:
        finish_time = json_data["finishTime"]
        if not isinstance(finish_time, str):
            errors.append("'finishTime' must be a string")
        else:
            # Try to parse as ISO 8601 format
            try:
                datetime.fromisoformat(finish_time.replace('Z', '+00:00'))
            except ValueError:
                errors.append(f"'finishTime' is not a valid ISO 8601 timestamp: '{finish_time}'")
    
    # Validate curveName field
    if "curveName" in json_data:
        curve_name = json_data["curveName"]
        if not isinstance(curve_name, str):
            errors.append("'curveName' must be a string")
        elif len(curve_name) == 0:
            errors.append("'curveName' is empty")
    
    return (len(errors) == 0, errors)


def notification_handler(sender, data: bytearray):
    """
    Callback for BLE notifications from ESP32.
    Accumulates data and tries to parse complete JSON objects.
    """
    global received_data_buffer, received_messages
    
    # Append new data to buffer
    received_data_buffer.extend(data)
    
    print(f"\n📥 Received {len(data)} bytes (total buffered: {len(received_data_buffer)} bytes)")
    print(f"   Raw data: {data.decode('utf-8', errors='replace')}")
    
    # Try to parse complete JSON objects from buffer
    try:
        buffer_str = received_data_buffer.decode('utf-8')
        
        # Count braces to detect complete JSON
        brace_count = 0
        json_start = -1
        
        for i, char in enumerate(buffer_str):
            if char == '{':
                if brace_count == 0:
                    json_start = i
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                
                # Found a complete JSON object
                if brace_count == 0 and json_start != -1:
                    json_str = buffer_str[json_start:i+1]
                    
                    try:
                        # Parse JSON
                        json_obj = json.loads(json_str)
                        
                        print(f"\n✓ Complete JSON received ({len(json_str)} bytes)")
                        print("=" * 80)
                        print(json.dumps(json_obj, indent=2))
                        print("=" * 80)
                        
                        # Validate against feedback template
                        is_valid, errors = validate_feedback_json(json_obj)
                        
                        if is_valid:
                            print("\n✅ VALIDATION PASSED - JSON matches feedback template!")
                            
                            # Print summary
                            print(f"\n📊 Feedback Summary:")
                            print(f"   Curve Name: {json_obj.get('curveName', 'N/A')}")
                            print(f"   Finish Time: {json_obj.get('finishTime', 'N/A')}")
                            print(f"   Temperature Points: {len(json_obj.get('temperatures', []))}")
                            
                            # Print temperature statistics if available
                            if 'temperatures' in json_obj and len(json_obj['temperatures']) > 0:
                                temps = json_obj['temperatures']
                                print(f"   Min Temp: {min(temps):.1f}°C")
                                print(f"   Max Temp: {max(temps):.1f}°C")
                                print(f"   Avg Temp: {sum(temps)/len(temps):.1f}°C")
                        else:
                            print("\n❌ VALIDATION FAILED - JSON does not match feedback template!")
                            for error in errors:
                                print(f"   ✗ {error}")
                        
                        # Store message
                        received_messages.append({
                            'timestamp': datetime.now().isoformat(),
                            'data': json_obj,
                            'valid': is_valid,
                            'errors': errors
                        })
                        
                        # Remove processed JSON from buffer
                        received_data_buffer = bytearray(buffer_str[i+1:].encode('utf-8'))
                        
                    except json.JSONDecodeError as e:
                        print(f"\n⚠ JSON parse error: {e}")
                        # Don't clear buffer, might need more data
                    
                    json_start = -1
        
    except UnicodeDecodeError as e:
        print(f"⚠ Unicode decode error: {e}")


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
            print("Quitting...")
            return None
        
        index = int(user_input)
        
        if 0 <= index < len(named_devices):
            selected = named_devices[index]
            print(f"\n✓ Selected: {selected.name} ({selected.address})")
            return selected
        else:
            print(f"✗ Invalid index. Please choose 0-{len(named_devices)-1}")
            return None
            
    except ValueError:
        print("✗ Invalid input. Please enter a number or 'q'")
        return None
    except KeyboardInterrupt:
        print("\nCancelled by user")
        return None


async def listen_for_feedback(address: str, duration: int = 30):
    """
    Connect to BLE device and listen for feedback data.
    
    Args:
        address: BLE device address
        duration: How long to listen for data in seconds (default 30)
    """
    global received_data_buffer, received_messages
    
    # Reset buffers
    received_data_buffer = bytearray()
    received_messages = []
    
    print(f"\nConnecting to {address}...")
    
    async with BleakClient(address) as client:
        print(f"✓ Connected to {address}")
        
        # Check if services are available
        services = client.services
        nus_service = services.get_service(NUS_SERVICE_UUID)
        
        if not nus_service:
            print(f"✗ Nordic UART Service not found!")
            return False
        
        print("✓ Nordic UART Service found")
        
        # Get MTU size
        mtu = client.mtu_size
        print(f"✓ MTU size: {mtu} bytes")
        
        # Subscribe to notifications from TX characteristic
        print(f"\n📡 Subscribing to notifications from ESP32...")
        await client.start_notify(NUS_TX_CHAR_UUID, notification_handler)
        print(f"✓ Subscribed to notifications")
        
        print(f"\n🎧 Listening for feedback data (timeout: {duration}s)...")
        print("   Press Ctrl+C to stop early\n")
        print("-" * 80)
        
        # Listen for the specified duration
        try:
            await asyncio.sleep(duration)
        except KeyboardInterrupt:
            print("\n\n⚠ Interrupted by user")
        
        # Unsubscribe from notifications
        await client.stop_notify(NUS_TX_CHAR_UUID)
        print(f"\n✓ Unsubscribed from notifications")
        
        # Print summary
        print("\n" + "=" * 80)
        print("📊 SESSION SUMMARY")
        print("=" * 80)
        print(f"Total messages received: {len(received_messages)}")
        print(f"Buffered data remaining: {len(received_data_buffer)} bytes")
        
        if received_messages:
            valid_count = sum(1 for msg in received_messages if msg['valid'])
            print(f"Valid feedback messages: {valid_count}")
            print(f"Invalid feedback messages: {len(received_messages) - valid_count}")
            
            print("\n📝 Message Details:")
            for i, msg in enumerate(received_messages, 1):
                status = "✅ VALID" if msg['valid'] else "❌ INVALID"
                print(f"\n  Message {i}: {status}")
                print(f"    Timestamp: {msg['timestamp']}")
                if 'curveName' in msg['data']:
                    print(f"    Curve Name: {msg['data']['curveName']}")
                if not msg['valid']:
                    print(f"    Errors: {', '.join(msg['errors'])}")
        else:
            print("\n⚠ No complete messages received")
            if len(received_data_buffer) > 0:
                print(f"   Partial data in buffer: {received_data_buffer.decode('utf-8', errors='replace')}")
        
        return len(received_messages) > 0


async def main():
    parser = argparse.ArgumentParser(
        description='Listen for feedback data from RoastABean ESP32 via BLE',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Listen for 30 seconds (default):
  python ble_receive_data.py
  
  # Listen for 60 seconds:
  python ble_receive_data.py --duration 60
  
  # Scan for devices with custom timeout:
  python ble_receive_data.py --scan-timeout 20
        """
    )
    
    parser.add_argument(
        '-t', '--scan-timeout',
        help='BLE scan timeout in seconds (default: 5)',
        default=5.0,
        type=float
    )
    
    parser.add_argument(
        '-d', '--duration',
        help='How long to listen for data in seconds (default: 30)',
        default=30,
        type=int
    )
    
    args = parser.parse_args()
    
    # Scan and let user select device
    device = await scan_and_select_device(timeout=args.scan_timeout)
    
    if not device:
        print("\n✗ No device selected")
        return 1
    
    # Listen for feedback data
    try:
        success = await listen_for_feedback(device.address, duration=args.duration)
        
        if success:
            print("\n✅ Session completed successfully")
            return 0
        else:
            print("\n⚠ No data received")
            return 1
    
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        exit(exit_code)
    except KeyboardInterrupt:
        print("\n\n⚠ Cancelled by user")
        exit(130)
