/*
 * Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
 * and Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <unistd.h> 
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ioctl.h>
  
#include "halMev2Api.h"
#include "uclo.h"

extern "C"
{
  #include "api.h"
}

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

#define SUCCESS   RC_SUCCESS
#define FAILURE   0xFFFFFFFF

typedef struct
{
   UINT32 rxPort;
   UINT32 rxInterface;
   UINT32 txInterface;
   UINT32 txPort;
} Connection;

#define ONL_CONNECTION_COUNTA 10
#define ONL_CONNECTION_COUNTB 10

/* ONL (RTM 0-4 to NPU A and back, RTM 5-9 to NPU B and back) */
Connection ONLConnectionsA[ONL_CONNECTION_COUNTA] =
{
   /* NPUA to RTM connections */
   {0, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 0},
   {1, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 1},
   {2, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 2},
   {3, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 3},
   {4, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 4},
   
   /* RTM to NPUA connections */
   {0, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 0},
   {1, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 1},
   {2, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 2},
   {3, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 3},
   {4, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 4},
};
Connection ONLConnectionsB[ONL_CONNECTION_COUNTB] =
{
   /* NPUB to RTM connections */
   {0, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 5},
   {1, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 6},
   {2, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 7},
   {3, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 8},
   {4, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 9},

   /* RTM to NPUB connections */
   {5, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 0},
   {6, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 1},
   {7, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 2},
   {8, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 3},
   {9, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 4},
};

UINT32 openConnections(Connection connections[], UINT32 connectionCount);
UINT32 NPU_SPI_Setup(UINT32 Processor);
UINT32 Train_SPI_NPU(UINT32 Processor, UINT32 CalendarLength);
UINT32 Train_NPU_SPI_Calendar(UINT32 Processor, UINT32 CalendarLength, CALENDAR *pCalendar);

void print_usage(char *);
std::string int2str(unsigned int);

UINT32 openConnections(Connection connections[], UINT32 connectionCount)
{
   INT32  result;
   UINT32 i;

   for (i = 0; i < connectionCount; i++)
   {
      result = SpiSwitchOpenConnection(connections[i].rxPort,
                                       connections[i].rxInterface,
                                       connections[i].txInterface,
                                       connections[i].txPort);
      if (result != RC_SUCCESS)
      {
         return (result);
      }
   }

   return(RC_SUCCESS);
}

