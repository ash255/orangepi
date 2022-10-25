#ifndef __TLV_H
#define __TLV_H
#include <stdint.h>

#define TAG_INVAILD 0xFF

struct tlv_t
{
	uint8_t tag;
	uint8_t len;
	uint8_t *value;
	struct tlv_t *next;
};

struct tlv_t* tlv_parser(uint8_t *data, int len);
struct tlv_t* tlv_get(struct tlv_t *tlv, uint8_t tag);
void tlv_free(struct tlv_t* tlv);

#endif