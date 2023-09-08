  #include "FS.h"
  #include <functional>
  #include <base64.h>
  #include <WiFiClientSecure.h>
  #include <ESP8266WiFi.h>
  #include <WiFiUdp.h>
  #include <SoftwareSerial.h>
  #include <NTPClient.h>
  #include <string.h>
  #define THREASHOLD_ERROR 20
  #define CHUNK_SIZE 100
  #define HEADER_SIZE	400
  #define GET   "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n"
  #define HEADER  "POST /post-data.php HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s"
  #define ADD_BODY  "api_key=%s&ID=%d&MaNV=%s&HOTEN=%s&method=ADD"
  #define REMOVE_BODY "api_key=%s&ID=%s&method=REMOVE"
  #define EMPTY_BODY "api_key=%s&method=EMPTY"
  #define UPDATE_BODY "api_key=%s&method=ADD&ID=%d&DateTime=%s"
  #define HEADER_UPDATE "POST /update.php HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s"
  #define rxPin 12
  #define txPin 14
  // Set up a new SoftwareSerial object
  char buff_time[50];
  SoftwareSerial mySerial =  SoftwareSerial(rxPin,txPin);
  int tempt_id = 0;
  char name[50];
  char id[10];
  char MANV[10];
  String CommandSTM;
  const char* ssid  = "AndroidAPD4EA";
  const char* password = "15012002";
  int timeout = 0;
  long int pre_time = 0;
  long int now_time = 0;
  WiFiUDP NtpUDP;
  NTPClient TimeClient(NtpUDP);
  File root;
  File root2;
  File VersionFile;
  File StateFile;
  void sendChunk(Client *client, uint8_t *buf, int len){
      int idx = 0;
      size_t result;
      while(len > 0){
          if(len < CHUNK_SIZE){
              result = client->write(&buf[idx], len);
              len -= result;
              idx += result;
          } else {
              result = client->write(&buf[idx], CHUNK_SIZE);
              len -= result;
              idx += result;
          }
      }
      //Serial.println("Da gui xong ban tin http request");
  }
  int download(String HostName){
    int test_sum = 0;
    WiFiClient client;
    char buffer[HEADER_SIZE];
    memset(buffer,0,HEADER_SIZE);
    int length = 0;
    int received = 0;
    int byte = 0;
    uint8_t test;
    if (!client.connect(HostName.c_str(),80)){
          Serial.println("Connection failed");
          return -1;
      }
    char datalength[10];
    // Gui yeu cau tai file NewVersion
    snprintf(buffer,HEADER_SIZE,GET,(char*)"/NewVersion",HostName.c_str());
    Serial.println("Cau truc goi tin http da gui di la:");
    Serial.println(buffer);
    sendChunk(&client,(uint8_t *)buffer,strlen(buffer));
    char* endheader = NULL;
    memset(buffer,0,HEADER_SIZE);
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
            // Serial.print(buffer[received]);
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
    }
    //Serial.println("Ban tin http response la: ");
    //Serial.print(buffer);
    if(strstr(buffer,"404 Not Found")!=NULL)return 3;
    char* tempt1 = NULL;
    endheader = strstr(buffer,"Content-Length: ");
    tempt1 = strstr(endheader,"\r\n");
    memcpy(datalength,endheader+strlen("Content-Length: "),tempt1 - endheader-strlen("Content-Length: "));
    length = atoi(datalength);
    received = 0;
    // Open version.txt on esp8266 to check with data downloaded from website
    String FileName = "";
    while(received<length){
          if(client.available()){
              byte = client.read(&test,1);
              if(byte!=-1){
                received+=byte;
                FileName+=(char)test;
                //Serial.print((char)test);
              }
          }
    }
    String VersionOnEsp = "";
    VersionFile = SPIFFS.open("/version.txt", "r");
    if (!VersionFile) {
    Serial.println("Error opening version file for reading for comparing with website newversion");
    return -1;
    }
    while(VersionFile.available()){
        test =  VersionFile.read();
        if(test!=0x0D&&test!=0x0A)VersionOnEsp+=(char)test;
    }
    VersionFile.close();
    Serial.printf("NewVersion file: %s\n",FileName);
    if(strstr(FileName.c_str(),VersionOnEsp.c_str())!=NULL)return 2;
    else {
      VersionFile = SPIFFS.open("/version.txt","w");
      if (!VersionFile) {
      Serial.println("Error opening version file for reading for comparing with website newversion");
      return -1;
      }
      VersionFile.print(FileName);
      VersionFile.close();
    }
    // Gui yeu cau tai file firmware
    char WebFileFolder[50];
    sprintf(WebFileFolder,"/%s.txt",FileName.c_str());
    memset(buffer,0,HEADER_SIZE);
    snprintf(buffer,HEADER_SIZE,GET,WebFileFolder,HostName.c_str());
    Serial.println("Cau truc goi tin http da gui di la:");
    Serial.println(buffer);
    sendChunk(&client,(uint8_t *)buffer,strlen(buffer));
    endheader = NULL;
    memset(buffer,0,HEADER_SIZE);
    received = 0;
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
            // Serial.print(buffer[received]);
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
    }
    //Serial.println("Ban tin http response la: ");
    //Serial.print(buffer);
    if(strstr(buffer,"404 Not Found")!=NULL)return 3;
    tempt1 = NULL;
    endheader = strstr(buffer,"Content-Length: ");
    tempt1 = strstr(endheader,"\r\n");
    memcpy(datalength,endheader+strlen("Content-Length: "),tempt1 - endheader-strlen("Content-Length: "));
    length = atoi(datalength);
    received = 0;
    // Open file.txt on esp8266 to write data downloaded from website
    root = SPIFFS.open("/file.txt", "w");
    if (!root) {
    Serial.println("Error opening file for writing");
    return -1;
    }
    while(received<length){
          if(client.available()){
              byte = client.read(&test,1);
              if(byte!=-1){
                received+=byte;
                root.write(test);
                //Serial.print((char)test);
              }
          }
    }
    root.close();
    return  0;
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
  int CheckDownloadedFirmware(){
    char newtempt;
    int t = -1;
    int Size = 0;
    uint8_t IsFirst = 1;
    char StoredString[50];
    int PacketSize = 0;
    root = SPIFFS.open("/file.txt","r");
    if(!root){
      Serial.printf("Can not open file test.txt for reading");
      return -1;
    }
    while(root.available()){
      newtempt = root.read();
      StoredString[Size] = newtempt;
      //Serial.print(newtempt);
      if(StoredString[Size] == 0x0D){
        PacketSize = 0;
        //Serial.printf("%d",StoredString[0]);
        /* for(int i=1;i<=43;i++){
           if(StoredString[i]==0xFF)break;
           else Serial.print(StoredString[i]);
        }
        Serial.printf("%d\n",StoredString[44]); */
        //Serial.printf("Do dai goi tin: %c%c\n",StoredString[2],StoredString[3]);
        Size = 0;
        if('0'<=StoredString[2]&&StoredString[2]<='9')PacketSize+=16*(StoredString[2] - ('0'-0));
        else PacketSize+=16*(StoredString[2] - 'A'+10);
        if('0'<=StoredString[3]&&StoredString[3]<='9')PacketSize+=(StoredString[3] - ('0'-0));
        else PacketSize+=(StoredString[3] - 'A'+10);
        PacketSize*=2;
        //Serial.printf("Do dai goi tin: %d\n",PacketSize);
        if(((t=CheckError((char*)StoredString,PacketSize))!=0&&StoredString[8] == '0' && StoredString[9] == '0')||(IsFirst&&(StoredString[0]!=':'))||(!IsFirst&&(StoredString[1]!=':'))){ root.close(); return -1; }
        IsFirst = 0;
		    memset(StoredString,0xFF,sizeof(StoredString));
      }
      else Size++;
    }
    root.close();
    return 0;
  }
  int WriteFromFileToFile(){
    root2 = SPIFFS.open("/firmware.txt","w");
    if(!root2){
      Serial.printf("Can not open firmware.txt to trasnfering\n");
      return -1;
    }
    root = SPIFFS.open("/file.txt","r");
    if(!root){
      Serial.printf("Can not open file.txt to transfering\n");
      return -1;
    }
    while(root.available()){
      root2.write(root.read());
    }
    root2.close();
    root.close();
    return 0;
  }
  char tempt;
  int sum  =  0;
  String temp1 = "";
  void clear_buffer(){
    while(mySerial.available()>0)mySerial.read();
    //Serial.printf("Clearing receive buffer\n");
  }
  void TransferingFirmware(){
    int NumberError = 0;
    bool success = SPIFFS.begin();
    if(success){
      Serial.println("File system mounted with success");  
    }else{
      Serial.println("Error mounting the file system");  
    }
      root = SPIFFS.open("/firmware.txt", "r");
    if (!root) {
    Serial.println("Error opening file for reading");
    return;
    }
    Serial.printf("Ready to transfer, number of byte: %d\n",root.available());
    while(root.available()){
      tempt = root.read();
      temp1+=tempt;
      mySerial.write(tempt);
      Serial.print(tempt);
      if(tempt==0x0D){
        //Serial.printf("Got an endline\n");
        if(strstr(temp1.c_str(),":00000001FF")!=NULL){temp1 = ""; break;}
        while(1){
          if(mySerial.available()>0){
            tempt = mySerial.read();
            if(tempt=='E'){
              Serial.println("\nGot an error, transfering again\n");
              mySerial.write(temp1.c_str(),temp1.length());
            }
            else if(tempt=='O'){
              temp1 = "";
              break;
            }
            else Serial.println("Error when transfering firmware");
          }
        }
      }
    }
    //Serial.printf("End of firmware transfering, number of byte in buffer: %d",mySerial.available());
    //Serial.printf("End of firmware transfering, number of byte in buffer: %d",mySerial.available());
    root.close(); 
    SPIFFS.end();
  }
  void WaitFromSTM32(){
    tempt = 'A' ;
    clear_buffer();
    Serial.printf("Waiting for the S character\n");
    while(mySerial.available()<=0);
    Serial.printf("Number of byte available: %d\n",mySerial.available());
    tempt = mySerial.read();
    if((tempt) == 'S'){
        TransferingFirmware();
    }
    else Serial.printf("The character is wrong: %c\n",tempt);
    mySerial.end();
    mySerial.begin(38400);  
  }
  int Upload(char* name,int ID,char* MaNV){
    String BodyResponse = "";
    char buffer[1000];
    char body[150];
    int received = 0;
    int byte =  0;
    snprintf(body,150,ADD_BODY,"#150102",ID,MaNV,name);
    snprintf(buffer,1000,HEADER,"nambcn.000webhostapp.com",strlen(body),body);
    Serial.println("Content of post http message");
    Serial.print(buffer);
    WiFiClient client;
    if (!client.connect("nambcn.000webhostapp.com",80)){
          Serial.println("Connection failed");
          return -1;
    }
    sendChunk(&client,(uint8_t*)buffer,strlen(buffer));
    char* endheader = NULL;
    memset(buffer,0,sizeof(buffer));
    char tempt;
    long int pre_time = 0;
    long int now_time = 0;
    pre_time = millis();
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
          if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
              return -3;
          }
    }
    if(strstr(buffer,"404")!=NULL)return -2;
    Serial.println("The body of http response message: ");
    pre_time = millis();
    while(client.available()>0){
              byte = client.read(&tempt,1);
              Serial.print(tempt);
              BodyResponse += (char)tempt;
              if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
          }
    }
    if(strstr(BodyResponse.c_str(),"successfully")==NULL)return -3;
    return 0;
  }
  int Remove(char* ID){
    char buffer[500];
    char body[150];
    int received = 0;
    int byte =  0;
    snprintf(body,150,REMOVE_BODY,"#150102",ID);
    snprintf(buffer,500,HEADER,"nambcn.000webhostapp.com",strlen(body),body);
    Serial.println("Content of post http message");
    Serial.print(buffer);
    WiFiClient client;
    if (!client.connect("nambcn.000webhostapp.com",80)){
          Serial.println("Connection failed");
          return -1;
    }
    sendChunk(&client,(uint8_t*)buffer,strlen(buffer));
    char* endheader = NULL;
    memset(buffer,0,sizeof(buffer));
    long int pre_time = 0;
    long int now_time = 0;
    pre_time = millis();  
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
              // Serial.print(buffer[received]);
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
          if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
              return -3;
          }
    }
    if(strstr(buffer,"404")!=NULL)return -2;
    String BodyResponse = "";
    char tempt;
    Serial.println("The body of http response message: ");
    pre_time = millis();
    while(client.available()>0){
              byte = client.read(&tempt,1);
              Serial.print(tempt);
              BodyResponse += (char)tempt;
              if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
          }
    }
    if(strstr(BodyResponse.c_str(),"successfully")==NULL)return -3;
    return 0;
  }
  int Empty(){
    char buffer[500];
    char body[150];
    int received = 0;
    int byte =  0;
    snprintf(body,150,EMPTY_BODY,"#150102");
    snprintf(buffer,500,HEADER,"nambcn.000webhostapp.com",strlen(body),body);
    Serial.println("Content of post http message");
    Serial.print(buffer);
    WiFiClient client;
    if (!client.connect("nambcn.000webhostapp.com",80)){
          Serial.println("Connection failed");
          return -1;
    }
    sendChunk(&client,(uint8_t*)buffer,strlen(buffer));
    char* endheader = NULL;
    memset(buffer,0,sizeof(buffer));
    long int pre_time = 0;
    long int now_time = 0;
    pre_time = millis();
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
              // Serial.print(buffer[received]);
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
          if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
              return -3;
          }
    }
    if(strstr(buffer,"404")!=NULL)return -2;
    String BodyResponse = "";
    char tempt;
    Serial.println("The body of http response message: ");
    pre_time = millis();
    while(client.available()>0){
              byte = client.read(&tempt,1);
              Serial.print(tempt);
              BodyResponse += (char)tempt;
              if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
          }
    }
    if(strstr(BodyResponse.c_str(),"successfully") == NULL)return -3;
    return 0;
  }
  int UpdateDatabase(char* time,int id){
    char buffer[500];
    char body[150];
    int received = 0;
    int byte =  0;
    snprintf(body,150,UPDATE_BODY,"#150102",id,time);
    snprintf(buffer,500,HEADER_UPDATE,"nambcn.000webhostapp.com",strlen(body),body);
    Serial.println("Content of post http message:");
    Serial.print(buffer);
    WiFiClient client;
    if (!client.connect("nambcn.000webhostapp.com",80)){
          Serial.println("Connection failed");
          return -1;
    }
    sendChunk(&client,(uint8_t*)buffer,strlen(buffer));
    char* endheader = NULL;
    memset(buffer,0,sizeof(buffer));
    long int pre_time = 0;
    long int now_time = 0;
    pre_time = millis();
    while(endheader==NULL){
          if(client.available()){
              byte = client.read(&buffer[received],1);
              if(byte!=-1){
              // Serial.print(buffer[received]);
              received+=byte;
              endheader = strstr(buffer,"\r\n\r\n");
              }
          }
          if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
              return -3;
          }
    }
    if(strstr(buffer,"404")!=NULL)return -2;
    String BodyResponse = "";
    char tempt;
    Serial.println("The body of http response message: ");
    pre_time = millis();
    while(client.available()>0){
              byte = client.read(&tempt,1);
              Serial.print(tempt);
              BodyResponse += (char)tempt;
              if(((now_time=millis())-pre_time)>10000){
              Serial.println("Dont received response from server");
          }
    }
    if(strstr(BodyResponse.c_str(),"successfully") == NULL)return -3;
    return 0;
  }
  int GetTime(NTPClient client,char* formattedDate){
      long int pre_time = millis();
      long int now_time = 0;
      while(!client.update()){
          client.forceUpdate();
          if((now_time = millis()) - pre_time > 10000)return -1;
      }
      strcpy(formattedDate,(client.getFormattedTime()).c_str()); 
      int NumDay = client.getDay();
      char Day[20];
      switch(NumDay){
        case 0:
          strcpy(Day," Sunday");
          break;
        case 1:
          strcpy(Day," Monday");
          break;
        case 2:
          strcpy(Day," Tuesday");
          break;
        case 3:
          strcpy(Day," Wednesday");
          break;
        case 4:
          strcpy(Day," Thursday");
          break;
        case 5:
          strcpy(Day," Friday");
          break;
        case 6:
          strcpy(Day," Saturday");
          break;
        case 7:
          strcpy(Day," Sunday");
          break;
        default:
          strcpy(Day," Error");
          break;
      }    
      strcat(formattedDate,Day);
      return 0;
  }
  void setup(){
    pinMode(4,INPUT);
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    mySerial.begin(38400);
    Serial.begin(9600);
    bool success = SPIFFS.begin();
    if(success){
      Serial.println("File system mounted with success");  
    }
    else{
      Serial.println("Error mounting the file system");  
    } 
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    while(WiFi.status()!= WL_CONNECTED){
      delay(500);
      Serial.print(".");
    } 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("initialization done.");   
    //Remove(20020697);
    TimeClient.begin();
    TimeClient.setTimeOffset(+7*60*60);
    //char time[100];
    //Serial.printf("Gia tri date field: %s",test_time);
    int r = download((String)"nambcn.000webhostapp.com");
    if(r==-1)Serial.println("Error when downloading");
    else if(r==2)Serial.println("There is no updated firmware");   
    else if(r==3)Serial.println("Not found the wanted file ");
    else {
    if(CheckDownloadedFirmware()!=0)Serial.println("The firmware is wrong"); 
    if(WriteFromFileToFile()!=0)Serial.println("Transfering data from file.txt to firmware.txt failed");
    Serial.println("Completing checking and rewrite new firmware file to backup file");
    } 
    SPIFFS.end(); 
    Serial.println("Gui yeu cau Who");
    mySerial.write("WO",2);
    long int pre_time = millis();
    long int now_time = 0;
    while(mySerial.available()<=0){
        if(((now_time = millis()) - pre_time)>5000){
           Serial.println("No response for idetifing application or bootloader ");
           tempt = 'A';
           break;
        }
    }
    if((tempt=mySerial.read()) == 'B'){
    Serial.println("Bat dau truyen firmware voi Bootloader");
    //mySerial.write("UP",2);
    //delay(2000);
    mySerial.write("S",1);
    WaitFromSTM32();
    //clear_buffer();
    }
    else if(tempt == 'A'){
      if(r==0){
      delay(5000);
      Serial.println("Change the flow of application program to Bootloader");
      mySerial.write("UP",2);
      delay(2000);
      Serial.println("Gui yeu cau Who");
      mySerial.write("WO",2);
      while(mySerial.available()<=0);
      if((tempt=mySerial.read()) == 'B'){
      Serial.println("Bat dau truyen firmware voi Bootloader");
      //mySerial.write("UP",2);
      //delay(2000);
      mySerial.write("S",1);
      WaitFromSTM32();
      //clear_buffer();
     }
    }
    else mySerial.write("NO",2);
    }  
  }
  void Test(){
      Serial.println("This is a test of callback function");
  }
  long int time_out = 40000000;
  int test = 5;
  void light_sleep(){
   WiFi.mode(WIFI_OFF);
  // wifi_station_disconnect();
  // wifi_set_opmode_current(0x00);
   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // set sleep type, the above    posters wifi_set_sleep_type() didnt seem to work for me although it did let me compile and upload with no errors 
   wifi_disable_gpio_wakeup();
  //gpio_pin_wakeup_enable(GPIO_ID_PIN(4), GPIO_PIN_INTR_LOLEVEL); // GPIO_ID_PIN(2) corresponds to GPIO2 on ESP8266-01 , GPIO_PIN_INTR_LOLEVEL for a logic low, can also do other interrupts, see gpio.h above
   wifi_enable_gpio_wakeup(GPIO_ID_PIN(4), GPIO_PIN_INTR_LOLEVEL);
   wifi_fpm_open(); // Enables force sleep
   //wifi_fpm_set_wakeup_cb(Test);
   //long int now_time = millis();
   test = wifi_fpm_do_sleep(0x0FFFFFFE); // Sleep for longest possible time
   //long int pre_time = millis();
   //Serial.printf("Gia tri time: %ld\n",pre_time - now_time);
   //delay(200);
}
  void init_wifi(){
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    while(WiFi.status()!= WL_CONNECTED){
      delay(500);
      Serial.print(".");
    } 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("initialization done."); 
    TimeClient.begin(); 
    TimeClient.setTimeOffset(+7*60*60);
  }
  String ReadStringFromSTM(){
      long int pre_time = millis();
      long int now_time = 0;
      int IsHeader = 1;
      String buffer = "";
      while(mySerial.available()<=0);
      char test_tempt = 0;
      while(test_tempt != '\n'){
         if(((now_time = millis())-pre_time) > 10000){
             Serial.println("Time out when read string from stm32");
             break;
         }
         if(mySerial.available()>0){
            if((test_tempt = mySerial.read())!=NULL&&test_tempt!=1){
              if((test_tempt==127)&&(IsHeader==1)||IsHeader == 0){
              buffer += (char)test_tempt;
              IsHeader = 0;
              //Serial.printf("%c",test_tempt);
              }
            }
         }
      }
      //clear_buffer();
      return buffer;
  }  
  int CheckAddCommand(String buffer){
    char test_buffer[10];
    sprintf(test_buffer,"%c|AD|",127);
     char buffer_name[50];
     char buffer_mnv[10];
     char buffer_id[5];
     memset(buffer_name,0,50);
     memset(buffer_mnv,0,10);
     memset(buffer_id,0,5);
     if(strstr(buffer.c_str(),test_buffer)==NULL)return -1;
     const char *tempt =  buffer.c_str();
     char buffer_length[5];
     memset(buffer_length,0,5);
     tempt = strstr(tempt,"|");
     tempt = strstr(tempt+1,"|");
     memcpy(buffer_length,tempt+1,4);
     Serial.printf("Gia tri truong length: %s\n",buffer_length);
     int length = 0;
     int power = 1;
     for(int i=3;0<=i;i--,power*=16){
        if('0'<=buffer_length[i]&&buffer_length[i]<='9')length += power*(buffer_length[i] + (0-'0'));
        else if ('A'<=buffer_length[i]&&buffer_length[i]<='F')length += power*(buffer_length[i]-'A'+10);
     }
     //Serial.printf("Gia tri cua truong length: %d",length);
     int check_sum = 0;
     tempt =  strstr(tempt+1,"|");
     tempt+=1;
     for(int i=0,j=0;j<3;i++){
        if(tempt[i]=='|')j++;
        else check_sum += tempt[i];
     }
     check_sum = check_sum&(0xFF);
     for(int i=0;tempt[i]!='|';i++){
        buffer_name[i] = tempt[i];
     }
     Serial.println(buffer_name);
     tempt = strstr(tempt,"|");
     tempt+=1;
     for(int i=0;tempt[i]!='|';i++){
        buffer_mnv[i] = tempt[i];
     }
     Serial.println(buffer_mnv);
     tempt = strstr(tempt+1,"|");
     tempt += 1;
     for(int i=0;tempt[i]!='|';i++){
        buffer_id[i] = tempt[i];
     }
     Serial.println(buffer_id);
     tempt = strstr(tempt+1,"|");
     tempt += 1;
     int test_checksum = 0;
     if('0'<=tempt[1]&&tempt[1]<='9')test_checksum += (tempt[1] - ('0'-0));
     else if('A'<=tempt[1]&&tempt[1]<='F')test_checksum += (tempt[1] - 'A'+10);
     if('0'<=tempt[0]&&tempt[0]<='9')test_checksum += 16*(tempt[0] - ('0'-0));
     else if('A'<=tempt[0]&&tempt[0]<='F')test_checksum += 16*(tempt[0] - 'A'+10);
     if(check_sum!=test_checksum)return -2;
     if(Upload(buffer_name,atoi(buffer_id),buffer_mnv)!=0)return -3;
     return 0;
  }
  int CheckRemoveCommand(String buffer){
  char test_buffer[10];
  char ID[5];
  memset(ID,0,5);
  sprintf(test_buffer,"%c|RE|",127);
  if(strstr(buffer.c_str(),test_buffer)==NULL)return -1;
  char* tempt = strstr(buffer.c_str(),"|");
  tempt = strstr(tempt+1,"|");
  tempt++;
  int checksum = 0;
  for(int i=0;tempt[i]!='|';i++){
      checksum+= tempt[i];
      ID[i] = tempt[i];
  }
  tempt = strstr(tempt,"|");
  tempt++;
  int test_checksum = 0;
  if('0'<=tempt[1]&&tempt[1]<='9')test_checksum += (tempt[1] - ('0'-0));
  else if('A'<=tempt[1]&&tempt[1]<='F')test_checksum += (tempt[1] - 'A'+10);
  if('0'<=tempt[0]&&tempt[0]<='9')test_checksum += 16*(tempt[0] - ('0'-0));
  else if('A'<=tempt[0]&&tempt[0]<='F')test_checksum += 16*(tempt[0] - 'A'+10);
  //Serial.printf("Gia tri test_checksumm va checksum la: %d %d",test_checksum,checksum);
  //Serial.printf("Gia tri cua truong ID la: %s\n",ID);
  if(test_checksum == checksum){
    if(Remove(ID)!=0){
      Serial.println("Error when remove record on database");
     return -2;
    }
  }
  return 0;
  }
  int CheckEmptyCommand(String buffer){
    char test_buffer[10];
    sprintf(test_buffer,"%c|EM|",127);
    if(strstr(buffer.c_str(),test_buffer)==NULL)return -1;
    if(Empty()!=0){Serial.println("Error when empting database"); return -2;}
    return 0;
  } 
  int CheckAddTime(String buffer){
    char time[50];
    char id[5];
    char* tempt = NULL;
    memset(id,0,5);
    if(strstr(buffer.c_str(),"ADDTIME")!=NULL){
       tempt = strstr(buffer.c_str(),"|");
       tempt++;
       for(int i=0;tempt[i]!='|';i++){
          id[i] = tempt[i];
       }
       return atoi(id);
    }
    else return -1;
  }
  char* t1 = NULL;
  long int test1 = 0;
  long int test2 = 0;
  void loop(){
   timeout = 0;
   Serial.println("Going to sleep now");
   delay(200);
   test1 = millis();
   light_sleep();
   test2 = millis();
   delay(200);
   //wifi_fpm_do_wakeup();
   Serial.printf("Gia tri time: %ld\n",test2-test1);
   Serial.printf("Return of sleep function: %d\n",test);
   WiFi.mode(WIFI_STA);
   Serial.println("Wake up from STM32");
   pre_time = millis();
   while(mySerial.available()<=0){
        now_time = millis();
        if((now_time - pre_time) > 5000){
          Serial.println("Wake up by watchdog reset timer");
          timeout = 1;
          break;
        }
   }
   if(timeout!=1){
      CommandSTM = ReadStringFromSTM();
      Serial.print(CommandSTM);
      init_wifi();
      if(CheckAddCommand(CommandSTM)==0){ 
        Serial.println("Completing adding new record to database");  
        mySerial.write('O');
      }
      else if(CheckRemoveCommand(CommandSTM)==0){
        Serial.println("Conpleting removing record on database");
        mySerial.write('O');
      }
      else if(CheckEmptyCommand(CommandSTM)==0){
        Serial.println("Completing empting database");
        mySerial.write('O');
      }
      else if((tempt_id=CheckAddTime(CommandSTM))!=-1){
        tempt_id = GetTime(TimeClient,buff_time);
        UpdateDatabase(buff_time,tempt_id);
        //long int pre_time = 0;
        //long int now_time = 0;
        pre_time  = millis();
        while(1){
            if(mySerial.available()<=0)break;
            else {
            CommandSTM = ReadStringFromSTM();
            Serial.print(CommandSTM);
            if((tempt_id=CheckAddTime(CommandSTM))!=-1){
              tempt_id = GetTime(TimeClient,buff_time);
              UpdateDatabase(buff_time,tempt_id);
            }
          }
        }
      }
      else mySerial.write('E');
       /* if(strstr(CommandSTM.c_str(),"ADD")!=NULL){
          memset(name,0,sizeof(name));
          t1 = strstr(CommandSTM.c_str(),"|");
          memcpy(name,t1+1,(strstr(t1+1,"|")-1)-(t1+1)+1);
          Serial.println(name);
      } */
   }
}
