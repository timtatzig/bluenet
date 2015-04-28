/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

// resistance of the shunt in milli ohm
#define SHUNT_VALUE                              120

// size of the buffer used for characteristics
#define GENERAL_BUFFER_SIZE                      500

/* maximum length of strings used for characteristic values */
#define MAX_STRING_LENGTH                        25

/* Command to enter the bootloader and stay there.
 *
 * This should be the same value as defined in the bootloader. 
 */
#define COMMAND_ENTER_RADIO_BOOTLOADER           66

/*
 */
#define APP_TIMER_PRESCALER                      0
#define APP_TIMER_MAX_TIMERS                     10
#define APP_TIMER_OP_QUEUE_SIZE                  4

/*
 */
/* Maximum size of scheduler events. */
#define SCHED_MAX_EVENT_DATA_SIZE                MAX(APP_TIMER_SCHED_EVT_SIZE,\
                                                     BLE_STACK_HANDLER_SCHED_EVT_SIZE)
/* Maximum number of events in the scheduler queue. */
#define SCHED_QUEUE_SIZE                         10

// See https://devzone.nordicsemi.com/question/21164/s130-unstable-advertising-reports-during-scan-updated/
/* Determines scan interval in units of 0.625 millisecond. */
#define SCAN_INTERVAL                            0x00A0
/* Determines scan window in units of 0.625 millisecond. */
//#define SCAN_WINDOW                            0x0050
/* Determines scan window in units of 0.625 millisecond. */
#define SCAN_WINDOW                              0x009E