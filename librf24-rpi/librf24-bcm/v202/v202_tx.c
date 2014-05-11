#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include "RF24.h"
#include "v202_protocol.h"

#include "Raspduno.h"

#if 0
uint8_t txid[3] = { 0xcd, 0x31, 0x71 };
#elif 0
uint8_t txid[3] = { 0x3e, 0x6a, 0xaa };
#elif 0
uint8_t txid[3] = { 0xcd, 0x31, 0x72 };
#endif

#if 0
uint8_t rf_channels[16] = { 0x25, 0x2A, 0x1A, 0x3C, 0x37, 0x2B, 0x2E, 0x1D,
                            0x1B, 0x2D, 0x24, 0x3B, 0x13, 0x29, 0x23, 0x22 };
#elif 0
uint8_t rf_channels[16] = { 0x15, 0x1E, 0x39, 0x28, 0x2C, 0x1C, 0x29, 0x2E,
                            0x36, 0x2D, 0x18, 0x2B, 0x3A, 0x38, 0x1D, 0x1B };
#elif 1
uint8_t rf_channels[16] = { 0x2b, 0x1f, 0x3d, 0x2c, 0x28, 0x26, 0x32, 0x3a,
                            0x1d, 0x25, 0x2d, 0x18, 0x22, 0x16, 0x31, 0x1c };
#endif

SerialConsole Serial;
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);
v202Protocol protocol;

int counter = 0;
int direction = 1;
bool calibrated = false;

uint8_t throttle, flags;
int8_t yaw, pitch, roll;

int a0, a1, a2, a3;
int a0min, a0max;
int a1min, a1max;
int a2min, a2max;
int a3min, a3max;
bool bind = true;

bool readInput()
{
	static int v = 0;
#if 0
	static int d = 0;
	if (d == 0) {
		a0 = (v += 1);
	} else if (d == 1) {
		a0 = (v -= 1);
	} else {
		a0 = 0x10;
	}
#else
	if (bind) {
		a0 = 0;
	} else {
		if (v < 250) {
			a0 = 0xa0;
		} else if (v > 250 && v < 1000) {
			a0 = 0x80;
		} else if (v > 1000 && v < 2000) {
			a0 = 0x60;
		} else if (v > 2000 && v < 3000) {
			a0 = 0x30;
		} else if (v > 3000 && v < 4000) {
			a0 = 0x10;
		} else {
			a0 = 0x0;
		}
		v++;
	}
#endif
	a1 = 0x7f;
	a2 = 0x7f;
	a3 = 0x7f;
#if 0
	if (v >= 0xff) {
		d = 1;
	} else if (d == 1 && v < 1) {
		d = 2;
	}
#endif
	return true;
}

void setup() 
{
  uint8_t txid[3] = { 0xfe, 0xf7, 0xdd };
  protocol.init(&radio);
  protocol.setTXId(txid);  
  radio.txMode(8);
  throttle = 0; yaw = 0; pitch = 0; roll = 0; flags = 0;

  radio.printDetails();
  sleep(1);
} 

void loop() 
{
  bool changed = readInput();
  if (true) {
    Serial.write("sticks: ");
    Serial.print(a0); Serial.write(" ");
    Serial.print(a1); Serial.write(" ");
    Serial.print(a2); Serial.write(" ");
    Serial.print(a3); Serial.write("\n");
  }
  if (bind) {
    throttle = a0;
    flags = 0xc0;
#if 1
    /* Auto bind in 2.5 sec after turning on */
    counter += direction;
    if (direction > 0) {
      if (counter > 256) direction = -1;
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        bind = false;
        flags = 0;
        Serial.write("Bound\n");
      }
    } 
    /* */
#endif
    if (direction > 0) {
      if (throttle >= 255) direction = -1;
    } else {
      if (throttle == 0) {
        direction = 1;
        counter = 0;
        bind = false;
	sleep(1);
        flags = 0;
        Serial.write("Bound\n");
        Serial.write("a0min "); Serial.print(a0min);
        Serial.write(" a0max "); Serial.print(a0max);
        Serial.write("\na1min "); Serial.print(a1min);
        Serial.write(" a1max "); Serial.print(a1max);
        Serial.write("\na2min "); Serial.print(a2min);
        Serial.write(" a2max "); Serial.print(a2max);
        Serial.write("\na3min "); Serial.print(a3min);
        Serial.write(" a3max "); Serial.print(a3max);
      }
    }
  } else {
    throttle = a0;
    yaw = a1 < 0x80 ? 0x7f - a1 : a1;
    roll = a2 < 0x80 ? 0x7f - a2 : a2 ;
    pitch = a3 < 0x80 ? 0x7f - a3 : a3;

    // Blinking LED lights
    counter += direction;
    if (direction > 0) {
      if (counter > 255) {
        direction = -1;
        counter = 255;
        flags = 0x10;
      }
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        flags = 0x00;
      }
    }
    //
  }
  // flags = flags != 0xc0 ? 0x10 : 0xc0;
  protocol.command(throttle, yaw, pitch, roll, flags);
  delay(4);
}

void* ctlthread_proc(void* args)
{
	setup();
	while (1) loop();	
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	pthread_t pt;
	pthread_create(&pt, NULL, ctlthread_proc, NULL);
	while (1) {
		
	}
	return 0;
}