UINT32 NPU_SPI_Setup(UINT32 Processor)
{
  UINT32 Result;
  UINT32 Interface;

  InterfaceType  NpuSpiInterfaceSettings;

  UINT32 AllRxInts        = 0xFF;
  UINT32 AllTxInts        = 0xF;
  UINT32 CalLen           = 10;
  UINT32 CalMult          = 1;
  UINT32 CalReg0          = 0x76543210;
  UINT32 CalReg1          = 0xFEDCBA98;
  UINT32 ClkMult          = 8;
  UINT32 FifoSize         = 0;
  UINT32 MaxBurst1        = 8;
  UINT32 MaxBurst2        = 4;
  UINT32 MaxData          = 0;
  UINT32 MultiPort        = 0;
  UINT32 NumConnections   = 0;
  UINT32 RisingClockEdge  = 1;
  UINT32 RxHiWater        = 0x100;
  UINT32 RxLoWater        = 0x80;
  UINT32 ServiceLimit     = 0x804;
  UINT32 TxHiWater        = 5;
  UINT32 TxLoWater        = 2;
  UINT32 TrainingReps     = 2;
  UINT32 Unused           = 0;

  if (Processor == PPM_MASTER_PROC)
  {
    Interface = SPI_SWITCH_NPUA_INTERFACE;
  }
  else if (Processor == PPM_SLAVE_PROC)
  {
    Interface = SPI_SWITCH_NPUB_INTERFACE;
  }
  else
  {
    return (RC_SWITCH_INVALID_PARAM);
  }

/*
  Result = SpiSwitchStopInterface(Interface, SPI_SWITCH_TX_INTERFACE);
  if (Result != RC_SUCCESS)
  {
     std::cout << "SpiSwitchStopInterface (tx) failed ";
     return (Result);
  }
*/

  Result = SpiSwitchStopInterface(Interface, SPI_SWITCH_RX_INTERFACE);
  if (Result != RC_SUCCESS)
  {
     std::cout << "SpiSwitchStopInterface (rx) failed ";
     return (Result);
  }

  NpuSpiInterfaceSettings.rxInterface.calendarLength             = CalLen;
  NpuSpiInterfaceSettings.rxInterface.calendarMultiplicity       = CalMult;
  NpuSpiInterfaceSettings.rxInterface.interruptMask              = AllRxInts;
  NpuSpiInterfaceSettings.rxInterface.operationMode              = MultiPort;
  NpuSpiInterfaceSettings.rxInterface.calendar[0]                = CalReg0;
  NpuSpiInterfaceSettings.rxInterface.calendar[1]                = CalReg1;
  NpuSpiInterfaceSettings.rxInterface.watermarks.lowWatermark    = RxLoWater;
  NpuSpiInterfaceSettings.rxInterface.watermarks.highWatermark   = RxHiWater;
  NpuSpiInterfaceSettings.rxInterface.clockEdge                  = RisingClockEdge;
  NpuSpiInterfaceSettings.txInterface.fifoSize                   = FifoSize;

  NpuSpiInterfaceSettings.txInterface.calendarLength             = CalLen;
  NpuSpiInterfaceSettings.txInterface.calendarMultiplicity       = CalMult;
  NpuSpiInterfaceSettings.txInterface.interruptMask              = AllTxInts;
  NpuSpiInterfaceSettings.txInterface.operationMode              = MultiPort;
  NpuSpiInterfaceSettings.txInterface.calendar[0]                = CalReg0;
  NpuSpiInterfaceSettings.txInterface.calendar[1]                = CalReg1;
  NpuSpiInterfaceSettings.txInterface.watermarks.lowWatermark    = TxLoWater;
  NpuSpiInterfaceSettings.txInterface.watermarks.highWatermark   = TxHiWater;
  NpuSpiInterfaceSettings.txInterface.clockEdge                  = RisingClockEdge;
  NpuSpiInterfaceSettings.txInterface.clockMultiplier            = ClkMult;
  NpuSpiInterfaceSettings.txInterface.trainingRepetitions        = TrainingReps;
  NpuSpiInterfaceSettings.txInterface.dataMaxT                   = MaxData;
  NpuSpiInterfaceSettings.txInterface.serviceLimit               = ServiceLimit;

  /* Automatically determine NPU A RX FIFO size and watermark levels*/
  NumConnections = 5;
  FifoSize = 2;
  NpuSpiInterfaceSettings.rxInterface.fifoSize = FifoSize;
  NpuSpiInterfaceSettings.rxInterface.watermarks.lowWatermark <<= FifoSize;
  NpuSpiInterfaceSettings.rxInterface.watermarks.highWatermark <<= FifoSize;

  /* Automatically determine NPU A TX FIFO size and watermark levels*/
  NumConnections = 5;
  FifoSize = 2;
  NpuSpiInterfaceSettings.txInterface.fifoSize = FifoSize;
  /* Automatically adjusts the service limit and max bursts */
  NpuSpiInterfaceSettings.txInterface.serviceLimit = ServiceLimit << FifoSize;
  /* Mark unused calendar entries (we only use the first two) */
  for(int i = 2; i < SPI_SWITCH_MAX_CALENDAR_ENTRIES; i++)
  {
     NpuSpiInterfaceSettings.rxInterface.calendar[i] = Unused;
     NpuSpiInterfaceSettings.txInterface.calendar[i] = Unused;
  }

  for(int i = 0; i < SPI_SWITCH_MAX_FIFOS_PER_INTERFACE; i++)
  {
     NpuSpiInterfaceSettings.txInterface.maxBurst1[i] = MaxBurst1 << FifoSize;
     NpuSpiInterfaceSettings.txInterface.maxBurst2[i] = MaxBurst2 << FifoSize;
  }

  //Result = SpiSwitchConfigureInterface(Interface, SPI_SWITCH_RX_INTERFACE | SPI_SWITCH_TX_INTERFACE, 0, &NpuSpiInterfaceSettings);
  /*
  Result = SpiSwitchConfigureInterface(Interface, SPI_SWITCH_RX_INTERFACE, 0, &NpuSpiInterfaceSettings);
  if (Result != RC_SUCCESS)
  {
     std::cout << "SpiSwitchConfigureInterface failed ";
     return (Result);
  }
*/

  return RC_SUCCESS;
}

