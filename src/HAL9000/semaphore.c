#include "HAL9000.h"
#include "thread_internal.h"
#include "mutex.h"


typedef struct _SEMAPHORE
{
    DWORD           Value;

    // ... add more fields here ...
} SEMAPHORE, * PSEMAPHORE;

void
SemaphoreInit(
    OUT     PSEMAPHORE      Semaphore,
    IN      DWORD           InitialValue
);

void
SemaphoreDown(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
);

void
SemaphoreUp(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
);