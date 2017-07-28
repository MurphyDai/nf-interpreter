//
// Copyright (c) 2017 The nanoFramework project contributors
// See LICENSE file in the project root for full license information.
//

#include <ch.h>
#include <hal.h>
#include <cmsis_os.h>

#include <targetHAL.h>
#include <WireProtocol_ReceiverThread.h>
#include <nanoCLR_Application.h>
#include <nanoPAL_BlockStorage.h>

///////////////////////////////////////////////////////////////////////////////////////////
// RAM vector table declaration (valid for GCC only)
__IO uint32_t vectorTable[48] __attribute__((section(".RAMVectorTable")));

#define SYSCFG_MemoryRemap_SRAM     ((uint8_t)0x03)

///////////////////////////////////////////////////////////////////////////////////////////

// need to declare the Receiver thread here
osThreadDef(ReceiverThread, osPriorityNormal, 2048, "ReceiverThread");

//  Application entry point.
int main(void) {

  // HAL initialization, this also initializes the configured device drivers
  // and performs the board-specific initializations.
  halInit();

  // relocate the vector table to RAM
  // Copy the vector table from the Flash (mapped at the base of the application
  // load address) to the base address of the SRAM at 0x20000000.
  for(int i = 0; i < 48; i++)
  {
    vectorTable[i] = *(__IO uint32_t*)((uint32_t)&__nanoImage_start__ + (i<<2));
  } 

  // set CFGR1 register MEM_MODE bits value as "memory remap to SRAM"
  SYSCFG->CFGR1 |= SYSCFG_MemoryRemap_SRAM;

  // The kernel is initialized but not started yet, this means that
  // main() is executing with absolute priority but interrupts are already enabled.
  osKernelInitialize();

  // Prepares the serial driver 2 using UART2
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(1)); // USART2 TX
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(1)); // USART2 RX

  // create the receiver thread
  osThreadCreate(osThread(ReceiverThread), NULL);

  // start kernel, after this main() will behave like a thread with priority osPriorityNormal
  osKernelStart();

  // preparation for the CLR startup
  BlockStorage_AddDevices();

  CLR_SETTINGS clrSettings;

  memset(&clrSettings, 0, sizeof(CLR_SETTINGS));

  clrSettings.MaxContextSwitches         = 50;
  clrSettings.WaitForDebugger            = false;
  clrSettings.EnterDebuggerLoopAfterExit = true;

  // startup CLR now
  ClrStartup(clrSettings);

  while (true) { 
    osDelay(100);
  }
}