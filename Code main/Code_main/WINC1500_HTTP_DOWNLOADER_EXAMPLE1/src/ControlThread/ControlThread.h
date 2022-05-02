/**************************************************************************/ /**
 * @file      ControlThread.h
 * @brief     Thread code for the ESE516 Online game control thread
 * @author    You!
 * @date      2020-04-015

 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define CONTROL_TASK_SIZE 256  //<Size of stack to assign to the UI thread. In words
#define CONTROL_TASK_PRIORITY (configMAX_PRIORITIES - 1)
typedef enum controlStateMachine_state {
    CONTROL_WAIT_FOR_GAME = 0,  ///< State used to WAIT FOR A GAME COMMAND
    CONTROL_WAIT_FOR_PLACE,       ///< State used to wait for user to play a game move
    CONTROL_WAIT_FOR_TURN,           ///< State to show game end
	CONTROL_WAIT_FOR_ACTION,
    CONTROL_STATE_MAX_STATES    ///< Max number of states

} controlStateMachine_state;

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/


/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vControlHandlerTask(void *pvParameters);
void ControlSetGame(uint8_t *shiparr_in,uint8_t ship_num);

#ifdef __cplusplus
}
#endif