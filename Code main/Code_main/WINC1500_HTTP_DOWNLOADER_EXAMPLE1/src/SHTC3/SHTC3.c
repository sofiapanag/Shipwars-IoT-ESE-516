/*
 * CFile1.c
 *
 * Created: 4/30/2022 12:44:17 PM
 *  Author: Admin
 */ 

#include "SHTC3.h"
#include "I2cDriver\I2cDriver.h"

#define SHTC3_ADDRESS 0x70
#define SHTC3_TIMEOUT 10
#define SHTC3_delay 210/portTICK_PERIOD_MS
#define TIMEOUT_POLL 2000
#define WAKE_WRITE_BUFFER_LEN 2
#define MEAS_WRITE_BUFFER_LEN 2
#define SLEEP_WRITE_BUFFER_LEN 2
#define READ_BUFFER_LEN 6

static uint8_t wake_write_buffer[WAKE_WRITE_BUFFER_LEN] = {
	0x35 , 0x17
};

static uint8_t meas_write_buffer[MEAS_WRITE_BUFFER_LEN] = {
	0x58 , 0xE0
};

static uint8_t sleep_write_buffer[SLEEP_WRITE_BUFFER_LEN] = {
	0xB0 , 0x98
};

uint8_t read_buffer[READ_BUFFER_LEN];

I2C_Data shtcData = {
	.address = SHTC3_ADDRESS
};


static int32_t shtc_write(uint8_t *bufp, uint16_t len)
{
	shtcData.lenOut = len;
	shtcData.msgOut = bufp;
	shtcData.msgIn = NULL;
	shtcData.lenIn = 0;
	return I2cWriteDataWait(&shtcData, SHTC3_TIMEOUT);
} 

static int32_t shtc_read(uint8_t *bufp, uint16_t len)
{
	shtcData.lenOut = len;
	shtcData.msgOut = bufp;
	shtcData.msgIn = read_buffer;
	shtcData.lenIn = READ_BUFFER_LEN;
	return I2cReadDataWait(&shtcData,SHTC3_delay, SHTC3_TIMEOUT);
}


bool shtc_get(uint16_t *buf){
	int32_t err_ = ERROR_NONE;
	
	err_ = shtc_write(wake_write_buffer, WAKE_WRITE_BUFFER_LEN);
	if(err_ != ERROR_NONE) {goto exit_error;}
		
	err_ = shtc_read(meas_write_buffer, MEAS_WRITE_BUFFER_LEN);
	if(err_ != ERROR_NONE) {goto exit_error;}
		
	shtc_write(sleep_write_buffer, SLEEP_WRITE_BUFFER_LEN);
	
	buf[0] = (read_buffer[0] << 8) | read_buffer[1] ;
	buf[1] = (read_buffer[3] << 8) | read_buffer[4] ;
		
	buf[0] = 100 * buf[0]/65536;
	buf[1] = -45 +175 * buf[1]/65536;
	
	return true;
	
	exit_error:
		buf[0] = 0;
		buf[1] = 0;
		return false;
}

