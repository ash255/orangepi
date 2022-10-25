#ifndef __SIM_H
#define __SIM_H

#define CLA_2G 0x80

#define CLA_3G_NORMAL  0x00
#define CLA_3G_SPECIAL 0X80

#define INS_SELECT              	0xA4
#define INS_STATUS                  0xF2
#define INS_READ_BINARY             0xB0
#define INS_UPDATE_BINARY           0xD6
#define INS_READ_RECORD             0xB2
#define INS_UPDATE_RECORD           0xDC
#define INS_SEARCH_RECORD           0xA2
#define INS_INCREASE                0x32
#define INS_RETRIEVE_DATA           0xCB
#define INS_SET_DATA                0xDB
#define INS_VERIFY                  0x20
#define INS_CHANGE_PIN              0x24
#define INS_DISABLE_PIN             0x26
#define INS_ENABLE_PIN              0x28
#define INS_UNBLOCK_PIN             0x2C
#define INS_DEACTIVATE_FILE         0x04
#define INS_ACTIVATE_FILE           0x44
#define INS_AUTHENTICATE            0x88	//0x89
#define INS_GET_CHALLENGE           0x84
#define INS_TERMINAL_CAPABILITY     0xAA
#define INS_TERMINAL_PROFILE        0x10
#define INS_ENVELOPE                0xC2
#define INS_FETCH                   0x12
#define INS_TERMINAL_RESPONSE       0x14
#define INS_MANAGE_CHANNEL          0x70
#define INS_MANAGE_SECURE_CHANNEL   0x73
#define INS_TRANSACT_DATA           0x75
#define INS_GET_RESPONSE            0xC0

struct sim_resp_t
{
	uint8_t status;
   	uint8_t sw1;
	uint8_t sw2;
    uint8_t resp[256];
	uint32_t len;
};

#endif