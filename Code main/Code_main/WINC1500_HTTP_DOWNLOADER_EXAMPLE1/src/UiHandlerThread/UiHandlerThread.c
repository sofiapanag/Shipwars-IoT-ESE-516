/**
* @file      UiHandlerThread.c
* @brief     File that contains the task code and supporting code for the UI
Thread for ESE516 Spring (Online) Edition
* @author    You! :)
* @date      2020-04-09

******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "UiHandlerThread/UiHandlerThread.h"

#include <errno.h>

#include "IMU/lsm6dso_reg.h"
#include "SeesawDriver/Seesaw.h"
#include "SerialConsole.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "gfx_mono.h"
#include "main.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
uiStateMachine_state uiState;         ///< Holds the current state of the UI
uint8_t ship_arr[MAX_SHIP];
uint8_t ship_num;		  
uint8_t fire_loc;			 ///< int to hold location of fire
uint8_t ship_loc_buffer;
uint8_t ship_loc_out[MAX_TILE];
uint8_t ship_loc_out_num = 0;

uiPlacement_state place_tile_stat[MAX_TILE] = {UI_PLACE_INVALID};
	
/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void UiPlaceSuggest2(uint8_t loc);
static void UiPlaceSuggest3(uint8_t loc_1, uint8_t loc_2);
static void UiRemoveSuggest(uint8_t loc);

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * Task Function
 ******************************************************************************/

/**
 * @fn		void vUiHandlerTask( void *pvParameters )
 * @brief	STUDENT TO FILL THIS
 * @details 	student to fill this
 * @param[in]	Parameters passed when task is initialized. In this case we can ignore them!
 * @return		Should not return! This is a task defining function.
 * @note
 */
void vUiHandlerTask(void *pvParameters)
{
    // Do initialization code here
    SerialConsoleWriteString("UI Task Started!");
    uiState = UI_STATE_IGNORE_PRESSES;  // Initial state

    // Here we start the loop for the UI State Machine
    while (1) {
        switch (uiState) {
            case (UI_STATE_IGNORE_PRESSES): {
				//SeesawReadKeypad(NEO_TRELLIS_ADDR_1, &ship_loc_buffer, 1);
				//SeesawReadKeypad(NEO_TRELLIS_ADDR_2, &ship_loc_buffer, 1);
                break;
            }

            case (UI_STATE_PLACE_SHIP): {
				ship_loc_out_num = 0;
				
				for(int i = 0; i < ship_num; i++){
					uint8_t ship_head, ship_tail;
					uint8_t cur_ship_size = 0;
					uint8_t cur_ship_arr[MAX_SHIP_SIZE];
					
					while(cur_ship_size < ship_arr[i]){
						uint8_t temp = SeesawGetKeypadCount(NEO_TRELLIS_ADDR_1) ;
						if(temp == 99){uiState = UI_STATE_IGNORE_PRESSES;}
						if(temp  == 0){vTaskDelay(50); continue;}
							
						if( ERROR_NONE == SeesawReadKeypad(NEO_TRELLIS_ADDR_1, &ship_loc_buffer, 1) ){
							
							ship_loc_buffer = NEO_TRELLIS_SEESAW_KEY((ship_loc_buffer & 0xFD) >> 2);
							
							if(cur_ship_size == 0){
								if(place_tile_stat[ship_loc_buffer] == UI_PLACE_PLACED){continue;}
								cur_ship_arr[cur_ship_size] = ship_loc_buffer;
								place_tile_stat[ship_loc_buffer] = UI_PLACE_PLACED;
								SeesawSetLed(NEO_TRELLIS_ADDR_1,ship_loc_buffer, 0, 0, 50);
								SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);
								
								ship_head = ship_loc_buffer;
								ship_tail = ship_loc_buffer;
								if(cur_ship_size != ship_arr[i]-1){
									UiPlaceSuggest2(ship_loc_buffer);
								}
							}
							else{
								//check validity of 2nd position
								if(place_tile_stat[ship_loc_buffer] != UI_PLACE_VALID){continue;}
									
								cur_ship_arr[cur_ship_size] = ship_loc_buffer;
								place_tile_stat[ship_loc_buffer] = UI_PLACE_PLACED;
								SeesawSetLed(NEO_TRELLIS_ADDR_1, ship_loc_buffer, R_PLACE_PLACED, G_PLACE_PLACED, B_PLACE_PLACED);
								SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);
								
								UiRemoveSuggest(ship_head);
								UiRemoveSuggest(ship_tail);
								
								if(ship_loc_buffer < ship_head){ship_head = ship_loc_buffer;}
								else{ship_tail = ship_loc_buffer;}
								
								if(cur_ship_size != ship_arr[i]-1){
									UiPlaceSuggest3(ship_head,ship_tail);
								}
							}
							cur_ship_size++;
						}
							
					}
					
					for(int j = 0; j < ship_arr[i];j++){
						ship_loc_out[ship_loc_out_num] = cur_ship_arr[j];
						ship_loc_out_num++;
					}
					
					
				}
				uiState = UI_STATE_IGNORE_PRESSES;
				//publish data back to the cloud
				LogMessage(LOG_DEBUG_LVL, "Placement finished! \r\n");
                break;
            }

            case (UI_STATE_HANDLE_SHOOT): {

                break;
            }

            default:  // In case of unforseen error, it is always good to sent state
                      // machine to an initial state.
                uiState = UI_STATE_IGNORE_PRESSES;
                break;
        }

        // After execution, you can put a thread to sleep for some time.
        vTaskDelay(50);
    }
}

/******************************************************************************
 * Functions
 ******************************************************************************/
