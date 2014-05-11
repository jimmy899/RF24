/* v202_protocol.cpp -- Handle the v202 protocol.
 *
 * Copyright (C) 2014 Alexandre Clienti
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 * 
 * this file is copied from the project 'v202-receiver' from github,
 * and edited to make it works for me.
 * the project repository URL is https://github.com/execuc/v202-receiver.
 */

#include "v202_protocol.h"

/*****************************************************************/
/*	
    This portion of code is extracted from https://bitbucket.org/rivig/v202/src
    Thanks to rivig to his discovery and to share the v202 protocol.
*/

static uint8_t freq_hopping[][16] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

void v202Protocol::retrieveFrequency()
{
  uint8_t sum;
  sum = mTxid[0] + mTxid[1] + mTxid[2];
  // Base row is defined by lowest 2 bits
  uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
  // Higher 3 bits define increment to corresponding row
  uint8_t increment = (sum & 0x1e) >> 2;
  for (int i = 0; i < 16; ++i) {
    uint8_t val = fh_row[i] + increment;
    // Strange avoidance of channels divisible by 16
    mRfChannels[i] = (val & 0x0f) ? val : val - 3;
  }
}
/*****************************************************************/

// This array stores 8 frequencies to test for the bind process
static uint8_t freq_test[8];

v202Protocol::v202Protocol() : packet_sent(false)
{
  mTxid[0]=0;
  mTxid[1]=0;
  mTxid[2]=0;
  
  // Populate frequency for binding
  // 0x18 channel is present for the 4 frequencies hopping pattern
  // So create the 8 increments possible which is a combination
  // of the 3 higher bits of tx id.
  for(uint8_t i = 0; i < 8; ++i)
  {
    uint8_t freq = 0x18 + (i << 3);
    freq_test[i] = (freq & 0x0F) ? freq : freq - 3;
  }
  
  mState = NO_BIND;
  mLastSignalTime = 0;
  mRfChNum = 0;
}

v202Protocol::~v202Protocol()
{
  /*EMPTY*/
}

bool v202Protocol::checkCRC()
{  
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 15; ++i)
    sum+= mFrame[i];
  return (sum == mFrame[15]);
}

void v202Protocol::init(RF24 *wireless)
{
  mWireless = wireless;
  mWireless->init(16);
  delayMicroseconds(100);
  mWireless->rxMode(freq_test[0]);
  mLastSignalTime = millis();
}

// loop function, can be factorized (for later)
uint8_t v202Protocol::run( rx_values_t *rx_value )
{
  uint8_t returnValue = UNKNOWN;
  switch(mState)
  {
    case BOUND:
    {
      returnValue = BOUND_NO_VALUES;
      if( mWireless->rxFlag() )
      {
		printf("%s:%d -\n", __FILE__, __LINE__);
        bool incrementChannel = false;
        mWireless->resetRxFlag();
        while ( !mWireless->rxEmpty() )
        {
          mWireless->readPayload(mFrame, 16);
          if( checkCRC() && checkTXaddr() )
          {
		printf("%s:%d -\n", __FILE__, __LINE__);
            // a valid frame has been received
            incrementChannel = true;
            // Discard bind frame
		int k;
		for (k = 0; k < 16; k++) {
			printf("%02x ", mFrame[k]);
		}
		printf("\n");
            if( mFrame[14] != 0xc0 )
            {
		printf("%s:%d -\n", __FILE__, __LINE__);
              // Extract values
              returnValue = BOUND_NEW_VALUES;
              rx_value->throttle = mFrame[0];
              rx_value->yaw = mFrame[1] < 0x80 ? -mFrame[1] :  mFrame[1] - 0x80;
              rx_value->pitch = mFrame[2] < 0x80 ? -mFrame[2] :  mFrame[2] - 0x80;
              rx_value->roll = mFrame[3] < 0x80 ? -mFrame[3] :  mFrame[3] - 0x80;
              rx_value->trim_yaw = mFrame[4] - 0x40;
              rx_value->trim_pitch = mFrame[5] - 0x40;
              rx_value->trim_roll = mFrame[6] - 0x40;
              rx_value->flags = mFrame[14];
#if 0
		printf("%s:%d t: %d, p: %d, y: %d, r: %d\n", __FILE__, __LINE__,
			rx_value->throttle, rx_value->pitch, rx_value->yaw, rx_value->roll
		);
#endif
            }
          }
        }
        
        if(incrementChannel)
        {
          mRfChNum++;
          if( mRfChNum > 15) 
            mRfChNum = 0;
          mWireless->switchFreq(mRfChannels[mRfChNum]);
        }
      }
    }
    break;
    // Initial state
    case NO_BIND:
    {
		// printf("%s:%d -\n", __FILE__, __LINE__);
      returnValue = NOT_BOUND;
      unsigned long newTime = millis();
 
      if( !mWireless->rxFlag() )
      {
		// printf("%s:%d -\n", __FILE__, __LINE__);
        // Wait 128ms before switching the frequency
        // 128ms is th time to a TX to emits on all this frequency
        if((newTime - mLastSignalTime) > 128)
        {
          mRfChNum++;
          if( mRfChNum > 7) 
            mRfChNum = 0;
          mWireless->switchFreq(freq_test[mRfChNum]);
          mLastSignalTime = newTime;
        }
		// printf("%s:%d -\n", __FILE__, __LINE__);
      }
      else 
      {
		printf("%s:%d -\n", __FILE__, __LINE__);
        mWireless->resetRxFlag();
        bool bFrameOk = false;
	printf("%s:%d -\n", __FILE__, __LINE__);
        while ( !mWireless->rxEmpty() )
        {
		printf("%s:%d -\n", __FILE__, __LINE__);
          mWireless->readPayload(mFrame, 16);
                           
		int k;
		for (k = 0; k < 16; k++) {
			printf("%02x ", mFrame[k]);
		}
		printf("\n");
          if( checkCRC() && mFrame[14] == 0xC0 )
          {
            // Bind frame is OK
            mTxid[0] = mFrame[7];
            mTxid[1] = mFrame[8];
            mTxid[2] = mFrame[9];
            // Create TX frequency array
            retrieveFrequency();
            mRfChNum = 0;
            mWireless->switchFreq(mRfChannels[mRfChNum]);
            mLastSignalTime = newTime;  
            mState = WAIT_FIRST_SYNCHRO;
            mWireless->flushRx();
            returnValue = BIND_IN_PROGRESS;
            break;
          }
        }
      }
    }
    break;
    
    // Wait on the first frequency of TX
    case WAIT_FIRST_SYNCHRO:
      returnValue = BIND_IN_PROGRESS;
      if( mWireless->rxFlag() )
      {
        mWireless->resetRxFlag();
        bool incrementChannel = false;
        while ( !mWireless->rxEmpty() )
        {
          mWireless->readPayload(mFrame, 16);
		printf("%s:%d -\n", __FILE__, __LINE__);
		int k;
		for (k = 0; k < 16; k++) {
			printf("%02x ", mFrame[k]);
		}
		printf("\n");
          if( checkCRC() )
          {
            incrementChannel = true;
            mState = BOUND;
          }
        }
        
        if(incrementChannel)
        {
          // switch channel
          mRfChNum++;
          if( mRfChNum > 15) 
            mRfChNum = 0;
          mWireless->switchFreq(mRfChannels[mRfChNum]);
        }
      }
    break;
    // Not implement for the moment
    case SIGNAL_LOST:
      returnValue = BIND_IN_PROGRESS;
    break;
    
    default:
    break;
  }
  
  return returnValue;
}

