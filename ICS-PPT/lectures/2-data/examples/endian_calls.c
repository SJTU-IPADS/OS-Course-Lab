/* The two conversions on their own, so the generated code is readable. */
#include <endian.h>
#include <stdint.h>

uint32_t to_be(uint32_t x) { return htobe32(x); }   /* the other order */
uint32_t to_le(uint32_t x) { return htole32(x); }   /* this machine's  */
