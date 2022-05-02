/**************************************************************************/ /**
 * @file      ControlThread.c
 * @brief     Thread code for the ESE516 Online game control thread
 * @author    You!
 * @date      2020-04-015

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ControlThread/ControlThread.h"

#include <errno.h>

#include "SerialConsole.h"
#include "UiHandlerThread/UiHandlerThread.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "main.h"
#include "shtc3.h"
#include "stdio_serial.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/

controlStateMachine_state controlState;  ///< Holds the current state of the control thread
uint8_t ship_arr[MAX_SHIP];
uint8_t ship_num;
bool placement_status = false;

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * Task Functions
 ******************************************************************************/

/**
 * @fn		void vControlHandlerTask( void *pvParameters )
 * @brief	STUDENT TO FILL THIS
 * @details 	student to fill this

 * @param[in]	Parameters passed when task is initialized. In this case we can ignore them!
 * @return		Should not return! This is a task defining function.
 * @note
 */
void vControlHandlerTask(void *pvParameters)
{
    SerialConsoleWriteString((char *)"ESE516 - Control Init Code\r\n");

    controlState = CONTROL_WAIT_FOR_GAME;  // Initial state
	
    while (1) {
        switch (controlState) {
            case (CONTROL_WAIT_FOR_GAME): { 
                break;
            }

            case (CONTROL_WAIT_FOR_PLACE): {  
				if(placement_status == true){
				// and ship loc to queue and pub
				}
                break;
            }


            default:
                controlState = CONTROL_WAIT_FOR_GAME;
                break;
        }
        vTaskDelay(40);
    }
}



void ControlSetGame(uint8_t *shiparr_in,uint8_t ship_num_in)
{
	memcpy (ship_arr, shiparr_in, ship_num_in * sizeof (uint8_t));
	ship_num = ship_num_in;
	placement_status = false;
	controlState = CONTROL_WAIT_FOR_PLACE;
	LogMessage(LOG_DEBUG_LVL, "\r\nship_arr %d %d %d\r\n", ship_arr[0], ship_arr[1], ship_arr[2]);
	UiPlaceInit(ship_arr, ship_num);
}


void SetPlacementStatus(bool state){
	placement_status = state;
}