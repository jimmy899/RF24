#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>

#include "v202_protocol.h"
#include "Raspduno.h"

RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);
v202Protocol protocol;

rx_values_t rxValues;

bool bind_in_progress = false;
unsigned long newTime;

void setup(void)
{
	protocol.init(&radio);
#if 0
	/* for Debug */
	uint8_t val;
	val = radio.read_register(0x0);
	printf("[0x0] = %08x\n", val);
	val = radio.read_register(0x6);
	printf("[0x6] = %08x\n", val);
#endif

	radio.printDetails();
	/* delay before loop start */
	sleep(1);

	// t = micros();
	// printf("start\n");
}

void loop(void)
{
	// t = micros();
	uint8_t value = protocol.run(&rxValues);
	newTime = micros();

	switch (value) {
		case BIND_IN_PROGRESS:
			if (!bind_in_progress) 
			{
				bind_in_progress = true;
				// printf("Bind in Progress");
			}
			break;
		case BOUND_NEW_VALUES:
			// printf("%s:%d -\n", __FILE__, __LINE__);
			printf("%d, t: %d, p: %d, y: %d, r: %d\n", value, rxValues.throttle, rxValues.pitch, rxValues.yaw, rxValues.roll);
			 break;
		case BOUND_NO_VALUES:
			// printf("%s:%d -\n", __FILE__, __LINE__);
			// printf("%d, t: %d, p: %d, y: %d, r: %d\n", value, rxValues.throttle, rxValues.pitch, rxValues.yaw, rxValues.roll);
			break;
		default:
			// printf("%s:%d value: %d\n", __FILE__, __LINE__, value);
			break;
	}
	delay(2);
	// printf("%s:%d\n", __FILE__, __LINE__);
}

// Simulate Arduino 
int main(int argc, char* argv[])
{
	setup();
	while (1) loop();	
	return 0;
}

