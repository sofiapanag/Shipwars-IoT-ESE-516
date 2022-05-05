/**************************************************************************/ /**
 * @file      UiHandlerThread.h
 * @brief     File that contains the task code and supporting code for the UI Thread for ESE516 Spring (Online) Edition
 * @author    You! :)
 * @date      2020-04-09

 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
#include "ControlThread/ControlThread.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define UI_TASK_SIZE 400  //<Size of stack to assign to the UI thread. In words
#define UI_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/

typedef enum uiStateMachine_state {
	UI_STATE_IGNORE_PRESSES = 0,  ///< State used to handle buttons
	UI_STATE_PLACE_SHIP,      ///< State to ignore button presses
	UI_STATE_HANDLE_SHOOT,          ///< State to show ship loc.
	UI_STATE_MAX_STATES           ///< Max number of states

} uiStateMachine_state;

typedef enum uiPlacement_state {
	UI_PLACE_VALID = 0,  ///< State used to handle buttons
	UI_PLACE_PLACED,      ///< State to ignore button presses
	UI_PLACE_INVALID,          ///< State to show ship loc.
	UI_PLACE_MAX_STATES           ///< Max number of states

} uiPlacement_state;

#define R_PLACE_VALID 50
#define G_PLACE_VALID 50
#define B_PLACE_VALID 2

#define R_PLACE_PLACED 0
#define G_PLACE_PLACED 0
#define B_PLACE_PLACED 50

#define R_PLACE_INVALID 0
#define G_PLACE_INVALID 0
#define B_PLACE_INVALID 0

/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vUiHandlerTask(void *pvParameters);
void UiPlaceInit(uint8_t *shiparr_in,uint8_t ship_num_in);
void UiShowLed(uint8_t ship_fire_loc, uint8_t hit_res, uint8_t board);
void UiPlayerTurn(uint8_t turn);

#ifdef __cplusplus
}
#endif