void v202Protocol::setTXId(uint8_t* txId)
{
  uint8_t sum;
  int i;
  for (i = 0; i < 3; i++) {
    mTxid[i] = txId[i];
  }

  // mRfChannels, mRfChNum

  sum = mTxid[0] + mTxid[1] + mTxid[2];
  // Base row is defined by lowest 2 bits
  uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
  // Higher 3 bits define increment to corresponding row
  uint8_t increment = (sum & 0x1e) >> 2;
  for (int i = 0; i < 16; ++i) {
    uint8_t val = fh_row[i] + increment;
    // Strange avoidance of channels divisible by 16
    mRfChannels[i] = (val & 0x0f) ? val : val - 3;
  }
}

void v202Protocol::command(uint8_t throttle, uint8_t pitch, uint8_t yaw, uint8_t roll, uint8_t flags)
{
  uint8_t buf[16];
  if (flags == 0xc0) {
    // binding
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
  } else {
    // regular packet
    buf[0] = throttle;
    buf[1] = (uint8_t) yaw;
    buf[2] = (uint8_t) pitch;
    buf[3] = (uint8_t) roll;
    // Trims, middle is 0x40
    buf[4] = 0x40; // yaw
    buf[5] = 0x40; // pitch
    buf[6] = 0x40; // roll
  }
  // TX id
  buf[7] = mTxid[0];
  buf[8] = mTxid[1];
  buf[9] = mTxid[2];
  // empty
  buf[10] = 0x00;
  buf[11] = 0x00;
  buf[12] = 0x00;
  buf[13] = 0x00;
  //
  buf[14] = flags;
  uint8_t sum = 0;
  uint8_t i;
  for (i = 0; i < 15;  ++i) sum += buf[i];
  buf[15] = sum;
  if (packet_sent) {
    bool report_done = false;
//    if  (!(mWireless->read_register(STATUS) & _BV(TX_DS))) { Serial.write("Waiting for radio\n"); report_done = true; }
    while (!(mWireless->txFlag()));
    mWireless->resetTxFlag();
//    if (report_done) Serial.write("Done\n");
  }
  packet_sent = true;
#if 0
  // Each packet is repeated twice on the same
  // channel, hence >> 1
  // We're not strictly repeating them, rather we
  // send new packet on the same frequency, so the
  // receiver gets the freshest command. As receiver
  // hops to a new frequency as soon as valid packet
  // received it does not matter that the packet is
  // not the same one repeated twice - nobody checks this
  uint8_t rf_ch = rf_channels[rf_ch_num >> 1];
  rf_ch_num++; if (rf_ch_num >= 32) rf_ch_num = 0;
//  Serial.print(rf_ch); Serial.write("\n");
  // mWireless->write_register(RF_CH, rf_ch);
  mWireless->switchFreq(rf_ch);
#else
  mRfChNum++;
    if( mRfChNum > 31) 
      mRfChNum = 0;
  mWireless->switchFreq(mRfChannels[mRfChNum >> 1]);
#endif
  mWireless->flush_tx();
  int k;
  for (k = 0; k < 16; k++)
    printf("%02x ", buf[k]);
  printf("\n");
  mWireless->write_payload(buf, 16);
  //mWireless->ce(HIGH);
  delayMicroseconds(15);
  //mWireless->ce(LOW);
}

