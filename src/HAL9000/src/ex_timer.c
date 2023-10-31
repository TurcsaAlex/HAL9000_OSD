#include "HAL9000.h"
#include "ex_timer.h"
#include "iomu.h"
#include "thread_internal.h"


//Module1:Threads
//need to create a function to initialize our global list and its lock

//initialization of the global timer list
static GLOBAL_TIMER_LIST m_globalTimerList;
void
ExTimerSystemPreinit(){

    //memzero(&m_globalTimerList, sizeof(GLOBAL_TIMER_LIST));

    InitializeListHead(&m_globalTimerList.TimerListHead);

    LockInit(&m_globalTimerList.TimerListLock);

}


STATUS
ExTimerInit(
    OUT     PEX_TIMER       Timer,
    IN      EX_TIMER_TYPE   Type,
    IN      QWORD           Time
    )
{
    STATUS status;
    INTR_STATE dummyState;

    if (NULL == Timer)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if (Type > ExTimerTypeMax)
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;

    memzero(Timer, sizeof(EX_TIMER));

    Timer->Type = Type;
    if (Timer->Type != ExTimerTypeAbsolute)
    {
        // relative time

        // if the time trigger time has already passed the timer will
        // be signaled after the first scheduler tick
        Timer->TriggerTimeUs = IomuGetSystemTimeUs() + Time;
        Timer->ReloadTimeUs = Time;
    }
    else
    {
        // absolute
        Timer->TriggerTimeUs = Time;
    }
    
    // Module1: Threads
    //initialization of the timer event once a timer is created event
    //need to use a notification even type, as it's very likely that many other threads will be waiting for it. use false for signaled state,because we need
    // to initialize the event and signal it at a later time in the code.
    ExEventInit(&Timer->TimerEvent, ExEventTypeNotification,FALSE);

    //afterwards, we need to insert the timer into the global list

    //step 1: acquire lock of the global list

    LockAcquire(&m_globalTimerList.TimerListLock, &dummyState);
    //step 2: Insert timer into the list
    InsertOrderedList(&m_globalTimerList.TimerListHead, &Timer->TimerListElem, ExTimerCompareListElems,NULL);
    //step 3: done inserting,just release lock
    LockRelease(&m_globalTimerList.TimerListLock, dummyState);

    return status;
}

void
ExTimerStart(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    Timer->TimerStarted = TRUE;
    //Module1: Threads
    

}


void
ExTimerStop(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    Timer->TimerStarted = FALSE;
    //Module1:Threads
    //since the timer is stopped,we need to signal the waiting threads
    //we first signal using the event of the timer like so:

    ExEventSignal(&Timer->TimerEvent);

    //then we need to clear/uninitialize the timer's event using:
    //ExEventClearSignal(&Timer->TimerEvent);
}

void
ExTimerWait(
    INOUT   PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    ExEventWaitForSignal(&Timer->TimerEvent);
    //Module1:Threads
    //commenting out the previous busy-waiting implementation,in order to replace it with our new implementation
    /*while (IomuGetSystemTimeUs() < Timer->TriggerTimeUs && Timer->TimerStarted)
    {
        ThreadYield();
    }*/
    //we need to make the thread wait for the signal from the associated timer's event,hence:
    //if (Timer->TimerStarted) {
       
    
}

void
ExTimerUninit(
    INOUT   PEX_TIMER       Timer
    )
{
    INTR_STATE dummyState;
    ASSERT(Timer != NULL);

    ExTimerStop(Timer);

    Timer->TimerUninited = TRUE;
    //Module1:Threads
    //Timer is uninitialized,so remove it from global timer list

    //first, acquire the list's lock and use a dummyState for interrupt state,as we should not turn off interrupts in our system(mentioned in hal docs)
    LockAcquire(&m_globalTimerList.TimerListLock, &dummyState);
    
    //remove the specific entry from the list. It's not necessarily the head of the list, so use RemoveEntryList to ensure correct removal on the timer

    RemoveEntryList(&Timer->TimerListElem);

    //finally, after successful removal, release the lock of the list

    LockRelease(&m_globalTimerList.TimerListLock, dummyState);
}



INT64
ExTimerCompareTimers(
    IN      PEX_TIMER     FirstElem,
    IN      PEX_TIMER     SecondElem
)
{
    return FirstElem->TriggerTimeUs - SecondElem->TriggerTimeUs;
}
//Module1: Threads
// This will check if the global timer is larger(passed) than the timer of the TimerChecked parameter,i.e. it's past due of triggering

STATUS
EXTimerCheck(
    IN      PLIST_ENTRY     TimerEntry,
    IN_OPT  PVOID           Context

)
{
    PEX_TIMER Timer;
    Timer = CONTAINING_RECORD(TimerEntry, EX_TIMER, TimerListElem);
    STATUS status;
    UNREFERENCED_PARAMETER(Context);
    //verify if timer is bigger than our current timer's trigger time
    if (IomuGetSystemTimeUs() >= Timer->TriggerTimeUs) {
        ExEventSignal(&Timer->TimerEvent);


        // if the timer is absolute and is triggered, we can't remove it now,as this function will be used to iterate over the entire list of timers
        // so we need to acquire the lock for iteration,hence calling the Uninit function will not work because it will try to acquire the lock again and will cause a deadlock
        
        //if the timer is periodic,we DON'T STOP IT. we need to reload it to its next trigger time according to its reload timer
       /* if (Timer->Type == ExTimerTypeRelativePeriodic) {
            Timer->TriggerTimeUs = IomuGetSystemTimeUs() + Timer->ReloadTimeUs;

        }
        if (Timer->Type == ExTimerTypeAbsolute) {
            ASSERT(Timer != NULL);

            ExTimerStop(Timer);

            Timer->TimerUninited = TRUE;

            RemoveEntryList(&Timer->TimerListElem);
        }*/

        status = STATUS_SUCCESS;
    }
else {
		status = STATUS_UNSUCCESSFUL;
	}
	return status;
}
//we will now iterate over the entire global list of timers and check for each using TimerCheck

INT64 
ExTimerCompareListElems(
    IN PLIST_ENTRY FirstListElem,
    IN PLIST_ENTRY SecondListElem,
    IN_OPT PVOID Context
)
{
    //use the CONTAINING_RECORD macro to get the timer element from the list element and the second list element
    UNREFERENCED_PARAMETER(Context);
	PEX_TIMER FirstTimerElem = CONTAINING_RECORD(FirstListElem, EX_TIMER, TimerListElem);
	PEX_TIMER SecondTimerElem= CONTAINING_RECORD(SecondListElem, EX_TIMER, TimerListElem);
    //if the first timer is smaller than the second timer, return -1
	return ExTimerCompareTimers(FirstTimerElem, SecondTimerElem);
}

void
ExTimerCheckAll() 

{//Module1:Threads
    //declare a dummy interrupt state
    INTR_STATE dummyState;
//acquire the lock of the global timer list
    LockAcquire(&m_globalTimerList.TimerListLock, &dummyState);
//iterate over the entire list of timers using the ForEachElementInList macro
    // set AllMustSucceed to TRUE,as we've modified the EXTimerCheck function to return a STATUS which,in order to forcefully break the iteration
    // once we find our first timer that doesn't need to be triggered.
    ForEachElementExecute(&m_globalTimerList.TimerListHead, EXTimerCheck, NULL, TRUE);

    LockRelease(&m_globalTimerList.TimerListLock, dummyState);


}
