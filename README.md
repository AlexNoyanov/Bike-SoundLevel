# Bike Sound Level

ESP32-C3 Super Mini firmware that pairs with an iPhone over BLE and acts as a media remote using a KY-040 rotary encoder.

| Action | Result |
|--------|--------|
| Rotate clockwise | Volume up |
| Rotate counter-clockwise | Volume down |
| Press encoder knob | Play / pause |

## Wiring

| KY-040 | ESP32-C3 |
|--------|----------|
| SW | GPIO 5 |
| DT | GPIO 6 |
| CLK | GPIO 7 |
| + | 3.3 V |
| GND | GND |

The KY-040 module includes pull-up resistors; no extra components are required.

## Build & flash

Requires [PlatformIO](https://platformio.org/).

```bash
pio run -t upload
pio device monitor
```

Connect the board over USB, put it in bootloader mode if upload fails (hold **BOOT**, tap **RESET**, release **BOOT**).

## Pair with iPhone

1. Flash the firmware and power the board.
2. On iPhone: **Settings → Bluetooth**.
3. Tap **Bike Volume** to pair.
4. Start music in Apple Music, Spotify, or any app that responds to system media keys.
5. Rotate the knob for volume; press for play/pause.

If pairing fails, forget the device on the iPhone, power-cycle the ESP32, and try again.

## Notes

- Uses [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) with NimBLE. A build script patches the library for reliable ESP32-C3 + iOS pairing.
- Volume changes are limited to one step every 80 ms so rapid spins do not flood BLE.
- Some apps ignore system volume keys; test with Apple Music or the lock-screen player first.
