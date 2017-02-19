#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"

VR myVR(2, 19);    // 2:TX 19:RX (VR3)
SoftwareSerial BTserial(11, 14); // TX | RX (bluetooth)
/***************************************************************************/
#define device10  (0)
#define device11  (1)
#define device20  (2)
#define device21  (3)
#define device30  (4)
#define device31  (5)
#define CMD_BUF_LEN      64+1
#define CMD_NUM     10
typedef int (*cmd_function_t)(int, int);
/***************************************************************************/
uint8_t cmd[CMD_BUF_LEN];
uint8_t cmd_cnt;
uint8_t *paraAddr;
uint8_t buf[255];
uint8_t records[7]; // save record
short chnl = -1; // channel
short btn[5] = {4, 5, 6, 7, 8};
// Send via bluetooth
char device1 = '1'; // 8
char device2 = '2'; // 12
char device3 = '3'; // 13
/***************************************************************************/
short getBtn();
void beginTrn(short& chnl);
void sendCmd(const char& cmdSent);
void sptoR();
int findPara(int len, int paraIndex, uint8_t **addr);
int cmdLoad(int len, int paraNum);
int cmdClear(int len, int paraNum);
int cmdSigTrain(int len, int paraNum);
void printLoad(uint8_t *buf, uint8_t len);
int printSigTrain(uint8_t *buf, uint8_t len);
void printSignature(uint8_t *buf, int len);
void printVR(uint8_t *buf);
// Prepare before speaking command
void prepSpeech(){
  myVR.clear();
  myVR.load((uint8_t)device10); myVR.load((uint8_t)device11);
  myVR.load((uint8_t)device20); myVR.load((uint8_t)device21);
  myVR.load((uint8_t)device30); myVR.load((uint8_t)device31);
}

void setup(void){
  /** initialize */
  myVR.begin(9600); // VR3 was set baund by 9600
  Serial.begin(115200);
  pinMode(myVR.ring, OUTPUT); // Whistle
  for(short i = 1; i < 5; ++i){
    pinMode(btn[i], INPUT); // button (0 -> 4 <=> command, train, gate 1, 2, 3
  }
  pinMode(btn[0], INPUT_PULLUP); // if push on btn[0] (command) then set LOW, else set HIGH
  digitalWrite(myVR.ring, LOW);
  myVR.clear();
  cmd_cnt = 0;
}

void loop(void){
BEGIN:
  short btn = getBtn();
  if(btn == -1) goto BEGIN;
  short cmdSent; // command that will be sent
  switch(btn){
  case 0: // send command
    chnl = -1; // channel
    prepSpeech();
    sptoR(); // speech to recognize
    myVR.clear();
    break;
  case 1: // begin train
    if(chnl != -1){ // must choose the channel before trainning
      beginTrn(chnl);
      chnl = -1;
    }
    break;
  default: // choose channel for training (2, 3, 4 <=> 0, 1, 2)
    chnl = btn;
    break;
  }
}

// Get button is pressing
short getBtn(){
  short retbtn = -1;
  if(digitalRead(btn[0]) == LOW)retbtn = 0;
  Serial.println(retbtn);
  for(short i = 1; i < 5; ++i){
    if(digitalRead(btn[i])){
      if(retbtn != -1) return -1; // push on more one button at the same time
      retbtn = i;
    }
  }
  return retbtn;
}

// Begin train
void beginTrn(short& chnl){
  short len, paraNum;
  char str[13], channel[2], ch;
  if(chnl == 2){chnl = 0; ch = 'a';} // 0 -> 1, a -> b
  else if(chnl == 3){chnl = 2; ch = 'c';} // 2 -> 3, c -> d
  else if(chnl == 4){ch = 'e';} // 4 -> 5, e -> f
  for(short i = 0; i < 2; ++i){ // 1 command speech 2 times
    itoa(chnl + i, channel, 10);
    strcpy(str, "sigtrain ");
    strcat(str, channel); strcat(str, " ");
    channel[0] = ch + i; strcat(str, channel); // Ex: sigtrain 0 a
    //len = 13; // sigtrain 0 a => 13 charaters
    for(int i = 0; i < 13; ++i){ // convert char* to uint8_t*
      cmd[i] = (uint8_t)str[i];
    }
    //paraNum = 3; // number of para in command
    if(!cmdSigTrain(13, 3)){ // train kênh chưa thành công
      --i;
      continue;
    }
    strcpy(str, "load ");
    itoa(chnl + i, channel, 10); strcat(str, channel);
    Serial.print(chnl + i);
    Serial.print(" ");
    Serial.println(channel);
    cmdLoad(7, 2);
    myVR.ringRing(2, 125, 125);
  }
}

// Send command via bluetooth
void sendCmd(const char& device){
  BTserial.begin(38400);
  BTserial.print(device);
  myVR.begin(9600);
}

