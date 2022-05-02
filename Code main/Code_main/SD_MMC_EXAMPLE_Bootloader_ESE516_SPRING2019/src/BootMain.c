/**************************************************************************//**
* @file      BootMain.c
* @brief     Main file for the ESE516 bootloader. Handles updating the main application
* @details   Main file for the ESE516 bootloader. Handles updating the main application
* @author    Eduardo Garcia
* @date      2020-02-15
* @version   2.0
* @copyright Copyright University of Pennsylvania
******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "sd_mmc_spi.h"

#include "SD Card/SdCard.h"
#include "Systick/Systick.h"
#include "SerialConsole/SerialConsole.h"
#include "ASF/sam0/drivers/dsu/crc32/crc32.h"





/******************************************************************************
* Defines
******************************************************************************/
#define APP_START_ADDRESS  ((uint32_t)0x12000) ///<Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04) ///< Main application reset vector address
#define PAGE_PER_ROW 4
#define BOOTLOADER_ROW_NUM 288 //< number of rows the bootloader takes (0x00000 to 0x12000 = 288 rows)
#define PAGE_SIZE 64 //bytes
#define ROW_SIZE 256 //bytes

/******************************************************************************
* Structures and Enumerations
******************************************************************************/

struct usart_module cdc_uart_module; ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
* Local Function Declaration
******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);
static bool DeleteMainProgram(void);
static bool WriteMainProgram(char* binary_file_name);


/******************************************************************************
* Global Variables
******************************************************************************/
//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";	///<Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";	///<Test BINARY File name
char update_flag_name[] = "0:update.txt";	///<TestA FLAG File name
char fw_bin_file[] = "0:fw.bin";	///<TestB FLAG File name
char helpStr[100]; ///<help print string


Ctrl_status status; ///<Holds the status of a system initialization
FRESULT res; //Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs; //Holds the File System of the SD CARD
FIL file_object; //FILE OBJECT used on main for the SD Card Test



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn		int main(void)
* @brief	Main function for ESE516 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void)
{

	/*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	/* Initialize SD MMC stack */
	sd_mmc_init();

	//Initialize the NVM driver
	configure_nvm();

	irq_initialize_vectors();
	cpu_irq_enable();

	//Configure CRC32
	dsu_crc32_init();

	SerialConsoleWriteString("ESE516 - ENTER BOOTLOADER");	//Order to add string to TX Buffer

	/*END SYSTEM PERIPHERALS INITIALIZATION*/


	/*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

	//EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
	//See function inside to see how to open a file
	
	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");
	
	SerialConsoleWriteString("skip sd card");
	goto exit_bootloader;

	if(StartFilesystemAndTest() == false)
	{
		SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
		delay_cycles_ms(5000);
		system_reset();
	}
	else
	{
		SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
	}

	/*END SIMPLE SD CARD MOUNTING AND TEST!*/


	/*3.) STARTS BOOTLOADER HERE!*/
	update_flag_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	fw_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';

	res = f_open(&file_object, (char const *)update_flag_name, FA_READ);
	f_close(&file_object);
	if(res == FR_OK){
		SerialConsoleWriteString("Found update flag - attempting firmware update\r\n");
		if(!DeleteMainProgram()){goto fw_update_error;}
		if(!WriteMainProgram(fw_bin_file)){goto fw_update_error;}
		f_unlink(update_flag_name);
		SerialConsoleWriteString("Firmware updated!\r\n");
	}

	exit_bootloader:
	//4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
	SerialConsoleWriteString("ESE516 - EXIT BOOTLOADER");	//Order to add string to TX Buffer
	delay_cycles_ms(100); //Delay to allow print
		
	//Deinitialize HW - deinitialize started HW here!
	DeinitializeSerialConsole(); //Deinitializes UART
	
	sd_mmc_deinit(); //Deinitialize SD CARD


	//Jump to application
	jumpToApplication();

	//Should not reach here! The device should have jumped to the main FW.
	
	fw_update_error:
	SerialConsoleWriteString("FW update error... please restart \r\n");
	
}







/******************************************************************************
* Static Functions
******************************************************************************/

static bool DeleteMainProgram(void)
{
	struct nvm_parameters parameters;
	nvm_get_parameters (&parameters); //Get NVM parameters
	snprintf(helpStr, 63,"NVM Info: Number of Pages %d. Size of a page: %d bytes. \r\n", parameters.nvm_number_of_pages, parameters.page_size);
	SerialConsoleWriteString(helpStr);
	for(int i = 0; i < (parameters.nvm_number_of_pages / PAGE_PER_ROW) - BOOTLOADER_ROW_NUM; i++){
		enum status_code nvmError = nvm_erase_row((APP_START_ADDRESS + i * ROW_SIZE));
		
		if(nvmError != STATUS_OK)
		{
			snprintf(helpStr, 63,"NVM ERROR: Erase error at row %d \r\n", i + BOOTLOADER_ROW_NUM);
			SerialConsoleWriteString(helpStr);
			return false;
		}
	}
	return true;
	
	
}