UINT32 Train_SPI_NPU(UINT32 Processor, UINT32 CalendarLength)
{
   PPM_STATUS  Result = FAILURE;
   UINT32      Interface;
   UINT32      Len = CalendarLength;

   //UINT32 InterfaceStartedTx;

   if (Processor == PPM_MASTER_PROC)
   {
      Interface = SPI_SWITCH_NPUA_INTERFACE;
      //InterfaceStartedTx = NPUAInterfaceStartedTx;
   }
   else if (Processor == PPM_SLAVE_PROC)
   {
      Interface = SPI_SWITCH_NPUB_INTERFACE;
      //InterfaceStartedTx = NPUBInterfaceStartedTx;
   }
   else
   {
      return (RC_SWITCH_INVALID_PARAM);
   }

   /* Call API to start SPI Tx, which starts training             */
   /* Only call StartInterface if it has not already been called  */
   /* during this Train() iteration for this interface            */
   //if (InterfaceStartedTx == FALSE)
   //{
      Result = SpiSwitchStartInterface(Interface, SPI_SWITCH_TX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         std::cout << "SpiSwitchStartInterface failed ";
         return (Result);
      }
      else
      {
         if (Processor == PPM_MASTER_PROC)
         {
            //NPUAInterfaceStartedTx = TRUE;
         }
         else if (Processor == PPM_SLAVE_PROC)
         {
            //NPUBInterfaceStartedTx = TRUE;
         }
      }
   //}

   /* Call API to check if the NPU Data training   */
   /* was received and start sending flow control  */
   Result = MiscNPUReceiveTraining(Processor, Len);
   if (Result != RC_SUCCESS)
   {
      std::cout << "MiscNPUReceiveTraining failed ";
      return (Result);
   }

   /* Call API to check if data training stopped */
   Result = MiscNPUHasTrainingStopped(Processor);

   /* Wait for data training to stop */
   Len = 100;
   while ((Result == FALSE) && Len)
   {
      /* Sleep for 10 ms = 10000 us and check again */
      usleep(10 * 1000);
      Result = MiscNPUHasTrainingStopped(Processor);
      Len--;
   }

   if (Result == FALSE )
   {
      std::cout << "MiscNPUHasTrainingStopped failed ";
      return (0xFFFFFFFF);
   }

   /* Call API to check if the SPI Tx interface has sync */
   Result = SpiSwitchTxCheckSync(Interface);
   if (Result != RC_SUCCESS)
   {
      std::cout << "SpiSwitchTxCheckSync failed ";
      return (Result);
   }

   return (Result);
}

UINT32 Train_NPU_SPI_Calendar(UINT32 Processor, UINT32 CalendarLength, CALENDAR *pCalendar)
{
   PPM_STATUS  Result = FAILURE;
   UINT32      Interface;
   UINT32      Len = CalendarLength;

   //UINT32 InterfaceStartedRx;

   if (Processor == PPM_MASTER_PROC)
   {
      Interface = SPI_SWITCH_NPUA_INTERFACE;
      //InterfaceStartedRx = NPUAInterfaceStartedRx;
   }
   else if (Processor == PPM_SLAVE_PROC)
   {
      Interface = SPI_SWITCH_NPUB_INTERFACE;
      //InterfaceStartedRx = NPUBInterfaceStartedRx;
   }
   else
   {
      return (RC_SWITCH_INVALID_PARAM);
   }

   /* Call API to start data training from the NPU Tx interface */
   Result = MiscNPUSendTrainingCalendar(Processor, Len, pCalendar);
   if (Result != RC_SUCCESS)
   {
      std::cout << "MiscNPUSendTrainingCalendar failed ";
      return (Result);
   }
   //if (InterfaceStartedRx == FALSE)
   //{
      /* Call API to start SPI Rx interface */
      /* Only call StartInterface if it has not already been called  */
      /* during this Train() iteration for this interface            */
      Result = SpiSwitchStartInterface(Interface, SPI_SWITCH_RX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
        std::cout << "SpiSwitchStartInterface failed ";
        return (Result);
      }
      else
      {
         if (Processor == PPM_MASTER_PROC)
         {
            //NPUAInterfaceStartedRx = TRUE;
         }
         else if (Processor == PPM_SLAVE_PROC)
         {
            //NPUBInterfaceStartedRx = TRUE;
         }
      }
   //}

   /* Call API to check if the SPI Rx interface got sync */
   Result = SpiSwitchRxCheckSync(Interface);
   if (Result != RC_SUCCESS)
   {
      std::cout << "SpiSwitchRxCheckSync failed ";
      return (Result);
   }

   /* Call API to stop NPU Tx data training and */
   /* check if SPI Flow Control was received    */
   Result = MiscNPUSendFlowCntl(Processor);

   if (Result != RC_SUCCESS)
   {
      std::cout << "MuscNPUSendFlowCntl failed ";
      return (Result);
   }

   return (Result);
}

