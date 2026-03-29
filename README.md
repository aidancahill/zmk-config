# groguv12 ZMK Firmware

ZMK firmware for the groguv12 54-key split ergonomic keyboard with 74HC595 shift register column scanning.

## Hardware
- **MCU**: Seeeduino XIAO nRF52840 BLE
- **Matrix**: 14 columns × 5 rows (54 populated keys)
- **Columns**: 3× 74HC595 shift registers driven via SPI
- **Rows**: 5 GPIO inputs with pull-down

## Building

### Option 1: GitHub Actions (recommended)
1. Fork this repo
2. Push to `main` branch
3. GitHub Actions will build automatically
4. Download `zmk.uf2` from the Actions artifacts

### Option 2: Local build
```bash
# Install west
pip install west

# Initialize workspace
west init -l config
west update
west zephyr-export

# Build
west build -s zmk/app -b seeeduino_xiao_ble -- \
  -DSHIELD=groguv12 \
  -DZMK_CONFIG="$(pwd)/config" \
  -DZMK_EXTRA_MODULES="$(pwd)/zmk-module"
```

## Flashing
1. Double-tap the reset button on the XIAO BLE
2. A USB drive appears
3. Drag `build/zephyr/zmk.uf2` onto the drive
4. Board reboots with new firmware

## Keymap
```
Left half:                          Right half:
+---+---+---+---+---+---+          +---+---+---+---+---+---+
|Esc| 1 | 2 | 3 | 4 | 5 |          | 6 | 7 | 8 | 9 | 0 |Bks|
+---+---+---+---+---+---+          +---+---+---+---+---+---+
|Tab| Q | W | E | R | T |          | Y | U | I | O | P | \ |
+---+---+---+---+---+---+          +---+---+---+---+---+---+
| ` | A | S | D | F | G |          | H | J | K | L | ; | ' |
+---+---+---+---+---+---+          +---+---+---+---+---+---+
    | Z | X | C | V | B |          | N | M | , | . | / |
    +---+---+---+---+---+          +---+---+---+---+---+
            |Ctl|Shf|Alt|          |Alt|Shf|Ctl|
            +---+---+---+          +---+---+---+
                |Spc|                  |Ent|
                +---+                  +---+
```

## Custom Shift Register Driver
The `zmk-module/` directory contains a custom kscan driver (`kscan_sr_matrix`) that uses hardware SPI to scan the 74HC595 shift register chain. This is significantly faster than GPIO bit-banging.
