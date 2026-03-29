# groguv12 ZMK Firmware

ZMK firmware for the groguv12 54-key split ergonomic keyboard with 74HC595 shift register column scanning.

## Hardware

- **MCU**: Seeeduino XIAO nRF52840 BLE
- **Matrix**: 14 columns × 5 rows (54 populated keys)
- **Columns**: 3× daisy-chained 74HC595 shift registers via SPI
- **Rows**: 5 GPIO inputs with pull-down
- **Switches**: Kailh Choc V1 (PG1350) with hotswap sockets

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
  -DSHIELD=groguv12 \
  -DZMK_CONFIG="$(pwd)/config"
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
            |   |   |   |              |   |   |Fn |
            +---+---+---+              +---+---+---+
```
