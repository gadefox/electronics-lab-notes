NOTE: 8786u supports only commands [0, 1, 4]

=0: reset packet seqnum

=1: read firmware packet

=4: exec
base_addr==0
data_length==0
crc==0x188CDB1F

=5:
pcieuart8997combo pcieusb8997combo

example:
1 cmd=5 addr=0x00000000 length=12 crc=0x297480861

=6:
length==0
8887 8897 8977 8997

examples:
504 CMD6: data=0x00000000(0) crc=0x144A3610
505 DNLD: addr=0x00000000(0) length=512 crc=0x08BA4704
..
342 CMD6: data=0x00080A4D(526925) crc=0x39416A7F
343 DNLD: addr=0x00000000(0) length=512 crc=0x08BA4704
..
333 CMD6: data=0x00080CF9(527609) crc=0xAEE2B639
334 DNLD: addr=0x00000000(0) length=1024 crc=0x1A519400
..
338 CMD6: data=0x00080D2D(527661) crc=0x9AB29F76
339 DNLD: addr=0x00000000(0) length=512 crc=0x08BA4704
..
334 CMD6: data=0x00080ECD(528077) crc=0xF9A9F63F
335 DNLD: addr=0x00000000(0) length=1024 crc=0x1A519400
..
90 CMD6: data=0x00080F2D(528173) crc=0xB86231E8
91 DNLD: addr=0x00000000(0) length=512 crc=0x08BA4704

=7:
base_addr==data_length
no payload
8977_combo 8997_combo

example:
1 CMD7: data=3 crc=0x227E559B
2 DNLD: addr=0xA0080000(2684878848) length=512 crc=0xDBD6F6D3
