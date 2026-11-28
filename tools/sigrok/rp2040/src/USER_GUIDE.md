# User Guide

Complete guide for using the sigrok-pico logic analyzer and oscilloscope.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Channel Configuration](#channel-configuration)
3. [Trigger Modes](#trigger-modes)
4. [Storage Modes](#storage-modes)
5. [Sample Rates](#sample-rates)
6. [Best Practices](#best-practices)

---

## Getting Started

### Prerequisites

- Raspberry Pi PICO (RP2040)
- USB cable (data-capable)
- ≥1 kΩ resistors for input protection (recommended)

> **Warning**: Always use current-limiting resistors between signal sources and PICO inputs. Voltages outside 0V-3.3V can damage the device.

### Installation

1. **Flash firmware**: Copy a UF2 file from [`pico_sdk_sigrok/release/`](pico_sdk_sigrok/release/) to your PICO (hold BOOTSEL while connecting USB)
2. **Install software**: Install PulseView or sigrok-cli from [sigrok.org](https://sigrok.org/wiki/Downloads)
3. **Connect**: Replug the PICO to reset it

### First Capture

**Using sigrok-cli:**
```bash
# List available serial ports
sigrok-cli --list-serial

# Scan for device
sigrok-cli -l 2 -d raspberrypi-pico:conn=/dev/ttyACM0:serialcomm=115200/flow=0 --scan

# Capture
sigrok-cli -l 2 -d raspberrypi-pico:conn=/dev/ttyACM0:serialcomm=115200/flow=0 \
  --config samplerate=10000 --channels D2,D3,D4,D5 --samples 1000
```

**Using PulseView:**
1. Launch PulseView
2. Select "raspberrypi_pico" driver
3. Set serial port (e.g., `/dev/ttyACM0` or `COM3`)
4. Configure sample rate and channels
5. Click "Run"

### Windows Troubleshooting

1. Close other serial port applications
2. Reboot after installing PulseView
3. Try using Zadig to install USB driver if needed
4. Cycle USB connection
5. Test with a terminal: send `*` then `i` - should respond with `SRPICO,A03D21,00`

> Run with debug level 2 (`-l 2`) to see configuration issues.

---

## Channel Configuration

### Digital Channels (21)

| Channel | Pin |
|---------|-----|
| D2-D22 | Board pins D2-D22 |

**Rule**: Channels must be enabled contiguously starting from D2.

### Analog Channels (3)

| Channel | ADC | Pin |
|---------|-----|-----|
| A0 | ADC0 | 31 |
| A1 | ADC1 | 32 |
| A2 | ADC2 | 34 |

**Accuracy**: 7-bit (128 divisions, ~20mV resolution)

> 7-bit encoding avoids problematic ASCII characters in serial transfer. RP2040 ADC ENOB is ~8 bits despite 12-bit output.

### Optimization

Disable unused channels to reduce serial transfer overhead and allocate more trace storage.

---

## Trigger Modes

### Software Trigger (Default)

- Any enabled digital pin can be a trigger source
- Analog captured synchronously with digital triggers
- Types: level, rising, falling, changing
- Pre-capture ratio: 0-100%

> Software triggering adds host-side processing overhead. On slower processors, this may limit maximum streaming sample rates.

### Always Trigger

When no trigger specified, device immediately captures a fixed-length trace.

### Hardware Trigger (Removed in Rev2)

Hardware triggering was removed because:
- The implementation's processing overhead reduced streaming rates below software trigger performance
- No clear UI method existed to distinguish between hardware and software triggering

---

## Storage Modes

### Fixed Depth Mode

**When enabled**: No SW trigger AND sample count fits in device storage

**Advantages**:
- Guaranteed capture completion
- No data loss regardless of USB bandwidth

### Continuous Streaming Mode

**When enabled**: SW triggering active OR sample count exceeds internal storage

**Characteristics**:
- Data streams to host during capture
- May overflow if USB bandwidth insufficient
- Device detects overflow and sends abort code

**Trade-off**: Larger capture depth vs. risk of data loss.

---

## Sample Rates

Select the number of enabled digital and analog channels along with the number of samples to determine the maximum sample rate and its limiting factor.

### Configuration Table

| Digital Channels | Analog Channels | Number of Samples | Max Sample Rate | Limiting Factor |
|------------------|-----------------|-------------------|-----------------|-----------------|
| 1-4              | 0               | ≤400K             | 120 Msps        | PIO             |
| 1-4              | 0               | >400K             | 500 Ksps+RLE    | USB w/ RLE      |
| 5-7              | 0               | ≤200K             | 120 Msps        | PIO             |
| 5-7              | 0               | >200K             | 500 Ksps+RLE    | USB w/ RLE      |
| 8-14             | 0               | ≤100K             | 120 Msps        | PIO             |
| 8-14             | 0               | >100K             | 250 Ksps+RLE    | USB w/ RLE      |
| 15-21            | 0               | ≤50K              | 120 Msps        | PIO             |
| 15-21            | 0               | >50K              | 167 Ksps+RLE    | USB w/ RLE      |
| 0                | 1               | ≤200K             | 500 Ksps        | ADC             |
| 0                | 1               | >200K             | 500 Ksps        | USB & ADC       |
| 0                | 2               | ≤100K             | 250 Ksps        | ADC             |
| 0                | 2               | >100K             | 250 Ksps        | USB & ADC       |
| 0                | 3               | ≤67K              | 160 Ksps        | ADC             |
| 0                | 3               | >67K              | 160 Ksps        | USB & ADC       |
| 1-7              | 1               | ≤100K             | 500 Ksps        | ADC             |
| 1-7              | 1               | >100K             | 250 Ksps        | USB             |
| 1-7              | 2               | ≤67K              | 250 Ksps        | ADC             |
| 1-7              | 2               | >67K              | 160 Ksps        | ADC & USB       |
| 1-7              | 3               | ≤50K              | 160 Ksps        | ADC             |
| 1-7              | 3               | >50K              | 125 Ksps        | USB & ADC       |
| 8-14             | 1               | ≤67K              | 500 Ksps        | ADC             |
| 8-14             | 1               | >67K              | 160 Ksps        | USB             |
| 8-14             | 2               | ≤50K              | 250 Ksps        | ADC             |
| 8-14             | 2               | >50K              | 125 Ksps        | USB             |
| 8-14             | 3               | ≤40K              | 160 Ksps        | ADC             |
| 8-14             | 3               | >40K              | 100 Ksps        | USB             |

### Limiting Factors

| Factor | Limit          | Description                                      |
|--------|----------------|--------------------------------------------------|
| PIO    | 120 MHz        | Maximum system clock for programmable I/O        |
| ADC    | 500 Ksps       | Shared across all enabled analog channels        |
| USB    | 400-800 KB/sec | Varies by host; affects USB-limited sample rates |

### RLE Optimization

Run Length Encoding reduces bandwidth for sparse signals:

```
Effective rate = listed max × (1 / activity_factor)
```

Example: 25% activity factor with 1-4 digital channels can support ~2 Msps. The RLE algorithm for 1-4 digital channels is more wire-efficient than for 5-21 channels.

### Hard Limits

- **Common rate**: PIO and ADC share one sample rate
- **Range**: 5 KHz minimum, 120 Msps maximum (digital only)
- **Recommendation**: ≤60 Msps for 8+ digital channels

---

## Best Practices

1. **Use input protection**: ≥1 kΩ resistors inline with signals
2. **Start low**: Begin with lower sample rates, increase as needed
3. **Leverage RLE**: For digital-only sparse signals
4. **Disable unused channels**: Improves performance
5. **Check debug output**: Use `-l 2` for diagnostics

### Debug UART

UART0 TX outputs debug information:
- Rev1: 115200 bps
- Rev2+: 921600 bps

Not required for normal operation, but useful for bug reports.
