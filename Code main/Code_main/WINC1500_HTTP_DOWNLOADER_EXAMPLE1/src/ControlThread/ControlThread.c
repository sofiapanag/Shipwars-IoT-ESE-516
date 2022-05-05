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
bool is_fired = false;

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
					controlState = CONTROL_WAIT_FOR_TURN;
				}
                break;
            }
			case (CONTROL_WAIT_FOR_TURN): {
				break;
			}
			case (CONTROL_WAIT_FOR_ACTION): {
				if(is_fired == true){
					controlState = CONTROL_WAIT_FOR_TURN;
					is_fired = false;
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
	is_fired = false;
	controlState = CONTROL_WAIT_FOR_PLACE;
	LogMessage(LOG_DEBUG_LVL, "\r\nship_arr %d %d %d\r\n", ship_arr[0], ship_arr[1], ship_arr[2]);
	UiPlaceInit(ship_arr, ship_num);
}


void SetPlacementStatus(bool state){
	placement_status = state;
}

void SetFireStatus(void){
	is_fired = true;
}

void ControlTurnArray(uint8_t *shiparr_in) {
// [winner(0/1/2), new_turn(1,2), result(0,1), board_to_check(1,2), loc(0-15), hit_res(0/1)]
	// WINNER
	//if(controlState != CONTROL_WAIT_FOR_TURN) {return;}
	
	// RESULT
	if (shiparr_in[2] == 1) {
		UiShowLed(shiparr_in[4], shiparr_in[5], shiparr_in[3]);	// send location, hit_res, board
	}
	
	if (shiparr_in[0] == 1) {
		controlState = CONTROL_WAIT_FOR_GAME;
		LogMessage(LOG_DEBUG_LVL, "Player 1 Wins! \r\n");
		return;
	}
	else if (shiparr_in[0] == 2) {
		controlState = CONTROL_WAIT_FOR_GAME;
		LogMessage(LOG_DEBUG_LVL, "Player 2 Wins! \r\n");
		return;
	}
	
	if (shiparr_in[1] == PLAYER) {
		controlState = CONTROL_WAIT_FOR_ACTION;
		UiPlayerTurn(shiparr_in[1]); // send turn
	}
}



