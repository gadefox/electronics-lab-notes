# Technical Reference

Build instructions and serial protocol specification for sigrok-pico developers.

---

## Table of Contents

1. [Building the Firmware](#building-the-firmware)
2. [Building libsigrok](#building-libsigrok)
3. [Serial Protocol](#serial-protocol)
4. [Coding Guidelines](#coding-guidelines)

---

## Coding Guidelines

This project uses VSCode's built-in LLVM-based formatter for C code consistency.

### Setup

1. Open VSCode settings (`Ctrl+,`)
2. Enable "Format on Save" (`editor.formatOnSave`)
3. Set `C_Cpp.clang_format_style` to `LLVM`

### Configuration

The project uses default LLVM style formatting. Key conventions:

- Indent width: 2 spaces
- Column limit: 80 characters

> Run "Format Document" (`Shift+Alt+F`) before committing to ensure consistent code style.

## Building the Firmware

### Prerequisites

Install the "Raspberry Pi Pico Project" extension in VSCode.

### Build Steps (VSCode Extension)

1. Import the project using the Raspberry Pi Pico extension
2. Select the latest SDK version when prompted
3. Select the board type from existing options (e.g., `pico` or `pico2`)
4. Click "Run Project (USB)" to build and flash

### Build Steps (Command Line)

Alternatively, build from the command line:

```bash
# 1. Clone the repository
git clone https://github.com/pico-coder/sigrok-pico.git
cd sigrok-pico

# 2. Copy the SDK import file
cp <pico-sdk-path>/pico_sdk_import.cmake .

# 3. Set SDK path
export PICO_SDK_PATH=<path-to-pico-sdk>

# 4. Build
mkdir build && cd build
cmake ..
make
```

Output: `pico_sdk_sigrok.uf2` in the build directory.

---

## Building libsigrok

The raspberrypi_pico driver is merged into mainline libsigrok. Install from [sigrok.org](https://sigrok.org/wiki/Downloads) rather than building from source.

### If Building from Source

1. Follow official libsigrok build instructions
2. Resolve library dependencies (most challenging part)
3. Build sigrok-cli first to verify USB/serial libraries

> Resolving library dependencies is the greatest challenge. This is intrinsic to building PulseView, not specific to the PICO driver.

---

## Serial Protocol

### Protocol Overview

Four transfer flows:
1. Configuration (sample rates, channels, triggers)
2. General Data (analog or >4 digital channels)
3. Optimized Data (≤4 digital channels with RLE)
4. Final Count (byte count at transfer end)

### Control Commands

#### Immediate Commands (No Response)

| Command | Description                                      |
|---------|--------------------------------------------------|
| `*`     | Reset - terminate sampling, clear state          |
| `+`     | Abort - host-forced stop (PulseView stop button) |

#### Commands Requiring Response

| Command | Response           | Description                            |
|---------|--------------------|----------------------------------------|
| `i`     | `SRPICO,AxxDyy,00` | Identify - `Axx`=analog, `Dyy`=digital |
| `aX`    | `aaaaxbbbbb`       | Analog scale/offset for channel X      |

#### Commands with ACK

Device returns `*` on success, nothing on error.

| Command | Format      | Description                          |
|---------|-------------|--------------------------------------|
| `R`     | `R<rate>`   | Sample rate (e.g., `R100000`)        |
| `L`     | `L<count>`  | Sample limit (e.g., `L5000`)         |
| `A`     | `A<en><ch>` | Analog channel: `A103` enables A3    |
| `D`     | `D<en><ch>` | Digital channel: `D020` disables D20 |

#### Commands Without Response

| Command | Description                          |
|---------|--------------------------------------|
| `F`     | Fixed Sample mode (no SW triggering) |
| `C`     | Continuous mode (for SW triggering)  |

### Device-to-Host Commands

| Command | Description                             |
|---------|-----------------------------------------|
| `!`     | Abort signal - device detected overflow |

### Data Transfer Protocols

#### General Protocol

Used when: analog channels enabled OR >4 digital channels.

**Encoding**:
- Each byte OR'd with `0x80`
- 7 channels per byte (digital)
- 7-bit sample value per channel (analog)

**Example**: 14 digital + 2 analog channels:
```
0x8F, 0xA3, 0x91, 0xB6
```
- `0x8F`: D8:D2 = 0x0F
- `0xA3`: D15:D9 = 0x23
- `0x91`: A0 = 0x11
- `0xB6`: A1 = 0x36

#### Optimized Protocol (RLE)

Used for ≤4 digital channels with no analog.

**Purpose**: Enable high-speed sampling of narrow protocols (I2C, I2S, SPI) by reducing wire bandwidth.

**Encoding**: Each byte contains 4-bit sample value + RLE count (0-7 inline, or 8-640 extended).

---

## Architecture Notes

### Clock Sources

| Subsystem | Clock              | Max Rate |
|-----------|--------------------|----------|
| PIO       | sysclk (120 MHz)   | 120 Msps |
| ADC       | USB clock (48 MHz) | 500 Ksps |

- PIO and ADC share one sample rate (libsigrok limitation)
- ADC: Integer divisor of 48 MHz (worst case: 5 KHz granularity)
- PIO: Full fractional divisor of 120 MHz sysclk

### DMA

For 8+ digital channels at high rates, DMA requires read-modify-write operations. Limit reliable operation to ≤60 Msps.

### Revision Differences

| Feature       | Rev1       | Rev2       |
|---------------|------------|------------|
| Debug UART    | 115200 bps | 921600 bps |
| HW Triggering | Supported  | Removed    |
