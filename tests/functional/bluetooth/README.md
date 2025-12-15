# BLE Client for RoastABean ESP32

Python script to send roast profile data to your ESP32 device via Bluetooth Low Energy (BLE).

## Installation

1. Install Python dependencies:
```bash
pip install -r requirements.txt
```

## Usage

### Send default test data
```bash
python ble_send_data.py
```

### Send data from a JSON file
```bash
python ble_send_data.py --file example_roast_profile.json
```

### Custom device name
```bash
python ble_send_data.py --device "MyCustomDevice"
```

### Adjust scan timeout
```bash
python ble_send_data.py --scan-timeout 20
```

### All options
```bash
python ble_send_data.py --help
```

## JSON Format

The JSON data should contain the following fields:

```json
{
  "temperatures": [0, 171, 144, ...],
  "pointsQuantity": 31,
  "isScheduled": true,
  "scheduledTime": "2025-11-24T21:31:00.000Z",
  "currentTime": "2025-11-23T21:30:00.496Z",
  "curveName": "teste novo"
}
```

## Features

- **Auto-discovery**: Automatically scans and finds your ESP32 device
- **Chunked transmission**: Handles large JSON payloads by splitting into MTU-sized chunks
- **Error handling**: Comprehensive error messages and status reporting
- **Progress tracking**: Shows detailed progress during data transmission
- **Flexible input**: Accept data from files or use built-in test data

## Troubleshooting

### Device not found
- Make sure ESP32 is powered on and advertising
- Check that Bluetooth is enabled on your computer
- Ensure ESP32 is not already connected to another device
- Try increasing scan timeout: `--scan-timeout 20`

### Connection fails
- Restart the ESP32
- Restart Bluetooth on your computer
- On Windows, you may need to pair the device first in Settings

### Data not received
- Check ESP32 serial monitor for error messages
- Verify the NUS UUIDs match between script and firmware
- Try reducing chunk size: `--chunk-size 128`

## Notes

- The script uses the Nordic UART Service (NUS) UUIDs that match your ESP32 firmware
- Maximum data size is 2048 bytes (as defined in your firmware `BLE_MAX_DATA_LEN`)
- Data is automatically chunked based on the negotiated MTU size
- A small delay is added between chunks to prevent overwhelming the ESP32
