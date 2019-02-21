// depends on stdint.h

#ifdef OLT_INTERN_READING_H
#error multiple inclusion
#endif
#define OLT_INTERN_READING_H

typedef uint8_t  BYTES1;
typedef uint16_t BYTES2;
typedef uint32_t BYTES4;
typedef uint64_t BYTES8;

static inline uint16_t ru16(BYTES2 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint16_t b0 = bytes[1], b1 = bytes[0];
	return b0 | b1 << 8;
}

static inline int16_t ri16(BYTES2 raw)
{
	uint16_t u = ru16(raw);
	return *(int16_t *) &u;
}

static inline uint32_t ru32(BYTES4 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint32_t b0 = bytes[3], b1 = bytes[2];
	uint32_t b2 = bytes[1], b3 = bytes[0];
	return b0 | b1 << 8 | b2 << 16 | b3 << 24;
}
