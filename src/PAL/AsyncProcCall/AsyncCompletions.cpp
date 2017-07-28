//
// Copyright (c) 2017 The nanoFramework project contributors
// Portions Copyright (c) Microsoft Corporation.  All rights reserved.
// See LICENSE file in the project root for full license information.
//

#include <nanoPAL.h>


HAL_DblLinkedList<HAL_CONTINUATION> g_HAL_Completion_List;

/***************************************************************************/

void HAL_COMPLETION::Execute()
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    // TODO check if we need this 
    //ASSERT_IRQ_MUST_BE_OFF();

#if defined(_DEBUG)
    this->Start_RTC_Ticks = 0;
#endif

    if(this->ExecuteInISR)
    {
        HAL_CONTINUATION* cont = this;

        cont->Execute();
    }
    else
    {
        this->Enqueue();
    }
}

//--//

static const unsigned __int64 HAL_Completion_IdleValue = 0x0000FFFFFFFFFFFFull;

void HAL_COMPLETION::InitializeList()
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    g_HAL_Completion_List.Initialize();
}

//--//

void HAL_COMPLETION::DequeueAndExec()
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    GLOBAL_LOCK(irq);

    HAL_COMPLETION* ptr     = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();
    HAL_COMPLETION* ptrNext = (HAL_COMPLETION*)ptr->Next();

    // waitforevents does not have an associated completion, therefore we need to verify
    // than their is a next completion and that the current one has expired.
    if(ptrNext)
    {
        Events_Set(SYSTEM_EVENT_FLAG_SYSTEM_TIMER);

        ptr->Unlink();

        //
        // In case there's no other request to serve, set the next interrupt to be 356 years since last powerup (@25kHz).
        //
        Time_SetCompare( ptrNext->Next() ? ptrNext->EventTimeTicks : HAL_Completion_IdleValue );

#if defined(_DEBUG)
        ptr->EventTimeTicks = 0;
#endif  // defined(_DEBUG)

        // let the ISR turn on interrupts, if it needs to
        ptr->Execute();
    }

    GLOBAL_UNLOCK(irq);
}

//--//

void HAL_COMPLETION::EnqueueTicks( unsigned __int64 eventTimeTicks )
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    ASSERT(eventTimeTicks != 0);
    GLOBAL_LOCK(irq);

    this->EventTimeTicks  = eventTimeTicks;
#if defined(_DEBUG)
    this->Start_RTC_Ticks = HAL_Time_CurrentSysTicks();
#endif

    HAL_COMPLETION* ptr     = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();
    HAL_COMPLETION* ptrNext;

    for(;(ptrNext = (HAL_COMPLETION*)ptr->Next()); ptr = ptrNext)
    {
        if(eventTimeTicks < ptr->EventTimeTicks) break;
    }

    g_HAL_Completion_List.InsertBeforeNode( ptr, this );
    
    if(this == g_HAL_Completion_List.FirstNode())
    {
        Time_SetCompare( eventTimeTicks );
    }

    GLOBAL_UNLOCK(irq);
}

// the argument to enqueue is in miliseconds as we don't need anything bellow this in a reasonale use case scenario
void HAL_COMPLETION::EnqueueDelta64( unsigned __int64 miliSecondsFromNow )
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    // grab time first to be closest to now as possible from when this function was called
    unsigned __int64 now            = HAL_Time_CurrentSysTicks();
    unsigned __int64 eventTimeTicks = CPU_MilisecondsToSysTicks( miliSecondsFromNow );

    EnqueueTicks( now + eventTimeTicks );
}

// the argument to enqueue is in miliseconds as we don't need anything bellow this in a reasonale use case scenario
void HAL_COMPLETION::EnqueueDelta( unsigned int miliSecondsFromNow )
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    EnqueueDelta64( (unsigned __int64)miliSecondsFromNow );
}

//--//

void HAL_COMPLETION::Abort()
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    GLOBAL_LOCK(irq);

    HAL_COMPLETION* firstNode = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();

    this->Unlink();

#if defined(_DEBUG)
    this->Start_RTC_Ticks = 0;
#endif

    if(firstNode == this)
    {
        unsigned __int64 nextTicks;

        if(g_HAL_Completion_List.IsEmpty())
        {
            //
            // In case there's no other request to serve, set the next interrupt to be 356 years since last power up (@25kHz).
            //
            nextTicks = HAL_Completion_IdleValue;
        }
        else
        {
            firstNode = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();

            nextTicks = firstNode->EventTimeTicks;
        }

        Time_SetCompare( nextTicks );
    }

    GLOBAL_UNLOCK(irq);
}

//--//

void HAL_COMPLETION::WaitForInterrupts( unsigned __int64 expire, unsigned int sleepLevel, unsigned __int64 wakeEvents )
{
    // TODO 
    // still unclear on the best way to handle this

    // NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    // const int setCompare   = 1;
    // const int resetCompare = 2;
    // const int nilCompare   = 4;

    // ASSERT_IRQ_MUST_BE_OFF();

    // HAL_COMPLETION* ptr = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();
    // int             state;

    // if(ptr->Next() == NULL)
    // {
    //     state = setCompare | nilCompare;
    // }
    // else if(ptr->EventTimeTicks > expire)
    // {
    //     state = setCompare | resetCompare;
    // }
    // else
    // {
    //     state = 0;
    // }

    // if(state & setCompare) Time_SetCompare( expire );

    // CPU_Sleep( (SLEEP_LEVEL)sleepLevel, wakeEvents );

    // if(state & (resetCompare | nilCompare))
    // {   
    //     // let's get the first node again
    //     // it could have changed since CPU_Sleep re-enabled interrupts
    //     ptr = (HAL_COMPLETION*)g_HAL_Completion_List.FirstNode();
    //     Time_SetCompare( (state & resetCompare) ? ptr->EventTimeTicks : HAL_Completion_IdleValue );
    // }
}

void HAL_COMPLETION::Uninitialize()
{
    NATIVE_PROFILE_PAL_ASYNC_PROC_CALL();
    GLOBAL_LOCK(irq);
    
    HAL_COMPLETION* ptr;

    while(TRUE)
    {
        ptr = (HAL_COMPLETION*)g_HAL_Completion_List.ExtractFirstNode();

        if(!ptr) 
        {
            break;
        }
        
    }

    GLOBAL_UNLOCK(irq);
}