// Speech to recognize
void sptoR(){
  int ret, devc = -1;
  short state = -1;
  myVR.ringRing();
  while(digitalRead(btn[0]) == LOW){
    Serial.println("DMT");
    ret = myVR.recognize(buf, 50);
    if(ret > 0){
      switch(buf[1]){
        case device10: case device11:
          myVR.ringRing(2, 125, 125);
          sendCmd(device1);
          break;
        case device20: case device21:
          myVR.ringRing(2, 125, 125);
          sendCmd(device2);
          break;
        case device30: case device31:
          myVR.ringRing(2, 125, 125);
          sendCmd(device3);
          break;
      }
      //printVR(buf);
    }
  }
}

int findPara(int len, int paraIndex, uint8_t **addr){
  int cnt=0, i, paraLen;
  uint8_t dt;
  for(i=0; i<len; ){
    dt = cmd[i];
    if(dt!='\t' && dt!=' '){
      cnt++;
      if(paraIndex == cnt){ // đây là para cần lấy (truyền vào là 1 thì lấy para đầu tiên)
        *addr = cmd+i; // addr[0] trỏ tới vị trí đầu tiên của para trong chuỗi command
        paraLen = 0;
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){ // xác định chiều dài para này
          i++;
          paraLen++;
        }
        return paraLen;
      }
      else{
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n') i++; // chạy qua para này, vì nó không phải là para muốn lấy
      }
    }
    else i++; // gặp khoẳng trắng hay tab thì nhảy sang ký tự kế tiếp
  }
  return -1; // không có para nào trong chuỗi command
}

int cmdLoad(int len, int paraNum){
  int i, ret;
  for(i=2; i<=paraNum; i++){
    findPara(len, i, &paraAddr);
    records[i-2] = atoi((char *)paraAddr);
    if(records[i-2] == 0 && *paraAddr != '0') return -1;
  }
  ret = myVR.load(records, paraNum-1, buf);
  if(ret >= 0) printLoad(buf, ret);
  else Serial.println(F("Load failed or timeout."));
  return 0;
}

int cmdClear(int len, int paraNum){
  if(paraNum != 1) return -1;
  if(myVR.clear() == 0) Serial.println(F("Recognizer cleared."));
  else Serial.println(F("Clear recognizer failed or timeout."));
  return 0;
}

int cmdSigTrain(int len, int paraNum){
  int ret;
  uint8_t *lastAddr;
  findPara(len, 2, &paraAddr); // lấy para thứ 2 trong command (sigtrain 0 ono => 0 là para thứ 2 trong command)
  records[0] = atoi((char *)paraAddr);
  findPara(len, 3, &paraAddr); // từ thằng para thứ 3 trở đi là tên lệnh on, off, DMT NXV, ...
  ret = myVR.trainWithSignature(records[0], paraAddr, 261, buf);
  //  ret = myVR.trainWithSignature(records, paraNum-1);
  if(ret >= 0) return printSigTrain(buf, ret);
  else Serial.println(F("Train with signature failed or timeout."));
  return 0;
}

void printLoad(uint8_t *buf, uint8_t len){
  if(len == 0){
    Serial.println(F("Load Successfully."));
    return;
  }else{
    Serial.print(F("Load success: "));
    Serial.println(buf[0], DEC);
  }
  for(int i=0; i<len-1; i += 2){
    Serial.print(F("Record "));
    Serial.print(buf[i+1], DEC);
    Serial.print(F("\t"));
    switch(buf[i+2]){
    case 0:
      Serial.println(F("Loaded"));
      break;
    case 0xFC:
      Serial.println(F("Record already in recognizer"));
      break;
    case 0xFD:
      Serial.println(F("Recognizer full"));
      break;
    case 0xFE:
      Serial.println(F("Record untrained"));
      break;
    case 0xFF:
      Serial.println(F("Value out of range"));
      break;
    default:
      Serial.println(F("Unknown status"));
      break;
    }
  }
}

int printSigTrain(uint8_t *buf, uint8_t len){
  int check = 0;
  if(len == 0){
    Serial.println(F("Train With Signature Finish."));
    return 0;
  }else{
    Serial.print(F("Success: "));
    Serial.println(buf[0], DEC);
    check = (int)buf[0];
  }
  Serial.print(F("Record "));
  Serial.print(buf[1], DEC);
  Serial.print(F("\t"));
  switch(buf[2]){
  case 0:
    Serial.println(F("Trained"));
    break;
  case 0xF0:
    Serial.println(F("Trained, signature truncate"));
    break;
  case 0xFE:
    Serial.println(F("Train Time Out"));
    break;
  case 0xFF:
    Serial.println(F("Value out of range"));
    break;
  default:
    Serial.print(F("Unknown status "));
    Serial.println(buf[2], HEX);
    break;
  }
  Serial.print(F("SIG: "));
  Serial.write(buf+3, len-3);
  Serial.println();
  return check;
}

void printSignature(uint8_t *buf, int len){
  int i;
  for(i=0; i<len; i++){
    if(buf[i]>0x19 && buf[i]<0x7F){
      Serial.write(buf[i]);
    }
    else{
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

void printVR(uint8_t *buf){
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if(buf[0] == 0xFF){
    Serial.print("NONE");
  }
  else if(buf[0]&0x80){
    Serial.print("UG ");
    Serial.print(buf[0]&(~0x80), DEC);
  }
  else{
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if(buf[3]>0){
    printSignature(buf+4, buf[3]);
  }
  else{
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}