void UiPlaceInit(uint8_t *shiparr_in,uint8_t ship_num_in)
{
	LogMessage(LOG_DEBUG_LVL, "Placement started! \r\n");
	memcpy (ship_arr, shiparr_in, ship_num * sizeof (uint8_t));
	ship_num = ship_num_in;
	for(int i =0 ; i < MAX_TILE; i++){
		SeesawSetLed(NEO_TRELLIS_ADDR_1,i,R_PLACE_INVALID,G_PLACE_INVALID,B_PLACE_INVALID);
		place_tile_stat[i] = UI_PLACE_INVALID;
	}
	SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);
	uiState = UI_STATE_PLACE_SHIP;
}

static void UiPlaceSuggest2(uint8_t loc)
{
	uint8_t rec_loc;
	if(loc > 3){
		rec_loc = loc - 4;
		if(place_tile_stat[rec_loc] != UI_PLACE_PLACED){
			place_tile_stat[rec_loc] = UI_PLACE_VALID;
			SeesawSetLed(NEO_TRELLIS_ADDR_1,rec_loc, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
		}
	}
	
	if(loc < 12){
		rec_loc = loc + 4;
		if(place_tile_stat[rec_loc] != UI_PLACE_PLACED){
			place_tile_stat[rec_loc] = UI_PLACE_VALID;
			SeesawSetLed(NEO_TRELLIS_ADDR_1,rec_loc, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
		}
	}
	
	if(loc % 4 != 3){
		rec_loc = loc + 1;
		if(place_tile_stat[rec_loc] != UI_PLACE_PLACED){
			place_tile_stat[rec_loc] = UI_PLACE_VALID;
			SeesawSetLed(NEO_TRELLIS_ADDR_1,rec_loc, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
		}
	}
	
	if(loc % 4 != 0){
		rec_loc = loc - 1;
		if(place_tile_stat[rec_loc] != UI_PLACE_PLACED){
			place_tile_stat[rec_loc] = UI_PLACE_VALID;
			SeesawSetLed(NEO_TRELLIS_ADDR_1,rec_loc, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
			
		}
	}
	SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);
	
}

static void UiPlaceSuggest3(uint8_t loc_1, uint8_t loc_2)
{
	uint8_t loc_h, loc_t;
	
	if(loc_1 < loc_2){loc_h = loc_1; loc_t = loc_2;}
	else{loc_h = loc_2; loc_t = loc_1;}
	
	if(loc_h % 4 == loc_t % 4){
		if(loc_h > 4){
			if(place_tile_stat[loc_h - 4] == UI_PLACE_INVALID){
				place_tile_stat[loc_h - 4] = UI_PLACE_VALID;
				SeesawSetLed(NEO_TRELLIS_ADDR_1, loc_h - 4, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
			}
		}
		if(loc_t < 12){
			if(place_tile_stat[loc_t + 4] == UI_PLACE_INVALID){
				place_tile_stat[loc_t + 4] = UI_PLACE_VALID;
				SeesawSetLed(NEO_TRELLIS_ADDR_1, loc_t + 4, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
			}
		}
	}
	else if((int)loc_h/4 == (int)loc_t/4){
		// if horizontal 
		if(loc_h % 4 != 0){
			if(place_tile_stat[loc_h - 1] == UI_PLACE_INVALID){
				place_tile_stat[loc_h - 1] = UI_PLACE_VALID;
				SeesawSetLed(NEO_TRELLIS_ADDR_1, loc_h - 1, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
			}
		}
		if(loc_t %4 != 3){
			if(place_tile_stat[loc_t + 1] == UI_PLACE_INVALID){
				place_tile_stat[loc_t + 1] = UI_PLACE_VALID;
				SeesawSetLed(NEO_TRELLIS_ADDR_1, loc_t + 1, R_PLACE_VALID, G_PLACE_VALID, B_PLACE_VALID);
			}
		}
		
	}
	
	SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);
}


static void UiRemoveSuggest(uint8_t loc)
{
	uint8_t rec_loc;

	rec_loc = loc + 4;
	if(place_tile_stat[rec_loc] == UI_PLACE_VALID){
		place_tile_stat[rec_loc] = UI_PLACE_INVALID;
		SeesawSetLed(NEO_TRELLIS_ADDR_1, rec_loc, R_PLACE_INVALID, G_PLACE_INVALID, B_PLACE_INVALID);

	}

	rec_loc = loc - 4;
	if(place_tile_stat[rec_loc] == UI_PLACE_VALID){
		place_tile_stat[rec_loc] = UI_PLACE_INVALID;
		SeesawSetLed(NEO_TRELLIS_ADDR_1, rec_loc, R_PLACE_INVALID, G_PLACE_INVALID, B_PLACE_INVALID);

	}

	rec_loc = loc + 1;
	if(place_tile_stat[rec_loc] == UI_PLACE_VALID){
		place_tile_stat[rec_loc] = UI_PLACE_INVALID;
		SeesawSetLed(NEO_TRELLIS_ADDR_1, rec_loc, R_PLACE_INVALID, G_PLACE_INVALID, B_PLACE_INVALID);

	}

	rec_loc = loc - 1;
	if(place_tile_stat[rec_loc] == UI_PLACE_VALID){
		place_tile_stat[rec_loc] = UI_PLACE_INVALID;
		SeesawSetLed(NEO_TRELLIS_ADDR_1, rec_loc, R_PLACE_INVALID, G_PLACE_INVALID, B_PLACE_INVALID);
	}
	SeesawOrderLedUpdate(NEO_TRELLIS_ADDR_1);

}