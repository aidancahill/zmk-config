# custom-keyboard ZMK Firmware

ZMK firmware for the custom-keyboard 54-key split ergonomic keyboard with 74HC595 shift register column scanning, 74HC165 row reading, and analog joystick pointing devices.

## Hardware

- **MCU**: Seeeduino XIAO nRF52840 BLE
- **Matrix**: 14 columns × 5 rows (54 populated keys)
- **Columns**: 3× daisy-chained 74HC595 shift registers (SIPO) via SPI
- **Rows**: 1× 74HC165 shift register (PISO) via SPI
- **Joysticks**: 2× COM-09032 analog joysticks
- **Switches**: Kailh Choc V1 (PG1350) with hotswap sockets

## Pin Mapping

| XIAO Pin | Function |
|----------|----------|
| D0 (A0)  | Left Joystick Vertical (ADC) |
| D1 (A1)  | Left Joystick Horizontal (ADC) |
| D2 (A2)  | Right Joystick Vertical (ADC) |
| D3 (A3)  | Right Joystick Horizontal (ADC) |
| D4       | Left Joystick Button |
| D5       | Right Joystick Button |
| D6       | 74HC165 PL (parallel load) |
| D7       | 74HC595 RCLK (latch) |
| D8       | SCK (shared SPI clock) |
| D9       | MISO (74HC165 serial data) |
| D10      | MOSI (74HC595 serial data) |

## Joystick Controls

| Joystick | Axis | Function |
|----------|------|----------|
| Right    | Move | Mouse cursor movement |
| Right    | Click | Left mouse click |
| Left     | Move | Scroll (up/down/left/right) |
| Left     | Click | Right mouse click |

## Building

### GitHub Actions (recommended)

1. Fork this repo
2. Push to `main` branch
3. GitHub Actions will build automatically
4. Download `zmk.uf2` from the Actions artifacts

### Local build

```bash
pip install west
west init -l config
west update
west zephyr-export
west build -s zmk/app -b xiao_ble//zmk -- \
  -DSHIELD=custom-keyboard \
  -DZMK_CONFIG="$(pwd)/config" \
  -DZMK_EXTRA_MODULES="$(pwd)/zmk-module"
```

## Flashing

1. Double-tap the reset button on the XIAO BLE
2. A USB drive appears
3. Drag `zmk.uf2` onto the drive
4. Board reboots with new firmware

## Keymap

### Layer 0: Default

```
Left half:                              Right half:
+---+---+---+---+---+---+              +---+---+---+---+---+---+
|Esc| 1 | 2 | 3 | 4 | 5 |              | 6 | 7 | 8 | 9 | 0 |Bks|
+---+---+---+---+---+---+              +---+---+---+---+---+---+
|Tab| Q | W | E | R | T |              | Y | U | I | O | P | \ |
+---+---+---+---+---+---+---+      +---+---+---+---+---+---+---+
| ` | A | S | D | F | G |Spc|      |Ent| H | J | K | L | ; | ' |
+---+---+---+---+---+---+---+      +---+---+---+---+---+---+---+
    | Z | X | C | V | B |              | N | M | , | . | / |
    +---+---+---+---+---+              +---+---+---+---+---+
            |Ctl|Shf|Alt|              |Alt|Shf|Fn |
            +---+---+---+              +---+---+---+
```

### Layer 1: Function (hold Fn)

```
Left half:                              Right half:
+---+---+---+---+---+---+              +---+---+---+---+---+---+
|F1 |F2 |F3 |F4 |F5 |F6 |              |F7 |F8 |F9 |F10|F11|F12|
+---+---+---+---+---+---+              +---+---+---+---+---+---+
| ~ | ! | @ | # | $ | % |              | ^ | & | * | ( | ) |Del|
+---+---+---+---+---+---+---+      +---+---+---+---+---+---+---+
|   | - | = | [ | ] | \ |   |      |   | ↓ | ← | ↑ | → |   |   |
+---+---+---+---+---+---+---+      +---+---+---+---+---+---+---+
    | _ | + | { | } | | |              |Hom|PgD|PgU|End|   |
    +---+---+---+---+---+              +---+---+---+---+---+
            |   |   |   |              |   |   |[Fn]|
            +---+---+---+              +---+---+---+
```

## Custom ZMK Module

The `zmk-module/` directory contains two custom drivers:

### 74HC165 GPIO Input Driver (`zmk,gpio-165`)

Parallel-in, serial-out shift register driver for reading matrix rows via SPI. Mirrors ZMK's built-in `zmk,gpio-595` but for input instead of output.

### Analog Joystick Input Driver (`zmk,input-analog-joystick`)

Reads two ADC channels for X/Y axes and an optional button GPIO. Supports two modes:

- **Mouse mode**: Generates `INPUT_REL_X` / `INPUT_REL_Y` events for cursor movement
- **Scroll mode**: Generates `INPUT_REL_WHEEL` / `INPUT_REL_HWHEEL` events for scrolling

## BOM

| Qty | Component | Package |
|-----|-----------|---------|
| 54  | Kailh Choc V1 switches | PG1350 |
| 54  | Kailh Choc hotswap sockets | |
| 54  | 1N4148W diodes | SOD-123 |
| 54  | MBK keycaps | 1u Choc |
| 3   | 74HC595 shift registers | SOIC-16 |
| 1   | 74HC165 shift register | SOIC-16 |
| 1   | Seeeduino XIAO nRF52840 BLE | |
| 2   | COM-09032 joysticks | |
| 2   | 7-pin female headers (2.54mm) | |
| 8   | M2 heat-set inserts + screws | |
