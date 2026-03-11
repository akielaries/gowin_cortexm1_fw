/*
 * ipv4.c — Layer 3: IPv4 helpers
 */

#include "ipv4.h"

uint16_t inet_checksum(const uint8_t *data, int len) {
  uint32_t sum = 0;
  for (int i = 0; i + 1 < len; i += 2)
    sum += ((uint32_t)data[i] << 8) | data[i + 1];
  if (len & 1)
    sum += (uint32_t)data[len - 1] << 8;
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);
  return (uint16_t)(~sum);
}
