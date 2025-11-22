#ifndef MARVELL_CRC_H
#define MARVELL_CRC_H

#include <stdint.h>

/*
 * NOTE:
 * polynomial 04C11DB7
 * initial remainder 0
 * no reflections
 * no inversion at the end
 * stored in big-endian format!
 */

/* HACK:
 * The CRC of the data with following CRC will be zero.
 */

void crc_init(void);
uint32_t crc_calc(uint32_t remainder, const uint8_t *data, uint32_t len);

#endif /* MARVELL_CRC_H */
