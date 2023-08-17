#include <as608.h>
#include "mfrc522.h"
extern UART_HandleTypeDef huart1;

uint8_t tx_flag = 0;
uint8_t test_count = 0;
uint8_t finger_data[64];
uint8_t rx_finger_data = 0;
uint8_t rx_buffer[64];
uint16_t idex = 0;
uint16_t length = 0;

void write_command(uint8_t *command, uint16_t size)
{
    SSD1306_Clear();
    uint8_t test[size + 1];
    sprintf(test, "%s", command);
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts(test, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

void write_command_at(uint8_t *command, uint16_t size, int x, int y)
{
    SSD1306_Clear();
    uint8_t test[size + 1];
    sprintf(test, "%s", command);
    SSD1306_GotoXY(x, y);
    SSD1306_Puts(test, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (idex == 8)
	{
		if (huart->Instance == huart1.Instance)
		{
			length = (uint16_t)(rx_buffer[7] << 8) + (uint16_t)(rx_buffer[8] & 0xFF);
			idex++;
			HAL_UART_Receive_IT(&huart1, &rx_buffer[idex], 1);
		}
	}
	else if (idex > 8)
	{
		if (length > idex - 8)
		{
			if (huart->Instance == huart1.Instance)
			{
					idex++;
					HAL_UART_Receive_IT(&huart1, &rx_buffer[idex], 1);
			}
		}
		else
		{
				tx_flag = 1;
		}
	}
	else if (idex < 8)
	{
		if (huart->Instance == huart1.Instance)
		{
				idex++;
				HAL_UART_Receive_IT(&huart1, &rx_buffer[idex], 1);
		}
	}
}

void write_byte(uint8_t transfer_finger_data)
{
    HAL_UART_Transmit(&huart1, &transfer_finger_data, 1, 1000);
}
void writeStructuredPacket(uint8_t *buffer_finger_data, uint8_t Size)
{
    /* Gui lan luot 8 msb bit va 8 lsb bit cua start code: 0xEF01 (packet header) */
    write_byte((uint8_t)(0xEF01 >> 8));   // gui 1 byte 0xEF
    write_byte((uint8_t)(0xEF01 & 0xFF)); // gui 1 byte 0x01
    /* truyen 32 bit dia chi cua cam bien */
    write_byte(0xFF);
    write_byte(0xFF);
    write_byte(0xFF);
    write_byte(0xFF);
    /* truyen type of packet (identifier) */
    write_byte(0x01);
    /* truyen 16 bit do dai cua packet */
    uint16_t wire_length = Size + 2;
    write_byte((uint8_t)(wire_length >> 8));
    write_byte((uint8_t)(wire_length & 0xFF));
    /* bien sum su dung de tinh 2 byte checksum */
    uint16_t sum = ((wire_length) >> 8) + ((wire_length)&0xFF) + 0x01;
    /* tinh checksum voi truong command code va password */
    for (uint8_t i = 0; i < Size; i++)
    {
        write_byte(buffer_finger_data[i]); // lan luot gui tung byte finger_data, byte
        sum += buffer_finger_data[i];      // cong cac byte finger_data de tinh gia tri checksum
    }
    /* vong lap tren lan luot gui du lieu trong buffer finger_data theo tung byte va gui gia tri checksum */
    /* Gui 2 byte checksum sau khi da tinh */
    write_byte((uint8_t)(sum >> 8));
    write_byte((uint8_t)(sum & 0xFF));
}
uint8_t getStructuredPacket(uint8_t *buffer_finger_data, uint32_t timeout)
{
    memset(buffer_finger_data, 0xFF, 64);
    HAL_StatusTypeDef result = HAL_UART_Receive(&huart1, buffer_finger_data, 64, timeout);
    return (uint8_t)result;
}
uint8_t getStructuredPacket_IT()
{
    tx_flag = 0;
    idex = 0;
    memset(rx_buffer, 0xFF, 64);
    HAL_UART_Receive_IT(&huart1, &rx_buffer[idex], 1);
}
uint8_t checkPassword(void)
{
    finger_data[0] = FINGERPRINT_VERIFYPASSWORD;
    finger_data[1] = 0x00;
    finger_data[2] = 0x00;
    finger_data[3] = 0x00;
    finger_data[4] = 0x00;
    writeStructuredPacket(finger_data, 5);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    if (rx_buffer[9] == 0x00)
        return 1;
    else
        return 0;
}
uint8_t getImage()
{
    finger_data[0] = FINGERPRINT_GETIMAGE;
    writeStructuredPacket(finger_data, 1);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    // getStructuredPacket_IT();
    // while(tx_flag == 0){};
    // return rx_buffer[9];
    return finger_data[9];
}
uint8_t emptyfinger_database(void)
{
    finger_data[0] = FINGERPRINT_EMPTY;
    writeStructuredPacket(finger_data, 1);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}
uint8_t image2Tz(uint8_t slot)
{
    finger_data[0] = FINGERPRINT_IMAGE2TZ;
    finger_data[1] = slot;
    writeStructuredPacket(finger_data, 2);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}
uint8_t createModel(void)
{
    finger_data[0] = FINGERPRINT_REGMODEL;
    writeStructuredPacket(finger_data, 1);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}
uint8_t storeModel(uint16_t location)
{
    finger_data[0] = FINGERPRINT_STORE;
    finger_data[1] = 0x01;
    finger_data[2] = (uint8_t)(location >> 8);
    finger_data[3] = (uint8_t)(location & 0xFF);
    writeStructuredPacket(finger_data, 4);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}
uint8_t fingerSearch(uint8_t slot)
{
    finger_data[0] = FINGERPRINT_SEARCH;
    finger_data[1] = slot;
    finger_data[2] = 0x00;
    finger_data[3] = 0x00;
    finger_data[4] = (uint8_t)(64 >> 8);
    finger_data[5] = (uint8_t)(64 & 0xFF);
    writeStructuredPacket(finger_data, 6);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}

uint8_t fingerIDSearch(uint8_t slot,uint16_t* FingerID){
      finger_data[0] = FINGERPRINT_SEARCH;
      finger_data[1] = slot;
      finger_data[2] = 0x00;
      finger_data[3] = 0x00;
      finger_data[4] = (uint8_t)(64 >> 8);
      finger_data[5] = (uint8_t)(64 & 0xFF);
      writeStructuredPacket(finger_data,6);
      getStructuredPacket(finger_data,DEFAULTTIMEOUT);
	    *FingerID = ((uint16_t)finger_data[10]<<8) + ((uint16_t)finger_data[11]);;
      return finger_data[9];
}
uint8_t checkInputFinger(uint8_t f)
{
    f = -1;
    while (f != FINGERPRINT_OK) {
        f = getImage();
        switch (f) {
        case FINGERPRINT_OK:
            write_command((uint8_t *)"Successfully get an image", sizeof("Successfully get an image"));
            HAL_Delay(100);
            break;
        case FINGERPRINT_NOFINGER:
            SSD1306_GotoXY(10, 20);
						SSD1306_Puts("Put your finger", &Font_7x10, 1);
						SSD1306_GotoXY(30, 30);
						SSD1306_Puts("on sensor", &Font_7x10, 1);
						SSD1306_UpdateScreen();
            HAL_Delay(100);
            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            write_command((uint8_t *)"Communication error", sizeof("Communication error"));
            HAL_Delay(100);
            break;
        case FINGERPRINT_IMAGEFAIL:
            write_command((uint8_t *)"Imaging error", sizeof("Imaging error"));
            HAL_Delay(100);
            break;
        default:
            write_command((uint8_t *)"Unknown error", sizeof("Unknown error"));
            HAL_Delay(100);
            break;
        }
        HAL_Delay(50);
    }

    f = image2Tz(1);
    switch (f) {
    case FINGERPRINT_OK:
        write_command((uint8_t *)"Image converted", sizeof("Image converted"));
        HAL_Delay(100);
        break;
    case FINGERPRINT_IMAGEMESS:
        write_command((uint8_t *)"Image too messy", sizeof("Image too messy"));
        HAL_Delay(100);
        return f;
    case FINGERPRINT_PACKETRECIEVEERR:
        write_command((uint8_t *)"Communication error", sizeof("Communication error"));
        HAL_Delay(100);
        return f;
    case FINGERPRINT_FEATUREFAIL:
        write_command((uint8_t *)"Could not find fingerprint features", sizeof("Could not find fingerprint features"));
        HAL_Delay(100);
        return f;
    case FINGERPRINT_INVALIDIMAGE:
        write_command((uint8_t *)"Could not find fingerprint features", sizeof("Could not find fingerprint features"));
        HAL_Delay(100);
        return f;
    default:
        write_command((uint8_t *)"Unknown error", sizeof("Unknown error"));
        HAL_Delay(100);
        return f;
    }
}

uint8_t checkFingerPrintID(uint16_t* FingerID)
{
		SSD1306_Clear();
    SSD1306_GotoXY(10, 20);
    SSD1306_Puts("Put your finger", &Font_7x10, 1);
		SSD1306_GotoXY(30, 30);
		SSD1306_Puts("on sensor", &Font_7x10, 1);
    SSD1306_UpdateScreen();
    uint8_t p;
    p = checkInputFinger(p);
		*FingerID = 0xFFFF;
	  char buffer[20];
    p = fingerIDSearch(1, FingerID);
    if (p == FINGERPRINT_OK){
			write_command_at((uint8_t *)"Match!", sizeof("Match!"), 40, 30);
			HAL_Delay(100);
			sprintf(buffer,"User ID: %d",*FingerID);
			write_command_at(buffer, strlen(buffer), 30, 30);
			HAL_Delay(1000);
    }
    else if (p == FINGERPRINT_PACKETRECIEVEERR){
			write_command((uint8_t *)"Communication error", sizeof("Communication error"));
			HAL_Delay(100);
			return p;
    }
    else if (p == FINGERPRINT_NOTFOUND){
			write_command_at((uint8_t *)"Not match!", sizeof("Not match!"), 30, 30);
			HAL_Delay(100);
			return p;
    }
    else{
			write_command((uint8_t *)"Unknown error", sizeof("Unknown error"));
			HAL_Delay(100);
			return p;
    }
    return 0;
}

uint8_t getFingerprintEnroll(uint8_t id){
     int p = -1;
	   uint8_t command[100];
		 sprintf(command,"Waiting for valid finger to enroll as %d. Please put your finger on the screen",id);
	   write_command(command,strlen(command));
     while (p != FINGERPRINT_OK){
        p = getImage();
				switch (p) {
					case FINGERPRINT_OK:
						write_command((uint8_t*)"Successfully get an image",sizeof("Successfully get an image"));
						break;
					case FINGERPRINT_NOFINGER:
						write_command((uint8_t*)".",sizeof("."));
					 HAL_UART_Transmit(&huart1,(uint8_t*)".",sizeof("."),1000);
						break;
					case FINGERPRINT_PACKETRECIEVEERR:
					write_command((uint8_t*)"Communication error",sizeof("Communication error"));
						break;
					case FINGERPRINT_IMAGEFAIL:
					write_command((uint8_t*)"Imaging error",sizeof("Imaging error"));
						break;
					default:
					write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
						break;
				}
			HAL_Delay(50);
		}

    p = image2Tz(1);
    switch (p) {
			case FINGERPRINT_OK:
				write_command((uint8_t*)"Image converted",sizeof("Image converted"));
				break;
			case FINGERPRINT_IMAGEMESS:
				write_command((uint8_t*)"Image too messy",sizeof("Image too messy"));
				return p;
			case FINGERPRINT_PACKETRECIEVEERR:
				write_command((uint8_t*)"Communication error",sizeof("Communication error"));
				return p;
			case FINGERPRINT_FEATUREFAIL:
				write_command((uint8_t*)"Could not find fingerprint features",sizeof("Could not find fingerprint features"));
				return p;
			case FINGERPRINT_INVALIDIMAGE:
				write_command((uint8_t*)"Could not find fingerprint features",sizeof("Could not find fingerprint features"));
				return p;
			default:
				write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
				return p;
    }
		write_command((uint8_t*)"Remove finger",sizeof("Remove finger"));
    HAL_Delay(2000);
    p = -1;
    while (p != FINGERPRINT_NOFINGER) {
			p = getImage();
    }
		p = -1;
    write_command((uint8_t*)"Place same finger again",sizeof("Place same finger again"));
		while (p != FINGERPRINT_OK) {
			p = getImage();
			switch (p) {
				case FINGERPRINT_OK:
					write_command((uint8_t*)"Image taken",sizeof("Image taken"));
					break;
				case FINGERPRINT_NOFINGER:
					write_command((uint8_t*)".",sizeof("."));
					break;
				case FINGERPRINT_PACKETRECIEVEERR:
					write_command((uint8_t*)"Communication error",sizeof("Communication error"));
					break;
				case FINGERPRINT_IMAGEFAIL:
					write_command((uint8_t*)"Imaging error",sizeof("Imaging error"));
					break;
				default:
					write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
					break;
    }
		HAL_Delay(50);
  }

  p = image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
		write_command((uint8_t*)"Image converted",sizeof("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
		write_command((uint8_t*)"Image too messy",sizeof("Image too messy"));
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
		write_command((uint8_t*)"Communication error",sizeof("Communication error"));
      return p;
    case FINGERPRINT_FEATUREFAIL:
		write_command((uint8_t*)"Could not find fingerprint features",sizeof("Could not find fingerprint features"));
      return p;
    case FINGERPRINT_INVALIDIMAGE:
		write_command((uint8_t*)"Could not find fingerprint features",sizeof("Could not find fingerprint features"));
      return p;
    default:
		write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
      return p;
  }
  sprintf(command,"Creating model for #%d",id);
	write_command((uint8_t*)command,strlen(command));
  p = createModel();
  if (p == FINGERPRINT_OK) {
		write_command((uint8_t*)"Prints matched!",sizeof("Prints matched!"));
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		write_command((uint8_t*)"Communication error",sizeof("Communication error"));
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
		write_command((uint8_t*)"Finger not match",sizeof("Finger not match"));
    return p;
  } else {
    //Serial.println("Unknown error");
		write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
    return p;
  }
  p = storeModel(id);
  if (p == FINGERPRINT_OK) {
		write_command((uint8_t*)"Stored!",sizeof("Stored!"));
		char buffer[20];
		sprintf(buffer, "Your ID: %d", id);
		write_command_at(buffer,strlen(buffer), 30, 30);
		HAL_Delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		write_command((uint8_t*)"Communication error",sizeof("Communication error"));
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
		write_command((uint8_t*)"Could not store in that location",sizeof("Could not store in that location"));
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
		write_command((uint8_t*)"Error writing to flash",sizeof("Error writing to flash"));
    return p;
  } else {
		write_command((uint8_t*)"Unknown error",sizeof("Unknown error"));
    return p;
  }
	return 0x00;
}

int find_avaiable_idex_table() {
    finger_data[0] = 0x1F;
    finger_data[1] = 0x00;
    writeStructuredPacket(finger_data, 2);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    int k = 0;
    int count = 1;
    for (int i = 10; i <= 41; i++)
    {
        count = 1;
        while (count <= 8)
        {
            if (finger_data[i] % 2 == 0)
                return k;
            k++;
            finger_data[i] = finger_data[i] / 2;
            count++;
        }
    }
    return -1; // khong tim thay
}

void show_idextable() {
    finger_data[0] = 0x1F;
    finger_data[1] = 0x00;
    writeStructuredPacket(finger_data, 2);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
}

uint8_t deleteFingerprint(uint8_t id) {
    finger_data[0] = FINGERPRINT_DELETE;
    finger_data[1] = 0x00;
    finger_data[2] = id;
    finger_data[3] = 0x00;
    finger_data[4] = 0x01;
    writeStructuredPacket(finger_data, 5);
    getStructuredPacket(finger_data, DEFAULTTIMEOUT);
    return finger_data[9];
}
