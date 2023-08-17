#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

#include "ssd1306.h"
#include <stdio.h>
#include <main.h>
#include <stdint.h>
#include "string.h"
#define FINGERPRINT_OK 0x00
#define FINGERPRINT_DELETE 0x0C
#define FINGERPRINT_BADPACKET 0xFE
#define FINGERPRINT_VERIFYPASSWORD 0x13
#define FINGERPRINT_COMMANDPACKET 0x01
#define FINGERPRINT_TIMEOUT 0xFF
#define FINGERPRINT_NOTFOUND 0x09
#define FINGERPRINT_SEARCH 0x04
#define FINGERPRINT_STORE 0x06
#define FINGERPRINT_FLASHERR 0x18
#define FINGERPRINT_BADLOCATION 0x0B
#define FINGERPRINT_EMPTY 0x0D
#define FINGERPRINT_PACKETRECIEVEERR 0x01
#define FINGERPRINT_NOFINGER 0x02
#define FINGERPRINT_ENROLLMISMATCH 0x0A
#define FINGERPRINT_IMAGEFAIL 0x03
#define FINGERPRINT_IMAGE2TZ 0x02
#define FINGERPRINT_IMAGEMESS 0x06
#define FINGERPRINT_INVALIDIMAGE 0x15
#define FINGERPRINT_GETIMAGE 0x01
#define FINGERPRINT_REGMODEL 0x05
#define FINGERPRINT_FEATUREFAIL 0x07
#define DEFAULTTIMEOUT 500
#define FINGERPRINT_STARTCODE 0xEF01 //!< Fixed falue of EF01H; High byte transferred first

void write_command(uint8_t *command, uint16_t size);
uint8_t deleteFingerprint(uint8_t id);
uint8_t checkPassword(void);
uint8_t checkFingerPrintID();
uint8_t fingerSearch(uint8_t slot);
uint8_t fingerIDSearch(uint8_t slot,uint16_t* FingerID);
uint8_t getFingerprintID(uint16_t* FingerID);
uint8_t checkInputFinger(uint8_t f);
uint8_t getFingerprintEnroll(uint8_t id);
uint8_t emptyDatabase(void);
int find_avaiable_idex_table();
void show_idextable();
uint8_t getImage();
uint8_t image2Tz(uint8_t slot);
uint8_t emptyfinger_database(void);