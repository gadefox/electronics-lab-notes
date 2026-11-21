# pgen

Simple test pattern generator for the Raspberry Pi Pico. Outputs a free-running binary counter on GPIO2–GPIO22, useful for testing logic analyzers (e.g. sigrok) and checking channel order, sample rate and RLE compression.

## Behavior

- GPIO2–GPIO22 (21 pins, mask `0x7FFFFC`) are outputs.
- GPIO2 is the counter LSB, GPIO22 the MSB.
- The counter is derived from the 1 µs hardware timer, shifted right by 5 bits (increments every 32 µs).
- GPIO0/GPIO1 are used as UART (115200 baud) and print a hello message at startup.

## Output frequencies (default `>> 5`)

| Pin    | Frequency  |
|--------|------------|
| GPIO2  | ~15.6 kHz  |
| GPIO3  | ~7.8 kHz   |
| GPIO4  | ~3.9 kHz   |
| ...    | halves with each pin |
| GPIO22 | ~15 µHz (period ~67 s) |

To change the speed, edit the shift in the `cval = (ctime - tstart) >> 5;` line. Several alternatives are included as comments. Note that left shifts (`<<`) make the counter jump in steps larger than 1, so the low bits stay constant.