static bool WriteMainProgram(char* binary_file_name)
{
	uint32_t resultCrcSd = 0;
	uint32_t resultCrcNVM = 0;
	uint8_t readBuffer[ROW_SIZE];
	FIL file_object_bin;
	struct nvm_parameters parameters;
	nvm_get_parameters (&parameters); //Get NVM parameters
	
	int i =0;
	UINT numBytesRead = ROW_SIZE;
	
	res = f_open(&file_object_bin, binary_file_name, FA_READ);
	if(res != FR_OK){
		SerialConsoleWriteString("could not find bin file \r\n");
		return false;
	}
	

	while(numBytesRead == ROW_SIZE){
		res = f_read(&file_object_bin, &readBuffer[0],  ROW_SIZE, &numBytesRead);
		if(res != FR_OK){
			SerialConsoleWriteString("read error \r\n");
			f_close(&file_object_bin);
			return false;
		}

		int cur_address = APP_START_ADDRESS + (i*ROW_SIZE);
		
		//pad last data with 0xFF
		if(numBytesRead < ROW_SIZE){
			for(int iter = numBytesRead; iter < ROW_SIZE; iter++){
				readBuffer[iter] = 0xFF;
			}
		}

		res = nvm_write_buffer(APP_START_ADDRESS + (i*ROW_SIZE), &readBuffer[0], PAGE_SIZE);
		res = nvm_write_buffer(APP_START_ADDRESS + (i*ROW_SIZE) + 64, &readBuffer[64], PAGE_SIZE);
		res = nvm_write_buffer(APP_START_ADDRESS +(i*ROW_SIZE) + 128, &readBuffer[128], PAGE_SIZE);
		res = nvm_write_buffer(APP_START_ADDRESS + (i*ROW_SIZE) + 192, &readBuffer[192], PAGE_SIZE);
		
		//calculate SD CRC chunck ->  See http://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D21-Family-Silicon-Errata-and-DataSheet-Clarification-DS80000760D.pdf Section 1.8.3
		*((volatile unsigned int*) 0x41007058) &= ~0x30000UL;
		dsu_crc32_cal(readBuffer, ROW_SIZE, &resultCrcSd);
		*((volatile unsigned int*) 0x41007058) |= 0x20000UL;
		
		
		//calculate NVM CRC chunck
		dsu_crc32_cal(APP_START_ADDRESS + (i*ROW_SIZE), ROW_SIZE, &resultCrcNVM);
		
		i++;
	}
	
	if(resultCrcSd != resultCrcNVM){
		SerialConsoleWriteString("CRC ERROR\r\n");
		snprintf(helpStr, 63,"CRC SD CARD: %d  CRC NVM: %d \r\n", resultCrcSd, resultCrcNVM);
		SerialConsoleWriteString(helpStr);
		f_close(&file_object_bin);
		return false;
	}
	
	f_close(&file_object_bin);
	return true;
	
	
}



/**************************************************************************//**
* function      static void StartFilesystemAndTest()
* @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       Returns true is SD card and file system test passed. False otherwise.
******************************************************************************/
static bool StartFilesystemAndTest(void)
{
	bool sdCardPass = true;
	uint8_t binbuff[256];

	//Before we begin - fill buffer for binary write test
	//Fill binbuff with values 0x00 - 0xFF
	for(int i = 0; i < 256; i++)
	{
		binbuff[i] = i;
	}

	//MOUNT SD CARD
	Ctrl_status sdStatus= SdCard_Initiate();
	
	
	if(sdStatus == CTRL_GOOD) //If the SD card is good we continue mounting the system!
	{
		SerialConsoleWriteString("SD Card initiated correctly!\n\r");

		//Attempt to mount a FAT file system on the SD Card using FATFS
		SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs); //Order FATFS Mount
		if (FR_INVALID_DRIVE == res)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}
		SerialConsoleWriteString("[OK]\r\n");

		//Create and open a file
		SerialConsoleWriteString("Create a file (f_open)...\r\n");

		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object,
		(char const *)test_file_name,
		FA_CREATE_ALWAYS | FA_WRITE);
		
		if (res != FR_OK)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");

		//Write to a file
		SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

		if (0 == f_puts("Test SD/MMC stack\n", &file_object))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");


		//Write binary file
		//Read SD Card File
		test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);
		
		if (res != FR_OK)
		{
			SerialConsoleWriteString("Could not open binary file!\r\n");
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		//Write to a binaryfile
		SerialConsoleWriteString("Write to test file (f_write)...\r\n");
		uint32_t varWrite = 0;
		if (0 != f_write(&file_object, binbuff,256, &varWrite))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");
		
		main_end_of_test:
		SerialConsoleWriteString("End of Test.\n\r");

	}
	else
	{
		SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
		sdCardPass = false;
	}

	return sdCardPass;
}



/**************************************************************************//**
* function      static void jumpToApplication(void)
* @brief        Jumps to main application
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       
******************************************************************************/
static void jumpToApplication(void)
{
// Function pointer to application section
void (*applicationCodeEntry)(void);

// Rebase stack pointer
__set_MSP(*(uint32_t *) APP_START_ADDRESS);

// Rebase vector table
SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

// Set pointer to application section
applicationCodeEntry =
(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

// Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry, 
//which should be the start of the main FW.
applicationCodeEntry();
}



/**************************************************************************//**
* function      static void configure_nvm(void)
* @brief        Configures the NVM driver
* @details      
* @return       
******************************************************************************/
static void configure_nvm(void)
{
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}