void print_usage(char *prog)
{
  std::cout << prog << " [--help] [--base router_base_uof_file]\n"
            << "\t--help: print this message\n"
            << "\t--base router_base_uof_file: default is HW_NPUA.uof\n";
  exit(0);
}

std::string int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}

int main(int argc, char **argv)
{
  std::string base_uof = "HW_NPUA.uof";
  char input[10];
  
  int status;

  void *base_handle;
  unsigned int base_me_mask;
  unsigned int me;
  unsigned int ctx_mask = ME_ALL_CTX;

  static struct option longopts[] =
  {{"help",   0, NULL, 'h'},
   {"base",   1, NULL, 'b'},
   {NULL,     0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hb:", longopts, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        break;
      case 'b':
        base_uof = optarg;
        break;
      default:
        print_usage(argv[0]);
        exit(1);
    }
  }

/*
  std::cout << "Calling rpclOpen...";
  RppClientOpts rpp_options = {50, FALSE, FALSE, ""};
  char rpp_addr[16];
  memset(rpp_addr, 0, 16);
  strcpy(rpp_addr, "127.0.0.1");
  if((status = rpclOpen((UCHAR *)rpp_addr,&rpp_options)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Calling UcLo_InitLib...";
  UcLo_InitLib();
  std::cout << "done.\n";
*/

  std::cout << "Calling halMe_Init...";
  if((status = halMe_Init(0xff00ff)) != HALME_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_LoadObjFile for the router base...";
  if((status = UcLo_LoadObjFile(&base_handle, (char *)base_uof.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  base_me_mask = UcLo_GetAssignedMEs(base_handle);
  
  std::cout << "Calling UcLo_WriteUimageAll for router base...";
  if((status = UcLo_WriteUimageAll(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling halMe_Start for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;

    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to stop the microengines.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to start the microengines.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Start for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to stop the microengines.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

/*
  std::cout << "Calling halMe_reset...";
  halMe_Reset(base_me_mask, 1);
  std::cout << "done.\n";
*/

/*
  std::cout << "Press enter when you want to reload then start the microengines.\n";
  std::cin.getline(input,10);
*/

/*
  std::cout << "Calling SpiSwitchReset...";
  if((status = SpiSwitchReset()) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Setting up SPI switch parameters...";
  if((status = NPU_SPI_Setup(PPM_MASTER_PROC)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Calling MiscNPUReset...";
  if((status = MiscNPUReset(PPM_MASTER_PROC)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Retraining NPUA to SPI switch...";
  UINT32 NPUATxCalLen = 10;
  CALENDAR npuATxCalendar;
  memset(&npuATxCalendar, 0, sizeof(npuATxCalendar));
  for(unsigned int i=0; i<NPUATxCalLen; ++i)
  {
     npuATxCalendar[i] = i;
  }
  if((status = Train_NPU_SPI_Calendar(PPM_MASTER_PROC, NPUATxCalLen, &npuATxCalendar)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Retraining SPI switch to NPUA...";
  UINT32 NPUARxCalLen = 10;
  if((status = Train_SPI_NPU(PPM_MASTER_PROC, NPUARxCalLen)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Calling halMe_ClrReset...";
  //halMe_ClrReset(base_me_mask);
  //halMe_ClrReset(0xff00ff);
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    halMe_PutMeCsr(me, CTX_ENABLES, 0);
    halMe_PutCtxIndrCsr(me, ME_ALL_CTX, CTX_STS_INDIRECT, ICS_CTX_PC & 0);
    halMe_PutMeCsr(me, CTX_ARB_CNTL, 0);
    halMe_PutMeCsr(me, CC_ENABLE, 0x2000);
    halMe_PutCtxWakeupEvents(me, ME_ALL_CTX, XCWE_VOLUNTARY);
    halMe_PutCtxSigEvents(me, ME_ALL_CTX, 0);
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Calling UcLo_WriteUimageAll for router base...";
  if((status = UcLo_WriteUimageAll(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  } 
  std::cout << "done.\n";
*/

/*
  std::cout << "Calling halMe_Start for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;

    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Press enter when you want to stop the microengines.\n";
  std::cin.getline(input,10);
*/

/*
  std::cout << "Calling halMe_Stop for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";
*/

/*
  std::cout << "Press enter when you want to delete the base object.\n";
  std::cin.getline(input,10);
*/

  std::cout << "Calling UcLo_DeleObj for the router base...";
  if((status = UcLo_DeleObj(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  } 
  std::cout << "done.\n";

  std::cout << "Calling halMe_DelLib...";
  halMe_DelLib();
  std::cout << "done.\n";

  return 0;
}
