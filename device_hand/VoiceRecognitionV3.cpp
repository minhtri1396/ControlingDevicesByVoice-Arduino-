#include "VoiceRecognitionV3.h"
#include <string.h>

VR* VR::instance;

/** temp data buffer */
uint8_t vr_buf[32];
uint8_t vr_bufCmd[32];
uint8_t hextab[17]="0123456789ABCDEF";

VR::VR(uint8_t receivePin, uint8_t transmitPin) : SoftwareSerial(receivePin, transmitPin){
	instance = this;
	SoftwareSerial::begin(38400);
}

int VR :: recognize(uint8_t *buf, int timeout){
  int ret, i;
  fflush(stdin);
  Serial.println((char *)vr_bufCmd);
  ret = receive_pkt(vr_bufCmd, timeout);
  if(vr_bufCmd[2] != FRAME_CMD_VR) return -1;
  if(ret > 0){
    for(i = 0; i < (vr_bufCmd[1] - 3); i++){
      buf[i] = vr_bufCmd[4+i];
    } 
    return i;
  }
  return 0;
}

int VR :: trainWithSignature(uint8_t record, const void *buf, uint8_t len, uint8_t * retbuf){
	int ret;
	unsigned long start_millis;
	if(len) send_pkt(FRAME_CMD_SIG_TRAIN, record, (uint8_t *)buf, len); // sigtrain 0 on => record = 0, buf = on, len = 2 (on dài 2 ký tự)
	else{
		if(buf == 0) return -1;
		len = strlen((char *)buf);
		if(len > 10) return -1;
		send_pkt(FRAME_CMD_SIG_TRAIN, record, (uint8_t *)buf, len);
	}
	start_millis = millis();
	while(1){
    //if(digitalRead(9)) return -2; // stop now
		ret = receive_pkt(vr_buf);
		if(ret>0){
			switch(vr_buf[2]){
				case FRAME_CMD_PROMPT:
					DBGSTR("Record:\t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR("\t");
					DBGBUF(vr_buf+4, ret-4); // trái tim cái này
          if((char)vr_buf[4] == 'S') ringRing(); // Speak now or Speak again
          else if((char)vr_buf[4] == 'C') ringRing(1, 1000); // Cann't matched
          // Speak again
          // Cann't matched
          // Speak now
          // Failed
          //for(int i = 1; i < ret - 2; ++i){
					break;
				case FRAME_CMD_SIG_TRAIN:
					if(retbuf != 0){
						memcpy(retbuf, vr_buf+3, vr_buf[1]-2);
						return vr_buf[1]-2;
					}
					DBGSTR("Train finish.\r\nSuccess: \t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR(" \r\n");
					writehex(vr_buf, vr_buf[1]+2);
					return 0;
			}
			start_millis = millis();
		}
		if(millis()-start_millis > 5000) return -2;
	}
	return 0;
}

int VR :: load(uint8_t *records, uint8_t len, uint8_t *buf){
	uint8_t ret;
	send_pkt(FRAME_CMD_LOAD, records, len);
	ret = receive_pkt(vr_buf);
	if(buf != 0){
		memcpy(buf, vr_buf+3, vr_buf[1]-2);
		return vr_buf[1]-2;
	}
	return 0;
}

int VR :: load(uint8_t record, uint8_t *buf){
  uint8_t ret;
  send_pkt(FRAME_CMD_LOAD, &record, 1);
  ret = receive_pkt(vr_buf);
  if(ret<=0) return -1;
  if(vr_buf[2] != FRAME_CMD_LOAD) return -1;
  if(buf != 0){
    memcpy(buf, vr_buf+3, vr_buf[1]-2);
    return vr_buf[1]-2;
  }
  return 0;
}

int VR :: clear(){	
	int len;
	send_pkt(FRAME_CMD_CLEAR, 0, 0);
	len = receive_pkt(vr_buf);
	if(len<=0) return -1;
	if(vr_buf[2] != FRAME_CMD_CLEAR) return -1;
	return 0;
}

int VR :: writehex(uint8_t *buf, uint8_t len){
	int i;
	for(i=0; i<len; i++){
		DBGCHAR(hextab[(buf[i]&0xF0)>>4]);
		DBGCHAR(hextab[(buf[i]&0x0F)]);
		DBGCHAR(' ');
	}
	return len;
}

void VR :: send_pkt(uint8_t cmd, uint8_t subcmd, uint8_t *buf, uint8_t len){
	while(available()) flush();
	write(FRAME_HEAD);
	write(len+3); // len của buf + thêm cmd, FRAME_HEAD và subcmd nên là len + 3 (không tính FRAME_END)
	write(cmd);
	write(subcmd);
	write(buf, len);
	write(FRAME_END);
}

void VR :: send_pkt(uint8_t cmd, uint8_t *buf, uint8_t len){
	while(available()) flush();
	write(FRAME_HEAD);
	write(len+2); // len của buf + thêm cmd và FRAME_HEAD nên là len + 2 (không tính FRAME_END)
	write(cmd);
	write(buf, len);
	write(FRAME_END);
}

int VR :: receive_pkt(uint8_t *buf, uint16_t timeout){ // timeout mặc định trong .h là 1000 ms = 1s
	int ret;
	ret = receive(buf, 2, timeout);
	if(ret != 2) return -1; // do chỉ lấy 2 phần tử đầu trong mảng buf
	if(buf[0] != FRAME_HEAD) return -2; // phần tử đầu mảng luôn là FRAME_HEAD, cuối mảng luôn là FRAME_END
	if(buf[1] < 2) return -3; // chiều dài len, vd: sigtrain 0 a, thì a là len = 1, mà tối thiểu là len + 1 = 2, nên len luôn phải >= 2
	ret = receive(buf+2, buf[1], timeout); // buf + 2 do đã đọc FRAME_HEAD và len; buf[1] là len đã cộng
	if(buf[buf[1]+1] != FRAME_END) return -4;
	return buf[1]+2;
}

int VR::receive(uint8_t *buf, int len, uint16_t timeout){
  int read_bytes = 0;
  int ret;
  unsigned long start_millis;
  while (read_bytes < len){
    start_millis = millis();
    do{
      ret = read();
      if (ret >= 0) break;
    }while((millis()- start_millis ) < timeout);
    if (ret < 0) return read_bytes; // đã đọc hết ký tự
    buf[read_bytes] = (char)ret;
    read_bytes++;
  }
  return read_bytes; // đã lấy đủ số byte mong đợi (có thể chưa đọc hết ký tự)
}
