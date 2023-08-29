#include "CommunicationESP.h"
uint8_t test_IT = 0;
uint8_t test_buffer[50];
uint8_t CommandFromArduino[10];
uint8_t test_buffer[50];
uint64_t ConvertToDecimal(char*array,int idex1,int idex2){
  int power = 1;
  uint64_t sum = 0;
	for(int i = idex2;i>=idex1;i--,power*=16){
	        if('0'<=array[i]&&array[i]<='9')sum+=power*(array[i] - ( '0' - 0 ));
	        else sum+=power*(array[i] - 'A' + 10);
	}  
	return sum;
}
int EmptyFlash(uint32_t flash_start_address,uint32_t NumberOfPage){
	uint32_t PageError;
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef InitFlash;
	InitFlash.TypeErase = FLASH_TYPEERASE_PAGES;
  InitFlash.PageAddress = flash_start_address;
	InitFlash.NbPages = NumberOfPage;
	int tempt = HAL_FLASHEx_Erase(&InitFlash, &PageError);
  HAL_FLASH_Lock();
	if(tempt != HAL_OK)return HAL_FLASH_GetError ();
  else return HAL_OK;	
}
int write_to_flash(char*array,int size,uint32_t flash_start_address){
	char tempt;
  if(size == 8 || size == 16 || size == 32 || size == 24){
	for(int i=10;i<14;i+=2){
	      tempt = array[i];
		  array[i] = array[16-(i-10)];
		  array[16-(i-10)] = tempt;
		  tempt = array[i+1];
		  array[i+1] = array[17-(i-10)];
		  array[17-(i-10)] = tempt;
	}
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,flash_start_address,ConvertToDecimal(array,10,17));
	flash_start_address += 4;
	}
  if(size == 16 || size == 32 || size == 24){
	for(int i=18;i<22;i+=2){
	      tempt = array[i];
		  array[i] = array[24-(i-18)];
		  array[24-(i-18)] = tempt;
		  tempt = array[i+1];
		  array[i+1] = array[25-(i-18)];
		  array[25-(i-18)] = tempt;
	}
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,flash_start_address,ConvertToDecimal(array,18,25));
	flash_start_address += 4;
	}
	if(size == 24 || size == 32){
	 for(int i=26;i<30;i+=2){
          tempt = array[i];
		  array[i] = array[32-(i-26)];
		  array[32-(i-26)] = tempt;
		  tempt = array[i+1];
		  array[i+1] = array[33-(i-26)];
		  array[33-(i-26)] = tempt;
    }
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,flash_start_address,ConvertToDecimal(array,26,33));
	flash_start_address += 4;
	}
	if(size == 32){
    for(int i=34;i<38;i+=2){
          tempt = array[i];
		  array[i] = array[40-(i-34)];
		  array[40-(i-34)] = tempt;
		  tempt = array[i+1];
		  array[i+1] = array[41-(i-34)];
		  array[41-(i-34)] = tempt;
    }
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,flash_start_address,ConvertToDecimal(array,34,41));
	flash_start_address += 4;
	}
 return 0;
}
int CheckError(char* CheckedArray,int size){
    int checksum = 0;
    for(int i=2;i<10+size;i+=2){
        if('0'<=CheckedArray[i]&&CheckedArray[i]<='9')checksum+=16*(CheckedArray[i] - ('0'-0));
        else checksum+=16*(CheckedArray[i] - 'A'+10);
        if('0'<=CheckedArray[i+1]&&CheckedArray[i+1]<='9')checksum+=(CheckedArray[i+1] - ('0'-0));
        else checksum+=(CheckedArray[i+1] - 'A'+10);
    }
    uint8_t mychecksum = 0;
    if('0'<=CheckedArray[10+size]&&CheckedArray[10+size]<='9')mychecksum+=16*(CheckedArray[10+size] - ('0'-0));
        else mychecksum+=16*(CheckedArray[10+size] - 'A'+10);
        if('0'<=CheckedArray[11+size]&&CheckedArray[11+size]<='9')mychecksum+=(CheckedArray[11+size] - ('0'-0));
        else mychecksum+=(CheckedArray[11+size] - 'A'+10);  
    checksum = (1+~checksum)&(uint8_t)0xFF;
    if(checksum == mychecksum)return 0;
    else return -1;
}
void JumpToUpdatedFirmware(){
	HAL_RCC_DeInit();
	HAL_DeInit();
	  /* Turn off fault harder*/
   SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk |\
   SCB_SHCSR_BUSFAULTENA_Msk | \
   SCB_SHCSR_MEMFAULTENA_Msk ) ; 
	  /* Set Main Stack Pointer*/
   __set_MSP(*((volatile uint32_t*) ADDRESS_START_BLINK_LED_APPLICATION));
  uint32_t JumpAddress = *((volatile uint32_t*) (ADDRESS_START_BLINK_LED_APPLICATION + 4));
  /* Set Program Counter to Blink LED Apptication Address*/
  void (*reset_handler)(void) = (void*)JumpAddress;
  reset_handler();
}
void ConvertDecimalToHex(uint32_t data,char*result){ 
	int idex  = 0;
	int tempt = 0;
  while(data>0){
  tempt = data%16;
	if(0<=tempt&&tempt<=9)result[idex] = tempt + '0' - 0;
  else if(9<tempt&&tempt<=15)result[idex] = tempt - 10 + 'A';
	idex++;
  data /= 16;
	}
}
int WriteFingerprintRecord(char*name,int size,uint32_t id){
	uint32_t data = 0;
	int returnvalue = 0;
	if(size>28)return -2;  
  char array[100];
	int power = 1;
  for(int i=0,idex = 0;idex<size;idex++,i+=2){
  array[i+1] = (int)(name[idex]%16);
	array[i] = ((int)name[idex]/16)%16;
	}
  int tempt = (size*2)%8;
  if(tempt!=0)tempt = 8-tempt;
  for(int i=size*2;i<size*2+tempt;i+=2){
	array[i+1] = 0;
	array[i] = 0;
	}   
  size = 2*size+tempt;
  uint32_t started_adress =  ADDRESS_DATABASE;
	while((data = *(__IO uint32_t*)(started_adress))!=0xFFFFFFFF){
				started_adress += 32;
	}
	uint32_t PlacedID = started_adress + 28;
	HAL_FLASH_Unlock();
  if(size>=8){
		data = 0; 
		power = 1;
		for(int i=0;i<8;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue=-1;
}
	if(size >= 16){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=8;i<16;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue= -1;
}
	if(size >= 24){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=16;i<24;i++,power*=16){
		data += power*array[i];
	}
	 if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue= -1;
}
	if(size >= 32){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=24;i<32;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue= -1;
} 
	if(size >= 40){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=32;i<40;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue=-1;
}
	if(size >= 48){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=40;i<48;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue= -1;
}
	if(size >= 48){
		data = 0;
		power = 1;
		started_adress += 4;
	  for(int i=40;i<48;i++,power*=16){
		data += power*array[i];
	}
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,started_adress,data)!=HAL_OK)returnvalue= -1;
}
  if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,PlacedID,(uint32_t)id)!=HAL_OK)returnvalue=-1;;
	HAL_FLASH_Lock();
  return returnvalue;
}
int test_flash = 0;
int ReadFirmware(){
	test_flash = 0;
	uint8_t ErrorNumber = 0;
  uint8_t test_buffer_length = 0;
	uint32_t flash_adress = (uint32_t)(0x08004000);
	test_flash = EmptyFlash(flash_adress,45);
  HAL_Delay(3000);
	//test_flash = 1;
	//EmptyFlash((uint32_t)flash_adress+1024*44,30);
	//HAL_Delay(2000);
  test_flash = 2;
	uint8_t confirmcode = 'O';
	uint8_t errorcode = 'E';
	uint8_t startcode = 'S';
  uint8_t closecode = 'C';
  int PacketSize = 0;   
	int t = -1;
	uint8_t IsFirst = 1;
	HAL_StatusTypeDef status = 0;
	HAL_FLASH_Unlock(); 
	memset(test_buffer,0xFF,sizeof(test_buffer));
	test_buffer_length = 0;
	HAL_UART_Transmit(&huart2,&startcode,1,100);
	while (1)
  {	  
		if(ErrorNumber > THREASHOLD_ERROR){
			HAL_UART_Transmit(&huart2,&closecode,1,100);
			NVIC_SystemReset();
		}
		status = HAL_UART_Receive(&huart2,&test_buffer[test_buffer_length],1,10000);
		if(status == HAL_TIMEOUT)break;
	  if(test_buffer[test_buffer_length] == 0x0D){
		test_buffer_length = 0;
		//HAL_Delay(4000);
		PacketSize = 0;
		if(strstr((char*)test_buffer,":00000001FF")!=NULL){ break; }
	  if('0'<=test_buffer[2]&&test_buffer[2]<='9')PacketSize+=16*(test_buffer[2] - ('0'-0));
        else PacketSize+=16*(test_buffer[2] - 'A'+10);
    if('0'<=test_buffer[3]&&test_buffer[3]<='9')PacketSize+=(test_buffer[3] - ('0'-0));
        else PacketSize+=(test_buffer[3] - 'A'+10);
		PacketSize*=2;
			if(((t=CheckError((char*)test_buffer,PacketSize))!=0&&test_buffer[8] == '0' && test_buffer[9] == '0')||(IsFirst&&(test_buffer[0]!=':'))||(!IsFirst&&(test_buffer[1]!=':'))){HAL_UART_Transmit(&huart2,&errorcode,1,100); ErrorNumber++;}
		else if(t==0 || test_buffer[8] != '0' || test_buffer[9] != '0'){
			HAL_UART_Transmit(&huart2,&confirmcode,1,100);
			if(test_buffer[8] == '0' && test_buffer[9] == '0'){
				write_to_flash((char*)test_buffer,PacketSize,flash_adress);
				flash_adress += ((int)(PacketSize/8))*4;
				ErrorNumber = 0;
			}
		}
		IsFirst = 0;
		memset(test_buffer,0xFF,sizeof(test_buffer));
		}
		else {
		test_buffer_length++;
		}  
  }
	HAL_FLASH_Lock();
	return 0;
}
void WakeUpESP(){
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
}
int AddRecord(char* name,char* tempt_mnv, int ID){
  char buffer[200];  
	char tempt_id[5];
	char tempt_length[10];
	char checksum_tempt[5];
	int check_sum = 0;
  sprintf(tempt_id,"%d",ID);
	for(int i=0;i<strlen(tempt_id);i++)check_sum += tempt_id[i];
	for(int i=0;i<strlen(tempt_mnv);i++)check_sum += tempt_mnv[i];
	for(int i=0;i<strlen(name);i++)check_sum += name[i];
	int length = strlen(tempt_id) + strlen(tempt_mnv) + strlen(name);
    for(int i=3;0<=i;i--){
		 if(0<=(length%16)&&(length%16)<10)tempt_length[i] = length%16 + '0' - 0;
		else if(10<=(length%16)&&(length%16)<=15)tempt_length[i] = length%16 - 10 + 'A';
		length = (int)length/16;
	}
	for(int i=1;0<=i;i--){
		 if(0<=(check_sum%16)&&(check_sum%16)<10)checksum_tempt[i] = check_sum%16 + '0' - 0;
		else if(10<=(check_sum%16)&&(check_sum%16)<=15)checksum_tempt[i] = check_sum%16 - 10 + 'A';
		check_sum = (int)check_sum/16;
	}
   sprintf(buffer,"%s|%s|%s|%s|%s|%s|%s|%s",START_CODE,ADD_CODE,tempt_length,name,tempt_mnv,tempt_id,checksum_tempt,END_CODE);
   if(HAL_UART_Transmit(&huart2,buffer,strlen(buffer),5000)!=HAL_OK)return -1;
	 char t;
	 if(HAL_UART_Receive(&huart2,&t,1,20000)==HAL_OK){
			if(t== 'O')return 0;
		  else if(t == 'E')return -1;
	 }
   else return -1;
}
int RemoveRecord(int ID){
   char buffer[200];
	 char tempt_id[5];
	 sprintf(tempt_id,"%d",ID);
	 int check_sum = 0;
	 for(int i=0;i<strlen(tempt_id);i++)check_sum += tempt_id[i];
	 char tempt_checksum[5];
	 memset(tempt_checksum,0,5);
   if(0<=(check_sum%16)&&(check_sum%16)<=9)tempt_checksum[1]= (check_sum%16)+'0' - 0;
	 else if(10<=(check_sum%16)&&(check_sum%16)<=15)tempt_checksum[1] = (check_sum%16) - 10 + 'A';
	 check_sum /=16;
	 if(0<=(check_sum%16)&&(check_sum%16)<=9)tempt_checksum[0]= (check_sum%16)+'0' - 0;
	 else if(10<=(check_sum%16)&&(check_sum%16)<=15)tempt_checksum[0] = (check_sum%16) - 10 + 'A';
	 sprintf(buffer,"%s|%s|%d|%s|%s",START_CODE,REMOVE_CODE,ID,tempt_checksum,END_CODE);
	 if(HAL_UART_Transmit(&huart2,buffer,strlen(buffer),5000)!=HAL_OK)return -1;
}