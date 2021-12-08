#ifndef CRC32_H
#define CRC32_H

#include<stdint.h>
#include <cstddef>

static uint32_t crc32_table[256];

void generate_crc32_table();

uint32_t update_crc32(const uint8_t *buf, size_t len);

#endif //CRC32_H