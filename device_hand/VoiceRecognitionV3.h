#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "wiring_private.h"

#include "SoftwareSerial.h"
#include <avr/pgmspace.h>

#define DEBUG

#ifdef DEBUG
#define DBGSTR(message)     Serial.print(message)
#define DBGBUF(buf, len)	Serial.write(buf, len)
#define DBGLN(message)		Serial.println(message)
#define DBGFMT(msg, fmt)	Serial.print(msg, fmt)
#define DBGCHAR(c)			Serial.write(c)
#else
#define DBG(message)
#endif // DEBUG

#define VR_DEFAULT_TIMEOUT						(1000)

/***************************************************************************/
#define FRAME_HEAD							(0xAA)
#define FRAME_END							(0x0A)

/***************************************************************************/
#define FRAME_CMD_CHECK_SYSTEM				(0x00)
#define FRAME_CMD_CHECK_BSR					(0x01)
#define FRAME_CMD_CHECK_TRAIN				(0x02)
#define FRAME_CMD_CHECK_SIG					(0x03)

#define FRAME_CMD_RESET_DEFAULT				(0x10)	//reset configuration
#define FRAME_CMD_SET_BR					(0x11)	//baud rate
#define FRAME_CMD_SET_IOM					(0x12)	//IO mode
#define FRAME_CMD_SET_PW					(0x13)	//pulse width
#define FRAME_CMD_RESET_IO					(0x14)	// reset IO OUTPUT					
#define FRAME_CMD_SET_AL					(0x15)	// Auto load

#define FRAME_CMD_TRAIN						(0x20)
#define FRAME_CMD_SIG_TRAIN					(0x21) // 33
#define FRAME_CMD_SET_SIG					(0x22)

#define FRAME_CMD_LOAD						(0x30)	//Load N records
#define FRAME_CMD_CLEAR						(0x31)	//Clear BSR buffer					
#define FRAME_CMD_GROUP						(0x32)  //
	#define FRAME_CMD_GROUP_SET							(0x00)  //
	#define FRAME_CMD_GROUP_SUGRP						(0x01)  //
	#define FRAME_CMD_GROUP_LSGRP						(0x02)  //
	#define FRAME_CMD_GROUP_LUGRP						(0x03)  //
	#define FRAME_CMD_GROUP_CUGRP						(0x04)  //

#define FRAME_CMD_TEST						(0xEE)
	#define FRAME_CMD_TEST_READ							(0x01)	
	#define FRAME_CMD_TEST_WRITE						(0x00)	


#define FRAME_CMD_VR						(0x0D)	//Voice recognized
#define FRAME_CMD_PROMPT					(0x0A)
#define FRAME_CMD_ERROR						(0xFF)

/***************************************************************************/
// #define FRAME_ERR_UDCMD						(0x00)
// #define FRAME_ERR_LEN						(0x01)
// #define FRAME_ERR_DATA						(0x02)
// #define FRAME_ERR_SUBCMD					(0x03)

// //#define FRAME_ERR_
// #define FRAME_STA_SUCCESS					(0x00)
// #define FRAME_STA_FAILED					(0xFF)
/***************************************************************************/

class VR : public SoftwareSerial{
public:
	VR(uint8_t receivePin, uint8_t transmitPin);
  short ring = 10;
  // Ring ring
  void ringRing(const short& num = 1, const short& _delayset = 125, const short& _delay = 1){
    for(short i = 0; i < num; ++i){
      digitalWrite(ring, HIGH);
      delay(_delayset);
      digitalWrite(ring, LOW);
      delay(_delay);
    }
  }
	static VR* getInstance() {
	   return instance;
	}
	
	typedef enum{
		PULSE = 0,
		TOGGLE = 1,
		SET = 2, 
		CLEAR = 3
	}io_mode_t;
	
	typedef enum{
		LEVEL0 = 0,
		LEVEL1,
		LEVEL2, 
		LEVEL3,
		LEVEL4,
		LEVEL5,
		LEVEL6,
		LEVEL7,
		LEVEL8,
		LEVEL9,
		LEVEL10,
		LEVEL11,
		LEVEL12,
		LEVEL13,
		LEVEL14,
		LEVEL15,
	}pulse_width_level_t;
	
	typedef enum{
		GROUP0 = 0,
		GROUP1,
		GROUP2,
		GROUP3,
		GROUP4,
		GROUP5,
		GROUP6,
		GROUP7,
		GROUP_ALL = 0xFF,
	}group_t;

  int recognize(uint8_t *buf, int timeout = 5000);
	int trainWithSignature(uint8_t record, const void *buf, uint8_t len=0, uint8_t *retbuf = 0);
	int load(uint8_t *records, uint8_t len=1, uint8_t *buf = 0);
  int load(uint8_t record, uint8_t *buf = 0);
	int clear(); 
	
	int writehex(uint8_t *buf, uint8_t len);
	
/***************************************************************************/
	/** low level */
	void send_pkt(uint8_t cmd, uint8_t *buf, uint8_t len);
	void send_pkt(uint8_t cmd, uint8_t subcmd, uint8_t *buf, uint8_t len);
	int receive(uint8_t *buf, int len, uint16_t timeout = VR_DEFAULT_TIMEOUT);
	int receive_pkt(uint8_t *buf, uint16_t timeout = VR_DEFAULT_TIMEOUT);
/***************************************************************************/
private:
	static VR*  instance;
};

