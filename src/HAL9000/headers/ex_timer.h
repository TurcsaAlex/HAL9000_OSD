#pragma once
#include "ex_event.h"

typedef enum _EX_TIMER_TYPE
{
    ExTimerTypeAbsolute,
    ExTimerTypeRelativeOnce,
    ExTimerTypeRelativePeriodic,
    ExTimerTypeMax = ExTimerTypeRelativePeriodic
} EX_TIMER_TYPE;

typedef struct _EX_TIMER
{
    // absolute time
    QWORD               TriggerTimeUs;

    // valid only if timer is periodic
    QWORD               ReloadTimeUs;

    EX_TIMER_TYPE       Type;
    
    // list of waiting/blocked threads for a specific timer

    EX_EVENT            TimerEvent;
    //for placing timer in global timers list
    LIST_ENTRY          TimerListElem;

    volatile BOOLEAN    TimerStarted;
    BOOLEAN             TimerUninited;
} EX_TIMER, *PEX_TIMER;
//Module1:Threads
// creating global list of timers
typedef struct _GLOBAL_TIMER_LIST
{
    // for protecting list access
    LOCK TimerListLock;
    
    //head of the list
   

    LIST_ENTRY          TimerListHead;
   
}GLOBAL_TIMER_LIST,*PGLOBAL_TIMER_LIST;

//******************************************************************************
// Function:     ExTimerInit
// Description:  Initializes a timer to trigger to trigger at a specified time.
//               Once this function returns, the timer can be waited by multiple
//               threads at once. When the timer triggers - no matter its type -
//               all the threads waiting on it must be woken up.
// Returns:      STATUS
// Parameter:    OUT PEX_TIMER Timer - Timer to initialize
// Parameter:    IN EX_TIMER_TYPE Type - Defines the periodicity of the timer
//               (one-shot or periodic) and defines the meaning of the 3rd
//               parameter (if absolute or relative to the current time).
// Parameter:    IN QWORD TimeUs
//
// Examples:
// Initialize a one-shot timer to trigger after 1 second:
// ExTimerInit(&timer, ExTimerTypeRelativeOnce, 1 * SEC_IN_US);
//
// Initialize a periodic timer to trigger every minute:
// ExTimerInit(&timer, ExTimerTypeRleativePeriodic, 60 * SEC_IN_US);
//
// Initialize an absolute timer after the OS has run 2 minutes:
// ExTimerInit(&timer, ExTimerTypeAbsolute, 120 * SEC_IN_US);
//******************************************************************************
STATUS
ExTimerInit(
    OUT     PEX_TIMER       Timer,
    IN      EX_TIMER_TYPE   Type,
    IN      QWORD           TimeUs
    );

//******************************************************************************
// Function:     ExTimerStart
// Description:  Starts the timer countdown. If the time has already elapsed all
//               the waiting threads must be woken up.
// Returns:      void
// Parameter:    IN PEX_TIMER Timer
//******************************************************************************
void
ExTimerStart(
    IN      PEX_TIMER       Timer
    );


//******************************************************************************
// Function:     ExTimerStop
// Description:  Stops the timer countdown. All the threads waiting must be
//               woken up.
// Returns:      void
// Parameter:    IN PEX_TIMER Timer
//******************************************************************************
void
ExTimerStop(
    IN      PEX_TIMER       Timer
    );

//******************************************************************************
// Function:     ExTimerWait
// Description:  Called by a thread to wait for the timer to trigger. If the
//               timer already triggered and it's not periodic or if the timer
//               is uninitialized this function must return instantly.
// Returns:      void
// Parameter:    INOUT PEX_TIMER Timer
//******************************************************************************
void
ExTimerWait(
    INOUT   PEX_TIMER       Timer);

//******************************************************************************
// Function:     ExTimerUninit
// Description:  Uninitialized a timer. It may not be used in the future without
//               calling the ExTimerInit function. All threads waiting for the
//               timer must be woken up.
// Returns:      void
// Parameter:    INOUT PEX_TIMER Timer
//******************************************************************************
void
ExTimerUninit(
    INOUT   PEX_TIMER       Timer
    );

//******************************************************************************
// Function:     ExTimerCompareTimers
// Description:  Utility function to compare to two timers.
// Returns:      INT64 - if NEGATIVE => the first timers trigger time is earlier
//                     - if 0 => the timers trigger time is equal
//                     - if POSITIVE => the first timers trigger time is later
// Parameter:    IN PEX_TIMER FirstElem
// Parameter:    IN PEX_TIMER SecondElem
//******************************************************************************
INT64
ExTimerCompareTimers(
    IN      PEX_TIMER     FirstElem,
    IN      PEX_TIMER     SecondElem
    );
//Module1: Threads
//adding these 2 functions to the header file as they will be used in other files

void ExTimerSystemPreinit();
INT64
ExTimerCompareListElems(
    IN PLIST_ENTRY FirstListElem,
    IN PLIST_ENTRY SecondListElem,
    IN_OPT PVOID Context
);
void ExTimerCheckAll();

STATUS
EXTimerCheck(
    IN      PLIST_ENTRY     TimerEntry,
    IN_OPT PVOID           Context

);