/******************************************************************************/
/* RadiSys ATCA-70xx Software Development Kit                                 */
/* All of this software and any documentation provided with this software is  */
/* Copyright 2005, 2006 RadiSys Corporation.                                  */
/*                                                                            */
/* All source code provided in this distribution is RadiSys Confidential.     */
/*                                                                            */
/* RADISYS LICENSES THIS SOFTWARE AND ANY DOCUMENTATION PROVIDED WITH THIS    */
/* SOFTWARE TO LICENSEE AS-IS, WITHOUT WARRANTY.  RADISYS DOES NOT MAKE, AND  */
/* LICENSEE EXPRESSLY HEREBY WAIVES, ALL WARRANTIES, EXPRESS OR IMPLIED.      */
/* WITHOUT LIMITING THE FOREGOING, RADISYS DOES NOT MAKE AND EXPRESSLY        */
/* DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         */
/* PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, ACCURACY OF DATA, QUIET       */
/* ENJOYMENT AND FREEDOM FROM DESIGN DEFECT.                                  */
/*                                                                            */
/* This software and any documentation provided with this software is         */
/* subject to United States export control laws and regulations.  Each        */
/* licensee must comply with all applicable United States export control      */
/* laws, including, but not limited to, the rules and regulations of the      */
/* Bureau of Industry and Secruity ("BIS") of the US Department of Commerce   */
/* and the Office of Foreign Assets Control ("OFAC") of the US Department of  */
/* the Treasury.  Such compliance includes, but is not limited to,            */
/* licensee's refraining from the export or re-export, either directly or     */
/* indirectly, of this software and any documentation provided with this      */
/* software or any product made from or with such software or documentation   */
/* without first obtaining any required license or other approval from BIS    */
/* or from any other agency or department of the United States government     */
/* with jurisdiction over such export or re-export.                           */
/*                                                                            */
/* This software and any documentation provided with this software            */
/* constitute a "commericial item," as that term is defined in 48 C.F.R.      */
/* 2.101, consisting of "commercial computer software" and "commercial        */
/* computer software documentation," as such terms are used in 48 C.F.R.      */
/* 12.212.  Consistent with 48 C.F.R. 12.212 and 48 C.F.R. 227.7202-1         */
/* through 227.7202-4, all U.S. government users acquire this software and    */
/* any documentation proivided with this software with only those rights      */
/* provided in the applicable license agreement between RadiSys Corporation   */
/* and the licensee.                                                          */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*    Filename:         common_train.c                                        */
/*                                                                            */
/*    Description:      Common training code for ATCA-7010                    */
/*                                                                            */
/*    Version History:                                                        */
/*                                                                            */
/*    Date                 Comment                          Author            */
/*    ======================================================================  */
/*    05/26/2006           Common training code created     Andrew Watts      */
/*    06/14/2006           Dynamic FIFO sizing              JJS               */
/*    08/15/2006           Dynamic Device Training          AES               */
/*                                                                            */
/*    Functions:                                                              */
/*    Train_SPI_NPU                 Train the SPI switch -> NPU interface     */
/*    Train_NPU_SPI                 Train the NPU -> SPI switch interface     */
/*    Train_RTM                     Train the RTM <-> SPI switch interface    */
/*    Train_FIC                     Train the FIC <-> SPI switch interface    */
/*    openConnections               Open a list of SPI data connections       */
/*    countConnections              Counts the number of conns per interface  */
/*    numConnectionsToFifoSize      Compute size of FIFOs based on connections*/
/*    Train                         The main training function                */
/******************************************************************************/


/*******************************************************************************/
/*                          Includes                                           */
/*******************************************************************************/
#include "common_train.h"

/*******************************************************************************/
/*                          Constants                                          */
/*******************************************************************************/

/*******************************************************************************/
/*                          GLOBALS                                            */
/*******************************************************************************/
/* An array for user-readable names for the SPI switch interfaces */
char *InterfaceNames[9] =
{
   "RTM",
   "FIC",
   "UNUSED1",
   "UNUSED2",
   "NPUA",
   "NPUB",
   "ERROR",
   "ERROR",
   "CPU"
};

/* A structure used to denote what devices are detected (RTM/FIC) */
Misc_Device_Info_t DeviceInfo;

/* Flag denoting whether or not to auto-append the CRC */
UINT32 appendCRC = TRUE;

/* Record the trained state of the NPU interfaces */
UINT32 NPUAInterfaceStartedRx = FALSE;
UINT32 NPUAInterfaceStartedTx = FALSE;
UINT32 NPUBInterfaceStartedRx = FALSE;
UINT32 NPUBInterfaceStartedTx = FALSE;

/******************************************************************************/
/* Function:      Train_SPI_NPU                                               */
/*                                                                            */
/* Description:   Train the SPI Tx -> NPU Rx Interfaces                       */
/*                                                                            */
/* Input:         Processor      - The NPU to train                           */
/*                CalendarLength - The calendar length to use                 */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_TX_SYNC_FAILED                                    */
/*                RC_MISC_INVALID_CALENDAR_LENGTH                             */
/*                RC_MISC_NO_DATA_TRAIN                                       */
/******************************************************************************/
UINT32 Train_SPI_NPU(UINT32 Processor, UINT32 CalendarLength)
{
   PPM_STATUS  Result = FAILURE;
   UINT32      Interface;
   UINT32      Len = CalendarLength;

   UINT32 InterfaceStartedTx;

   if (Processor == PPM_MASTER_PROC)
   {
      printf("\t\tTraining Switch to NPUA ... ");
      Interface = SPI_SWITCH_NPUA_INTERFACE;
      InterfaceStartedTx = NPUAInterfaceStartedTx;
   }
   else if (Processor == PPM_SLAVE_PROC)
   {
      printf("\t\tTraining Switch to NPUB ... ");
      Interface = SPI_SWITCH_NPUB_INTERFACE;
      InterfaceStartedTx = NPUBInterfaceStartedTx;
   }
   else
   {
      return (RC_SWITCH_INVALID_PARAM);
   }

   fflush(stdout);

   /* Call API to start SPI Tx, which starts training             */
   /* Only call StartInterface if it has not already been called  */
   /* during this Train() iteration for this interface            */
   if (InterfaceStartedTx == FALSE)
   {
      Result = SpiSwitchStartInterface(Interface, SPI_SWITCH_TX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\t\tSpiSwitchStartInterface failure 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         if (Processor == PPM_MASTER_PROC)
         {
            NPUAInterfaceStartedTx = TRUE;
         }
         else if (Processor == PPM_SLAVE_PROC)
         {
            NPUBInterfaceStartedTx = TRUE;
         }
      }
   }

   /* Call API to check if the NPU Data training   */
   /* was received and start sending flow control  */
   Result = MiscNPUReceiveTraining(Processor, Len);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tMiscNPUReceiveTraining failure 0x%08lX\n", Result);
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
      printf("Unsuccessful\n\t\t\tMiscNPUHasTrainingStopped failure 0x%08lX\n", Result);
      return (0xFFFFFFFF);
   }

   /* Call API to check if the SPI Tx interface has sync */
   Result = SpiSwitchTxCheckSync(Interface);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tSpiSwitchTxCheckSync failure 0x%08lX\n", Result);
      return (Result);
   }

   printf("DONE\n");
   return (Result);
}

/******************************************************************************/
/* Function:      Train_NPU_SPI                                               */
/*                                                                            */
/* Description:   Train the NPU Tx -> SPI Rx Interfaces                       */
/*                                                                            */
/* Input:         Processor      - The NPU to train                           */
/*                CalendarLength - The calendar length to use                 */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_RX_SYNC_FAILED                                    */
/*                RC_MISC_INVALID_CALENDAR_LENGTH                             */
/*                RC_MISC_INVALID_BASEPORT_NUM                                */
/******************************************************************************/
UINT32 Train_NPU_SPI(UINT32 Processor, UINT32 CalendarLength)
{
   PPM_STATUS  Result = FAILURE;
   UINT32      BasePortNum = 0;
   UINT32      Interface;
   UINT32      Len = CalendarLength;

   UINT32 InterfaceStartedRx;

   if (Processor == PPM_MASTER_PROC)
   {
      printf("\t\tTraining NPUA to Switch ... ");
      Interface = SPI_SWITCH_NPUA_INTERFACE;
      InterfaceStartedRx = NPUAInterfaceStartedRx;
   }
   else if (Processor == PPM_SLAVE_PROC)
   {
      printf("\t\tTraining NPUB to Switch ... ");
      Interface = SPI_SWITCH_NPUB_INTERFACE;
      InterfaceStartedRx = NPUBInterfaceStartedRx;
   }
   else
   {
      return (RC_SWITCH_INVALID_PARAM);
   }

   fflush(stdout);

   /* Call API to start data training from the NPU Tx interface */
   Result = MiscNPUSendTraining(Processor, Len, BasePortNum);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tNPU send training error = 0x%08lX\n", Result);
      return (Result);
   }

   if (InterfaceStartedRx == FALSE)
   {
      /* Call API to start SPI Rx interface */
      /* Only call StartInterface if it has not already been called  */
      /* during this Train() iteration for this interface            */
      Result = SpiSwitchStartInterface(Interface, SPI_SWITCH_RX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\t\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         if (Processor == PPM_MASTER_PROC)
         {
            NPUAInterfaceStartedRx = TRUE;
         }
         else if (Processor == PPM_SLAVE_PROC)
         {
            NPUBInterfaceStartedRx = TRUE;
         }
      }
   }

   /* Call API to check if the SPI Rx interface got sync */
   Result = SpiSwitchRxCheckSync(Interface);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tSwitch RX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }

   /* Call API to stop NPU Tx data training and */
   /* check if SPI Flow Control was received    */
   Result = MiscNPUSendFlowCntl(Processor);

   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tNPU send flow control error = 0x%08lX\n", Result);
      return (Result);
   }

   printf("DONE\n");
   return (Result);
}

/******************************************************************************/
/* Function:      Train_NPU_SPI_Calendar                                      */
/*                                                                            */
/* Description:   Train the NPU Tx -> SPI Rx Interfaces                       */
/*                                                                            */
/* Input:         Processor      - The NPU to train                           */
/*                CalendarLength - The calendar length to use                 */
/*                pCalendar      - The calendar to set                        */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_RX_SYNC_FAILED                                    */
/*                RC_MISC_INVALID_CALENDAR_LENGTH                             */
/*                RC_MISC_INVALID_BASEPORT_NUM                                */
/******************************************************************************/
UINT32 Train_NPU_SPI_Calendar(UINT32 Processor,
                              UINT32 CalendarLength,
                              CALENDAR *pCalendar)
{
   PPM_STATUS  Result = FAILURE;
   UINT32      Interface;
   UINT32      Len = CalendarLength;

   UINT32 InterfaceStartedRx;

   if (Processor == PPM_MASTER_PROC)
   {
      printf("\t\tTraining NPUA to Switch ... ");
      Interface = SPI_SWITCH_NPUA_INTERFACE;
      InterfaceStartedRx = NPUAInterfaceStartedRx;
   }
   else if (Processor == PPM_SLAVE_PROC)
   {
      printf("\t\tTraining NPUB to Switch ... ");
      Interface = SPI_SWITCH_NPUB_INTERFACE;
      InterfaceStartedRx = NPUBInterfaceStartedRx;
   }
   else
   {
      return (RC_SWITCH_INVALID_PARAM);
   }

   fflush(stdout);

   /* Call API to start data training from the NPU Tx interface */
   Result = MiscNPUSendTrainingCalendar(Processor, Len, pCalendar);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tNPU send training error = 0x%08lX\n", Result);
      return (Result);
   }
   if (InterfaceStartedRx == FALSE)
   {
      /* Call API to start SPI Rx interface */
      /* Only call StartInterface if it has not already been called  */
      /* during this Train() iteration for this interface            */
      Result = SpiSwitchStartInterface(Interface, SPI_SWITCH_RX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\t\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         if (Processor == PPM_MASTER_PROC)
         {
            NPUAInterfaceStartedRx = TRUE;
         }
         else if (Processor == PPM_SLAVE_PROC)
         {
            NPUBInterfaceStartedRx = TRUE;
         }
      }
   }

   /* Call API to check if the SPI Rx interface got sync */
   Result = SpiSwitchRxCheckSync(Interface);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tSwitch RX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }

   /* Call API to stop NPU Tx data training and */
   /* check if SPI Flow Control was received    */
   Result = MiscNPUSendFlowCntl(Processor);

   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\t\tNPU send flow control error = 0x%08lX\n", Result);
      return (Result);
   }

   printf("DONE\n");
   return (Result);
}


/******************************************************************************/
/* Function:      Train_RTM                                                   */
/*                                                                            */
/* Description:   Train the RTM <---> SPI Interfaces                          */
/*                                                                            */
/* Input:         maxFrameSize   - The maximum frame size to support          */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_RX_SYNC_FAILED                                    */
/******************************************************************************/
UINT32 Train_RTM(UINT32 maxFrameSize)
{
   PPM_STATUS              Result = FAILURE;
   RSYS_enetIO_XGMAC_Cfg_t RsysXGMACCfg;
   rtm_cfg_t               RTMCfg;
   rtm_status_t            RtmStatus;
   UINT32                  i;
   UINT32                  LongTimeout = 100;      // 10 s in 100 ms increments
   UINT32                  PortStatus = 0;
   UINT32                  PortStatusMask = 0x7FE; // bits 1-10 set

   static UINT32 InterfaceStartedTx = FALSE;
   static UINT32 InterfaceStartedRx = FALSE;

   printf("\t\tTraining RTM to Switch ... ");
   fflush(stdout);

   /* Reset the RTM prior to training it.  Since the driver */
   /* defaults to auto-negotiation mode out of reset, the   */
   /* way to determine the driver is re-initialized is to   */
   /* check that auto-negotiation is complete for all ports */

   if (DeviceInfo.RTM == IS_10x1GbE_RTM_PRESENT)
   {
      /* Call API to reset RTM */
      Result = RtmReset();
      if (Result != RC_SUCCESS)
      {
         printf("RtmReset failure 0x%08lX\n", Result);
         return (Result);
      }
   }
   else if (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT)
   {
      Result = RSYS_EnetIO_Reset(RSYS_ENET_IO_10GbE_RTM);
      if (Result != RSYS_ENET_IO_RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\tRTM reset failure = 0x%08lX\n", Result);
         return(Result);
      }

      /* Set the needed XGMAC settings for both of the two ports  */
      /* that are present on the 2x10 GbE RTM.                    */
      for (i = 1; i <= 2; i++)
      {
         memset(&RsysXGMACCfg, 0, sizeof(RsysXGMACCfg));
         RsysXGMACCfg.portNo = i;
         Result = RSYS_EnetIO_XGMAC_CfgGet(RSYS_ENET_IO_10GbE_RTM, &RsysXGMACCfg);
         if (Result != RSYS_ENET_IO_RC_SUCCESS)
         {
            printf("Unsuccessful\n\t\tRTM could not obtain XGMAC configs = 0x%08lX\n", Result);
            return(Result);
         }

         RsysXGMACCfg.portEnable = RSYS_ENABLE;
         RsysXGMACCfg.maxFrameSzB = maxFrameSize;
         Result = RSYS_EnetIO_XGMAC_CfgSet(RSYS_ENET_IO_10GbE_RTM, &RsysXGMACCfg);
         if (Result != RSYS_ENET_IO_RC_SUCCESS)
         {
            printf("Unsuccessful\n\t\tRTM could not set XGMAC configs = 0x%08lX\n", Result);
            return(Result);
         }
      }
   }
   else
   {
      return (RSYS_ETH_IO_RC_DEV_NOT_FOUND);
   }

   /* Call API to start SPI Tx interface */
   if (InterfaceStartedTx == FALSE)
   {
      Result = SpiSwitchStartInterface(SPI_SWITCH_RTM_INTERFACE,
                                       SPI_SWITCH_TX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED\n\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         InterfaceStartedTx = TRUE;
      }
   }

   /* Call API to check SPI Tx sync */
   Result = SpiSwitchTxCheckSync(SPI_SWITCH_RTM_INTERFACE);
   if (Result != RC_SUCCESS)
   {
      printf("FAILED\n\tSwitch TX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }

   /* Call API to start SPI Rx interface */
   /* Only call StartInterface if it has not already been called  */
   /* during this Train() iteration for this interface            */
   if (InterfaceStartedRx == FALSE)
   {
      Result = SpiSwitchStartInterface(SPI_SWITCH_RTM_INTERFACE,
                                       SPI_SWITCH_RX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED\n\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         InterfaceStartedRx = TRUE;
      }
   }

   /* Call API to check if the SPI Rx interface got sync */
   Result = SpiSwitchRxCheckSync(SPI_SWITCH_RTM_INTERFACE);
   if (Result != RC_SUCCESS)
   {
      printf("FAILED\n\tSwitch RX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }


   /* Sleep for 20 ms = 20000 us to let things settle down */
   usleep(20 * 1000);

   if (DeviceInfo.RTM == IS_10x1GbE_RTM_PRESENT)
   {
      /* Re-apply the default driver settings */
      Result = RtmSPI4Trained();
      if (Result != RC_SUCCESS)
      {
         printf("FAILED\n\tRTM default setting error = 0x%08lX\n", Result);
         return (Result);
      }

      for (i = 1; i <= RTM_MAX_NUMBER_OF_PORTS; i++)
      {
         RTMCfg.portNumber = i;
         Result = RtmGetCfg(&RTMCfg);
         if (Result != RC_SUCCESS)
         {
            printf("FAILED\n\tCouldn't get RTM configuration for port %ld: 0x%08lX\n",
                    i,
                    Result);
            return (Result);
         }

         RTMCfg.configFlag = TX_CONFIG | XGMAC_CONFIG;
         RTMCfg.txConfig.txMacThreshold = 1000;
         if (appendCRC == TRUE)
         {
            RTMCfg.xgmacConfig.xgmac_config.xgmacAutomaticCRCAppending = RTM_TRUE;
         }
         else
         {
            RTMCfg.xgmacConfig.xgmac_config.xgmacAutomaticCRCAppending = RTM_FALSE;
         }
         RTMCfg.xgmacConfig.xgmac_config.xgmacAutoNegotiation = RTM_TRUE;
         RTMCfg.xgmacConfig.xgmac_config.xgmacMaxFrameSize = maxFrameSize;
         Result = RtmSetCfg(&RTMCfg);
         if (Result != RC_SUCCESS)
         {
            printf("FAILED\n\tCouldn't set RTM configuration for port %ld: 0x%08lX\n",
                   i,
                   Result);
            return (Result);
         }
      }


      LongTimeout = 100;      // 10 s in 100 ms increments
      PortStatus = 0;
      PortStatusMask = 0x7FE; // bits 1-10 set

      /* 8/24/06 - JJS - Check the LossOfSync in the RTM status    */
      /* to determine if the link is up or not.  If the link is    */
      /* not up, wait up to 10s for it to come up.  If the link    */
      /* does not come up after that time, continue through the    */
      /* training process.  This is done to give the links,        */
      /* especially the copper ones, enough time to complete auto- */
      /* negotiation.                                              */
      /* Check the link up/down status for port i and set bit i in */
      /* PortStatus if it is set                                   */
      while ((LongTimeout != 0) && (PortStatus != PortStatusMask))
      {
         for (i = 1; i <= RTM_MAX_NUMBER_OF_PORTS; i++)
         {
            /* Clear the status structure before making the call */
            memset(&RtmStatus, 0, sizeof(RtmStatus));

            RtmStatus.portNumber = i;
            Result = RtmGetStatus(&RtmStatus);
            if (Result != RC_SUCCESS)
            {
               printf("FAILED\n\tRtmGetStatus (port %ld) failure 0x%08lX\n", i, Result);
               return (Result);
            }

            /* If the AN is complete for that port, set the bit */
            if ((RtmStatus.xgmacStatus.xgmacLossOfSync == RTM_TRUE) &&
                (RtmStatus.xgmacStatus.xgmacANComplete == RTM_TRUE))
               PortStatus |= (1 << i);
            else
               PortStatus &= ~(1 << i);
         }

         /* Sleep for 100 ms = 100000 us and decrement the timeout counter */
         usleep(100000);
         LongTimeout--;
      }
   }
   else if (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT)
   {
      RSYS_enetIO_XGMAC_Status_t portStatus[2];
      Result = RSYS_EnetIO_TrainingComplete(RSYS_ENET_IO_10GbE_RTM);
      if (Result != RSYS_ENET_IO_RC_SUCCESS)
      {
         printf("FAILED\n\tUnable to complete RTM training = 0x%08lX\n", Result);
         return(Result);
      }

      LongTimeout = 100;      // 10 s in 100 ms increments
      /* Check for link up on both 2x10 GbE RTM ports */
      do
      {
         for(i = 1; i <= 2; i++)
         {
            portStatus[i-1].portNo = i;
            Result = RSYS_EnetIO_XGMAC_StatusGet(RSYS_ENET_IO_10GbE_RTM,
                                                 &(portStatus[i-1]));
            if (Result != RSYS_ENET_IO_RC_SUCCESS)
            {
               printf("FAILED\n\tUnable to get port status.  Error code = 0x%08lX\n", Result);
               return(Result);
            }
         }
         usleep(100000);
         LongTimeout--;
      } while ((((portStatus[0].portState & RSYS_ENET_IO_XGMAC_RX_LINK_DOWN) ==
                 RSYS_ENET_IO_XGMAC_RX_LINK_DOWN) ||
                ((portStatus[1].portState & RSYS_ENET_IO_XGMAC_RX_LINK_DOWN) ==
                 RSYS_ENET_IO_XGMAC_RX_LINK_DOWN)) &&
               (LongTimeout > 0));
   }
   else
   {
      //This condition should never be reached here
      //It should have been caught above
      return (RSYS_ETH_IO_RC_DEV_NOT_FOUND);
   }

   /* Set the static "interface started" variables to false       */
   /* so if we run Train() multiple times in the same executable, */
   /* it will start the interface the second time around          */
   InterfaceStartedTx = FALSE;
   InterfaceStartedRx = FALSE;

   printf("DONE\n");
   return (SUCCESS);
}

/******************************************************************************/
/* Function:      Train_FIC                                                   */
/*                                                                            */
/* Description:   Train the FIC <---> SPI Interfaces                          */
/*                                                                            */
/* Input:         MACEnable      -  Flag denoting whether or not to enable    */
/*                                  the XGMAC on the FIC                      */
/*                LoopEnable     -  Flag denoting whether or not to set the   */
/*                                  in loopback mode                          */
/*                maxFrameSize   -  The maximum frame size to support         */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_RX_SYNC_FAILED                                    */
/******************************************************************************/
UINT32 Train_FIC(BOOL32 MACEnable, BOOL32 LoopEnable, UINT32 maxFrameSize)
{
   int                     i;
   PPM_STATUS              Result = FAILURE;
   RSYS_enetIO_XGMAC_Cfg_t RsysXGMACCfg;
   UINT32                  HardwareType;
   UINT32                  MaxPort;

   static UINT32 InterfaceStartedTx = FALSE;
   static UINT32 InterfaceStartedRx = FALSE;

   /* Determine the FIC characteristics */
   if (DeviceInfo.FIC == IS_10GbE_FIC_PRESENT)
   {
      HardwareType = RSYS_ENET_IO_10GbE_FIC;
      MaxPort = 2;
   }
   else if (DeviceInfo.FIC == IS_4GbE_FIC_PRESENT)
   {
      HardwareType = RSYS_ENET_IO_4GbE_FIC;
      MaxPort = 8;
   }
   else
   {
      return (RSYS_ENET_IO_RC_RESET_FAILED);
   }

   /* Reset FIC if present */
   Result = RSYS_EnetIO_Reset(HardwareType);
   if (Result != RSYS_ENET_IO_RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\tFIC reset failure = 0x%08lX\n", Result);
      return(Result);
   }

   /* Apply the non-default FIC configuration settings as needed */
   for (i = 1; i <= MaxPort; i++)
   {
      if ((MACEnable == TRUE) || (LoopEnable == TRUE))
      {
         memset(&RsysXGMACCfg, 0, sizeof(RsysXGMACCfg));
         RsysXGMACCfg.portNo = i;
         Result = RSYS_EnetIO_XGMAC_CfgGet(HardwareType, &RsysXGMACCfg);
         if (Result != RSYS_ENET_IO_RC_SUCCESS)
         {
            printf("Unsuccessful\n\t\tFIC could not obtain XGMAC configs = 0x%08lX\n", Result);
            return(Result);
         }

         if (MACEnable == TRUE)
         {
             RsysXGMACCfg.portEnable = RSYS_ENABLE;
         }

         if (LoopEnable == TRUE)
         {
             RsysXGMACCfg.loopbackEnable = RSYS_ENABLE;
         }

         RsysXGMACCfg.maxFrameSzB = maxFrameSize;
         Result = RSYS_EnetIO_XGMAC_CfgSet(HardwareType, &RsysXGMACCfg);
         if (Result != RSYS_ENET_IO_RC_SUCCESS)
         {
            printf("Unsuccessful\n\t\tFIC could not set XGMAC configs = 0x%08lX\n", Result);
            return(Result);
         }
      }
   }

   /* Call API to start SPI Tx interface */
   if (InterfaceStartedTx == FALSE)
   {
      Result = SpiSwitchStartInterface(SPI_SWITCH_FIC_INTERFACE,
                                       SPI_SWITCH_TX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         InterfaceStartedTx = TRUE;
      }
   }

   /* Call API to start SPI Rx interface */
   if (InterfaceStartedRx == FALSE)
   {
      Result = SpiSwitchStartInterface(SPI_SWITCH_FIC_INTERFACE,
                                       SPI_SWITCH_RX_INTERFACE);
      if (Result != RC_SUCCESS)
      {
         printf("Unsuccessful\n\t\tSwitch interface start error = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         InterfaceStartedRx = TRUE;
      }
   }

   /* Call API to check if the SPI Rx interface got sync */
   Result = SpiSwitchRxCheckSync(SPI_SWITCH_FIC_INTERFACE);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\tSwitch RX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }

   /* Call API to check if the SPI Tx interface got sync */
   Result = SpiSwitchTxCheckSync(SPI_SWITCH_FIC_INTERFACE);
   if (Result != RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\tSwitch TX synchronization check error = 0x%08lX\n", Result);
      return (Result);
   }

   /* Call API to restore default settings following training */
   Result = RSYS_EnetIO_TrainingComplete(HardwareType);
   if (Result != RSYS_ENET_IO_RC_SUCCESS)
   {
      printf("Unsuccessful\n\t\tUnable to complete FIC training = 0x%08lX\n", Result);
      return(Result);
   }

   printf("DONE\n");

   /* Set the static "interface started" variables to false       */
   /* so if we run Train() multiple times in the same executable, */
   /* it will start the interface the second time around          */
   InterfaceStartedTx = FALSE;
   InterfaceStartedRx = FALSE;
   return(RC_SUCCESS);
}

/******************************************************************************/
/* Function:      openConnections                                             */
/*                                                                            */
/* Description:   Open a list of connections                                  */
/*                                                                            */
/* Input:         connections       - An array of connections to open         */
/*                connectionCount   - The number of connections to open       */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/******************************************************************************/
UINT32 openConnections(Connection connections[], UINT32 connectionCount)
{
   INT32  result;
   UINT32 i;

   for (i = 0; i < connectionCount; i++)
   {
      printf("Opening Connection from %s-port%lu to %s-port%lu ... ",
             InterfaceNames[connections[i].rxInterface],
             connections[i].rxPort,
             InterfaceNames[connections[i].txInterface],
             connections[i].txPort);
      result = SpiSwitchOpenConnection(connections[i].rxPort,
                                       connections[i].rxInterface,
                                       connections[i].txInterface,
                                       connections[i].txPort);
      if (result != RC_SUCCESS)
      {
         printf("FAILED, result = 0x%08lX\n", result);
         return (result);
      }
      else
      {
         printf("DONE\n");
      }
   }

   return(RC_SUCCESS);
}

/******************************************************************************/
/* Function:      countConnections                                            */
/*                                                                            */
/* Description:   Count the number of connections to the "Direction" side of  */
/*                "Interface" in the connections array                        */
/*                                                                            */
/* Input:         connections       - An array of connections to check        */
/*                connectionCount   - The number of connections to check      */
/*                Interface         - The interface to check against.  Valid  */
/*                                    values are:                             */
/*                                          SPI_SWITCH_NPUA_INTERFACE         */
/*                                          SPI_SWITCH_NPUB_INTERFACE         */
/*                                          SPI_SWITCH_RTM_INTERFACE          */
/*                                          SPI_SWITCH_FIC_INTERFACE          */
/*                Direction         - The direction of the interface.  Valid  */
/*                                    values are:                             */
/*                                          SPI_SWITCH_RX_INTERFACE           */
/*                                          SPI_SWITCH_TX_INTERFACE           */
/*                                                                            */
/* Output:        The number of connections that match (0 if none match)      */
/******************************************************************************/
UINT32 countConnections(Connection connections[],
                        UINT32 connectionCount,
                        UINT32 Interface,
                        UINT32 Direction)
{
   int i;
   UINT32 matchCount = 0;

   if (Direction == SPI_SWITCH_RX_INTERFACE)
   {
      for (i = 0; i < connectionCount; i++)
      {
         if (connections[i].rxInterface == Interface)
         {
            matchCount++;
         }
      }
      return matchCount;
   }
   else if (Direction == SPI_SWITCH_TX_INTERFACE)
   {
      for (i = 0; i < connectionCount; i++)
      {
         if (connections[i].txInterface == Interface)
         {
            matchCount++;
         }
      }
      return matchCount;
   }
   return 0;
}

/******************************************************************************/
/* Function:      numConnectionsToFifoSize                                    */
/*                                                                            */
/* Description:   Converts the number of connections into a FIFO size         */
/*                                                                            */
/* Input:         numberConnections - The number of connections to open       */
/*                                                                            */
/* Output:        The size of the FIFO to use, -1 on error, and 0 (1KB FIFOs) */
/*                for zero connections                                        */
/******************************************************************************/
UINT32 numConnectionsToFifoSize(UINT32 numberConnections)
{
   if (numberConnections == 0)
      return 0;

   /* Split the available 16 KB among the connections.  The following   */
   /* should be used:                                                   */
   /*    1 connection      :  16 KB FIFO                                */
   /*    2 connections     :  8 KB FIFOs                                */
   /*    3-4 connections   :  4 KB FIFOs                                */
   /*    5-8 connections   :  2 KB FIFOs                                */
   /*    9-16 connections  :  1 KB FIFOs                                */
   /* This can be computed dynamically as 16/numberConnections.         */
   /* For completeness, fill in the other possible values, even though  */
   /* they can't actually be achieved due to integer division.          */
   switch(16/numberConnections)
   {
      // 1KB FIFOs
      case 1:
         return 0;

      // 2KB FIFOs
      case 2:
      case 3:
         return 1;

      // 4KB FIFOs
      case 4:
      case 5:
      case 6:
      case 7:
         return 2;

      // 8KB FIFOs
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
         return 3;

      // 16KB FIFOs
      case 16:
         return 4;

      // error
      default:
         return -1;
    }
}

/******************************************************************************/
/* Function:      Train                                                       */
/*                                                                            */
/* Description:   The main SPI interface training function                    */
/*                                                                            */
/* Input:         connectionType - character specifying whether to make       */
/*                                 internal or system connections             */
/*                useFIC         - Flag denoting whether to include a FIC in  */
/*                                 the configuration                          */
/*                rxLoWaterMark  - The low watermark value to use on the      */
/*                                 receive side of the SPI Switch interfaces. */
/*                rxLoWaterMark  - The high watermark value to use on the     */
/*                                 receive side of the SPI Switch interfaces. */
/*                rtmClkMult     - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch RTM I/F    */
/*                npuAClkMult    - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch NPU A I/F  */
/*                npuBClkMult    - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch NPU B I/F  */
/*                maxFrameSize   - The maximum frame size to support          */
/*                FICLoopEnable  - Flag denoting whether to put the FIC in    */
/*                                 internal loopback mode                     */
/*                useTenGbECalLen- Flag denoting whether to use the default   */
/*                                 calendar length for the 10GbE devices      */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/*                RC_SWITCH_RESET_ERROR                                       */
/*                RC_SWITCH_INVALID_HANDLE                                    */
/*                RC_SWITCH_INVALID_INTERFACE                                 */
/*                RC_SWITCH_INVALID_PARAM                                     */
/*                RC_SWITCH_INTERFACE_NOT_RESET                               */
/*                RC_SWITCH_TX_PLL_LOCK_FAILED                                */
/*                RC_SWITCH_INTERFACE_NOT_CONFIGURED                          */
/*                RC_SWITCH_NO_AVAILABLE_CONNECTIONS                          */
/******************************************************************************/
UINT32 Train(Connection *connections,
             UINT32 connectionCount,
             UINT32 deviceMask,
             UINT32 rxLoWaterMark,
             UINT32 rxHiWaterMark,
             UINT32 rtmClkMult,
             UINT32 ficClkMult,
             UINT32 npuAClkMult,
             UINT32 npuBClkMult,
             UINT32 maxFrameSize,
             UINT32 FICLoopEnable,
             UINT32 useTenGbECalLen)
{
   BOOL32         useFIC = FALSE;
   BOOL32         useNPUA = FALSE;
   BOOL32         useNPUB = FALSE;
   BOOL32         useRTM = FALSE;
   InterfaceType  FicSpiInterfaceSettings;
   InterfaceType  NpuSpiInterfaceSettings;
   InterfaceType  RtmSpiInterfaceSettings;
   PPM_STATUS     Result = FAILURE;
   UINT32         i;
   UINT32         NoDefaults = 0;
   UINT32         RxTxMask = SPI_SWITCH_RX_INTERFACE | SPI_SWITCH_TX_INTERFACE;
   UINT32         TrainingAttempt = 0;

   /* SPI Switch config values */
   UINT32 AllRxInts        = 0xFF;
   UINT32 AllTxInts        = 0xF;
   UINT32 CalLen           = 10;
   UINT32 CalMult          = 1;
   UINT32 CalReg0          = 0x76543210;
   UINT32 CalReg1          = 0xFEDCBA98;
   UINT32 FallingClockEdge = 0;
   UINT32 TenGbECalLen     = 16;
   UINT32 TenGbECalReg0    = 0x10101010;
   UINT32 TenGbECalReg1    = 0x10101010;
   UINT32 FifoSize         = 0;
   UINT32 MaxBurst1        = 8;
   UINT32 MaxBurst2        = 4;
   UINT32 MaxData          = 0;
   UINT32 MultiPort        = 0;
   UINT32 NPUATxCalLen     = CalLen;
   UINT32 NPUARxCalLen     = CalLen;
   UINT32 NPUBTxCalLen     = CalLen;
   UINT32 NPUBRxCalLen     = CalLen;
   UINT32 NumConnections   = 0;
   UINT32 RisingClockEdge  = 1;
   UINT32 RxHiWater        = rxHiWaterMark;
   UINT32 RxLoWater        = rxLoWaterMark;
   UINT32 ServiceLimit     = 0x804;
   UINT32 TxHiWater        = 5;
   UINT32 TxLoWater        = 2;
   UINT32 TrainingReps     = 2;
   UINT32 Unused           = 0;

   NPUAInterfaceStartedRx = FALSE;
   NPUAInterfaceStartedTx = FALSE;
   NPUBInterfaceStartedRx = FALSE;
   NPUBInterfaceStartedTx = FALSE;

   CALENDAR npuATxCalendar;
   CALENDAR npuBTxCalendar;

   /* Determine which devices to train */
   if (deviceMask & TRAIN_FIC)
      useFIC = TRUE;
   if (deviceMask & TRAIN_NPUA)
      useNPUA = TRUE;
   if (deviceMask & TRAIN_NPUB)
      useNPUB = TRUE;
   if (deviceMask & TRAIN_RTM)
      useRTM = TRUE;

   Result = MiscGetDeviceInfo(&DeviceInfo);
   if (Result != RC_SUCCESS)
   {
      printf("Failed to detect which FIC and RTM are present: result = 0x%08lX\n", Result);
      return(Result);
   }

   /* Reset the switch */
   printf("Resetting SPI switch ... ");
   Result = SpiSwitchReset();
   if (Result != RC_SUCCESS)
   {
      printf("FAILED, result = 0x%08lX\n", Result);
      return(Result);
   }
   else
   {
      printf("DONE\n");
   }

   /***************************/
   /* Interface configuration */
   /***************************/
   /* Common settings for NPU A and NPU B */
   if ((useNPUA == TRUE) || (useNPUB == TRUE))
   {
      UINT8 index;

      /* Initialize the variables for the SPI Switch interface connecting to the NPUs */
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
      NpuSpiInterfaceSettings.txInterface.clockMultiplier            = npuAClkMult;
      NpuSpiInterfaceSettings.txInterface.trainingRepetitions        = TrainingReps;
      NpuSpiInterfaceSettings.txInterface.dataMaxT                   = MaxData;
      NpuSpiInterfaceSettings.txInterface.serviceLimit               = ServiceLimit;

      /* Initialize the variables for the NPU SPI interfaces */
      NPUARxCalLen = NpuSpiInterfaceSettings.txInterface.calendarLength;
      NPUATxCalLen = NpuSpiInterfaceSettings.rxInterface.calendarLength;
      memset(&npuATxCalendar, 0, sizeof(npuATxCalendar));
      for ( index=0; index<NPUARxCalLen; index++ )
      {
         npuATxCalendar[index] = index;
      }

      NPUBRxCalLen = NpuSpiInterfaceSettings.txInterface.calendarLength;
      NPUBTxCalLen = NpuSpiInterfaceSettings.rxInterface.calendarLength;
      memset(&npuBTxCalendar, 0, sizeof(npuBTxCalendar));
      for ( index=0; index<NPUBRxCalLen; index++ )
      {
         npuBTxCalendar[index] = index;
      }
   }

   /* Configure the interface for NPU A, if used */
   if (useNPUA == TRUE)
   {
      /* If the user wants to use the 2x10GbE calendar and length ... */
      if ( useTenGbECalLen == TRUE )
      {
         /* ... and there is a 2x10GbE RTM present ... */
         if ( (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT) || (DeviceInfo.RTM == RTM_NOT_PRESENT) )
         {
            UINT8 index;

            /* ... then set the calendar and length on the NPUA TX interface to match */
            NPUATxCalLen = TenGbECalLen;
            for (index=0; index<NPUATxCalLen; index++)
            {
               npuATxCalendar[index] = index%2;
            }

            /* ... and set the calendar and length on the SPI switch RX interface connected to NPUA to match */
            NpuSpiInterfaceSettings.rxInterface.calendarLength = NPUATxCalLen;
            NpuSpiInterfaceSettings.rxInterface.calendar[0]    = TenGbECalReg0;
            NpuSpiInterfaceSettings.rxInterface.calendar[1]    = TenGbECalReg1;
         }

         /* ... and there is a 2x10GbE FIC present ... */
         if ( (DeviceInfo.FIC == IS_10GbE_FIC_PRESENT) || (DeviceInfo.FIC == FIC_NOT_PRESENT))
         {
            /* ... then set the calendar and length on the NPUA RX interface to match */
            NPUARxCalLen = TenGbECalLen;

            /* ... and set the calendar and length on the SPI switch TX interface connected to NPUA to match */
            NpuSpiInterfaceSettings.txInterface.calendarLength = NPUARxCalLen;
            NpuSpiInterfaceSettings.txInterface.calendar[0]    = TenGbECalReg0;
            NpuSpiInterfaceSettings.txInterface.calendar[1]    = TenGbECalReg1;
         }
      }

      /* Automatically determine NPU A RX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_NPUA_INTERFACE,
                                        SPI_SWITCH_RX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      NpuSpiInterfaceSettings.rxInterface.fifoSize = FifoSize;
      NpuSpiInterfaceSettings.rxInterface.watermarks.lowWatermark <<= FifoSize;
      NpuSpiInterfaceSettings.rxInterface.watermarks.highWatermark <<= FifoSize;

      /* Automatically determine NPU A TX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_NPUA_INTERFACE,
                                        SPI_SWITCH_TX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      NpuSpiInterfaceSettings.txInterface.fifoSize = FifoSize;

      /* Automatically adjusts the service limit and max bursts */
      NpuSpiInterfaceSettings.txInterface.serviceLimit = ServiceLimit << FifoSize;

      /* Mark unused calendar entries (we only use the first two) */
      for (i = 2; i < SPI_SWITCH_MAX_CALENDAR_ENTRIES; i++)
      {
         NpuSpiInterfaceSettings.rxInterface.calendar[i] = Unused;
         NpuSpiInterfaceSettings.txInterface.calendar[i] = Unused;
      }

      for (i = 0; i < SPI_SWITCH_MAX_FIFOS_PER_INTERFACE; i++)
      {
         NpuSpiInterfaceSettings.txInterface.maxBurst1[i] = MaxBurst1 << FifoSize;
         NpuSpiInterfaceSettings.txInterface.maxBurst2[i] = MaxBurst2 << FifoSize;
      }

      /* Configure the interface for NPU A */
      printf("Configuring Switch interfaces connected to NPUA ... ");
      Result = SpiSwitchConfigureInterface(SPI_SWITCH_NPUA_INTERFACE,
                                           RxTxMask,
                                           NoDefaults,
                                           &NpuSpiInterfaceSettings);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED, result = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         printf("DONE\n");
      }
   }

   /* Configure the interface for NPU B, if used */
   if (useNPUB == TRUE)
   {
      /* Reset the NPU SPI interface settings descriptor */
      NpuSpiInterfaceSettings.rxInterface.calendarLength = CalLen;
      NpuSpiInterfaceSettings.rxInterface.calendar[0]    = CalReg0;
      NpuSpiInterfaceSettings.rxInterface.calendar[1]    = CalReg1;

      NpuSpiInterfaceSettings.txInterface.calendarLength = CalLen;
      NpuSpiInterfaceSettings.txInterface.calendar[0]    = CalReg0;
      NpuSpiInterfaceSettings.txInterface.calendar[1]    = CalReg1;

      /* If the user wants to use the 10GbE calendar and length ... */
      if ( useTenGbECalLen == TRUE )
      {
         /* ... and there is a 2x10GbE FIC present ... */
         if ( (DeviceInfo.FIC == IS_10GbE_FIC_PRESENT) || (DeviceInfo.FIC == FIC_NOT_PRESENT))
         {
            UINT8 index;

            /* ... then set the calendar and length on the NPUB TX interface to match */
            NPUBTxCalLen = TenGbECalLen;
            for (index=0; index<NPUBTxCalLen; index++)
            {
               npuBTxCalendar[index] = index%2;
            }

            /* ... and set the calendar and length on the SPI switch RX interface connected to NPUB to match */
            NpuSpiInterfaceSettings.rxInterface.calendarLength = NPUBTxCalLen;
            NpuSpiInterfaceSettings.rxInterface.calendar[0]    = TenGbECalReg0;
            NpuSpiInterfaceSettings.rxInterface.calendar[1]    = TenGbECalReg1;
         }

         /* ... and there is a 2x10GbE RTM present ... */
         if ( (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT) || (DeviceInfo.RTM == RTM_NOT_PRESENT))
         {
            /* ... then set the calendar and length on the NPUB RX interface to match */
            NPUBRxCalLen = TenGbECalLen;

            /* ... and set the calendar and length on the SPI switch TX interface connected to NPUB to match */
            NpuSpiInterfaceSettings.txInterface.calendarLength = NPUBRxCalLen;
            NpuSpiInterfaceSettings.txInterface.calendar[0]    = TenGbECalReg0;
            NpuSpiInterfaceSettings.txInterface.calendar[1]    = TenGbECalReg1;
         }
      }

      NpuSpiInterfaceSettings.rxInterface.watermarks.lowWatermark    = RxLoWater;
      NpuSpiInterfaceSettings.rxInterface.watermarks.highWatermark   = RxHiWater;
      NpuSpiInterfaceSettings.txInterface.watermarks.lowWatermark    = TxLoWater;
      NpuSpiInterfaceSettings.txInterface.watermarks.highWatermark   = TxHiWater;

      /* Set the transmit clock multiplier and configure the NPU B interface */
      NpuSpiInterfaceSettings.txInterface.clockMultiplier = npuBClkMult;

      /* Automatically determine NPU B RX FIFO sizeand watermark levels */
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_NPUB_INTERFACE,
                                        SPI_SWITCH_RX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      NpuSpiInterfaceSettings.rxInterface.fifoSize = FifoSize;
      NpuSpiInterfaceSettings.rxInterface.watermarks.lowWatermark <<= FifoSize;
      NpuSpiInterfaceSettings.rxInterface.watermarks.highWatermark <<= FifoSize;

      /* Automatically determine NPU B TX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_NPUB_INTERFACE,
                                        SPI_SWITCH_TX_INTERFACE);
      FifoSize = numConnectionsToFifoSize(NumConnections);
      NpuSpiInterfaceSettings.txInterface.fifoSize = FifoSize;

      /* Automatically adjusts the service limit and max bursts */
      NpuSpiInterfaceSettings.txInterface.serviceLimit = ServiceLimit << FifoSize;

      /* Mark unused calendar entries (we only use the first two) */
      for (i = 2; i < SPI_SWITCH_MAX_CALENDAR_ENTRIES; i++)
      {
         NpuSpiInterfaceSettings.rxInterface.calendar[i] = Unused;
         NpuSpiInterfaceSettings.txInterface.calendar[i] = Unused;
      }

      for (i = 0; i < SPI_SWITCH_MAX_FIFOS_PER_INTERFACE; i++)
      {
         NpuSpiInterfaceSettings.txInterface.maxBurst1[i] = MaxBurst1 << FifoSize;
         NpuSpiInterfaceSettings.txInterface.maxBurst2[i] = MaxBurst2 << FifoSize;
      }

      /* Configure the interface for NPU B */
      printf("Configuring Switch interfaces connected to NPUB ... ");
      Result = SpiSwitchConfigureInterface(SPI_SWITCH_NPUB_INTERFACE,
                                           RxTxMask,
                                           NoDefaults,
                                           &NpuSpiInterfaceSettings);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED, result = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         printf("DONE\n");
      }
   }

   /* Configure the interface for the RTM, if used */
   if (useRTM == TRUE)
   {
      RtmSpiInterfaceSettings.rxInterface.calendarMultiplicity       = CalMult;
      RtmSpiInterfaceSettings.rxInterface.interruptMask              = AllRxInts;
      RtmSpiInterfaceSettings.rxInterface.operationMode              = MultiPort;
      RtmSpiInterfaceSettings.rxInterface.watermarks.lowWatermark    = RxLoWater;
      RtmSpiInterfaceSettings.rxInterface.watermarks.highWatermark   = RxHiWater;
      RtmSpiInterfaceSettings.rxInterface.clockEdge                  = FallingClockEdge;

      RtmSpiInterfaceSettings.txInterface.calendarMultiplicity       = CalMult;
      RtmSpiInterfaceSettings.txInterface.interruptMask              = AllTxInts;
      RtmSpiInterfaceSettings.txInterface.operationMode              = MultiPort;
      RtmSpiInterfaceSettings.txInterface.watermarks.lowWatermark    = TxLoWater;
      RtmSpiInterfaceSettings.txInterface.watermarks.highWatermark   = TxHiWater;
      RtmSpiInterfaceSettings.txInterface.clockEdge                  = FallingClockEdge;
      RtmSpiInterfaceSettings.txInterface.clockMultiplier            = rtmClkMult;
      RtmSpiInterfaceSettings.txInterface.trainingRepetitions        = TrainingReps;
      RtmSpiInterfaceSettings.txInterface.dataMaxT                   = MaxData;
      RtmSpiInterfaceSettings.txInterface.serviceLimit               = ServiceLimit;

      /* Configure the SPI interface connecting to the RTM */
      /* Adjust the SPI calendar as needed */
      if (DeviceInfo.RTM == IS_10x1GbE_RTM_PRESENT)
      {
         RtmSpiInterfaceSettings.rxInterface.calendarLength             = CalLen;
         RtmSpiInterfaceSettings.rxInterface.calendar[0]                = CalReg0;
         RtmSpiInterfaceSettings.rxInterface.calendar[1]                = CalReg1;

         RtmSpiInterfaceSettings.txInterface.calendarLength             = CalLen;
         RtmSpiInterfaceSettings.txInterface.calendar[0]                = CalReg0;
         RtmSpiInterfaceSettings.txInterface.calendar[1]                = CalReg1;
      }
      else if (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT)
      {
         RtmSpiInterfaceSettings.rxInterface.calendarLength             = 16;
         RtmSpiInterfaceSettings.rxInterface.calendar[0]                = 0x10101010;
         RtmSpiInterfaceSettings.rxInterface.calendar[1]                = 0x10101010;

         RtmSpiInterfaceSettings.txInterface.calendarLength             = 16;
         RtmSpiInterfaceSettings.txInterface.calendar[0]                = 0x10101010;
         RtmSpiInterfaceSettings.txInterface.calendar[1]                = 0x10101010;
      }

      /* Automatically determine RTM RX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_RTM_INTERFACE,
                                        SPI_SWITCH_RX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      RtmSpiInterfaceSettings.rxInterface.fifoSize = FifoSize;
      RtmSpiInterfaceSettings.rxInterface.watermarks.lowWatermark <<= FifoSize;
      RtmSpiInterfaceSettings.rxInterface.watermarks.highWatermark <<= FifoSize;

      /* Automatically determine RTM TX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_RTM_INTERFACE,
                                        SPI_SWITCH_TX_INTERFACE);
      FifoSize = numConnectionsToFifoSize(NumConnections);
      RtmSpiInterfaceSettings.txInterface.fifoSize = FifoSize;

      /* Automatically adjusts the service limit and max bursts */
      RtmSpiInterfaceSettings.txInterface.serviceLimit = ServiceLimit << FifoSize;

      /* Mark unused calendar entries (we only use the first two) */
      for (i = 2; i < SPI_SWITCH_MAX_CALENDAR_ENTRIES; i++)
      {
         RtmSpiInterfaceSettings.rxInterface.calendar[i] = Unused;
         RtmSpiInterfaceSettings.txInterface.calendar[i] = Unused;
      }

      for (i = 0; i < SPI_SWITCH_MAX_FIFOS_PER_INTERFACE; i++)
      {
         RtmSpiInterfaceSettings.txInterface.maxBurst1[i] = MaxBurst1 << FifoSize;
         RtmSpiInterfaceSettings.txInterface.maxBurst2[i] = MaxBurst2 << FifoSize;
      }

      printf("Configuring Switch interfaces connected to the RTM ... ");
      Result = SpiSwitchConfigureInterface(SPI_SWITCH_RTM_INTERFACE,
                                           RxTxMask,
                                           NoDefaults,
                                           &RtmSpiInterfaceSettings);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED, result = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         printf("DONE\n");
      }
   }

   /* Configure the FIC interface, if specified */
   if (useFIC == TRUE)
   {
      /* Configure the SPI interface connecting to the FIC */
      FicSpiInterfaceSettings.rxInterface.calendarLength             = CalLen;
      FicSpiInterfaceSettings.rxInterface.calendarMultiplicity       = CalMult;
      FicSpiInterfaceSettings.rxInterface.interruptMask              = AllRxInts;
      FicSpiInterfaceSettings.rxInterface.operationMode              = MultiPort;
      FicSpiInterfaceSettings.rxInterface.calendar[0]                = CalReg0;
      FicSpiInterfaceSettings.rxInterface.calendar[1]                = CalReg1;
      FicSpiInterfaceSettings.rxInterface.watermarks.lowWatermark    = RxLoWater;
      FicSpiInterfaceSettings.rxInterface.watermarks.highWatermark   = RxHiWater;
      FicSpiInterfaceSettings.rxInterface.clockEdge                  = FallingClockEdge;
      FicSpiInterfaceSettings.rxInterface.watermarks.lowWatermark    = RxLoWater;
      FicSpiInterfaceSettings.rxInterface.watermarks.highWatermark   = RxHiWater;

      FicSpiInterfaceSettings.txInterface.calendarLength             = CalLen;
      FicSpiInterfaceSettings.txInterface.calendarMultiplicity       = CalMult;
      FicSpiInterfaceSettings.txInterface.interruptMask              = AllTxInts;
      FicSpiInterfaceSettings.txInterface.operationMode              = MultiPort;
      FicSpiInterfaceSettings.txInterface.calendar[0]                = CalReg0;
      FicSpiInterfaceSettings.txInterface.calendar[1]                = CalReg1;
      FicSpiInterfaceSettings.txInterface.watermarks.lowWatermark    = TxLoWater;
      FicSpiInterfaceSettings.txInterface.watermarks.highWatermark   = TxHiWater;
      FicSpiInterfaceSettings.txInterface.clockEdge                  = FallingClockEdge;
      FicSpiInterfaceSettings.txInterface.clockMultiplier            = ficClkMult;
      FicSpiInterfaceSettings.txInterface.trainingRepetitions        = TrainingReps;
      FicSpiInterfaceSettings.txInterface.dataMaxT                   = MaxData;
      FicSpiInterfaceSettings.txInterface.serviceLimit               = ServiceLimit;
      FicSpiInterfaceSettings.txInterface.watermarks.lowWatermark    = TxLoWater;
      FicSpiInterfaceSettings.txInterface.watermarks.highWatermark   = TxHiWater;

      if (DeviceInfo.FIC == IS_10GbE_FIC_PRESENT)
      {
         FicSpiInterfaceSettings.rxInterface.calendarLength = 16;
         FicSpiInterfaceSettings.txInterface.calendarLength = 16;
         FicSpiInterfaceSettings.txInterface.calendar[0]    = 0x10101010;
         FicSpiInterfaceSettings.txInterface.calendar[1]    = 0x10101010;
         FicSpiInterfaceSettings.rxInterface.calendar[0]    = 0x10101010;
         FicSpiInterfaceSettings.rxInterface.calendar[1]    = 0x10101010;
      }

      else if (DeviceInfo.FIC == IS_4GbE_FIC_PRESENT)
      {
         // Do nothing; unsupported for now
      }

      /* Automatically determine FIC RX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_FIC_INTERFACE,
                                        SPI_SWITCH_RX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      FicSpiInterfaceSettings.rxInterface.fifoSize = FifoSize;
      FicSpiInterfaceSettings.rxInterface.watermarks.lowWatermark <<= FifoSize;
      FicSpiInterfaceSettings.rxInterface.watermarks.highWatermark <<= FifoSize;

      /* Automatically determine FIC TX FIFO size and watermark levels*/
      NumConnections = countConnections(connections,
                                        connectionCount,
                                        SPI_SWITCH_FIC_INTERFACE,
                                        SPI_SWITCH_TX_INTERFACE);

      FifoSize = numConnectionsToFifoSize(NumConnections);
      FicSpiInterfaceSettings.txInterface.fifoSize = FifoSize;

      /* Automatically adjusts the service limit and max bursts */
      FicSpiInterfaceSettings.txInterface.serviceLimit = ServiceLimit << FifoSize;

      /* Mark unused calendar entries (we only use the first two) */
      for (i = 2; i < SPI_SWITCH_MAX_CALENDAR_ENTRIES; i++)
      {
         FicSpiInterfaceSettings.rxInterface.calendar[i] = Unused;
         FicSpiInterfaceSettings.txInterface.calendar[i] = Unused;
      }

      for (i = 0; i < SPI_SWITCH_MAX_FIFOS_PER_INTERFACE; i++)
      {
         FicSpiInterfaceSettings.txInterface.maxBurst1[i] = MaxBurst1 << FifoSize;
         FicSpiInterfaceSettings.txInterface.maxBurst2[i] = MaxBurst2 << FifoSize;
      }

      printf("Configuring Switch interface connected to the FIC ... ");
      Result = SpiSwitchConfigureInterface(SPI_SWITCH_FIC_INTERFACE,
                                           RxTxMask,
                                           NoDefaults,
                                           &FicSpiInterfaceSettings);
      if (Result != RC_SUCCESS)
      {
         printf("FAILED, result = 0x%08lX\n", Result);
         return (Result);
      }
      else
      {
         printf("DONE\n");
      }
   }

   /****************************/
   /* Connection configuration */
   /****************************/
   /* Set up the connections and start the switch */
   Result = openConnections(connections, connectionCount);
   if (Result != RC_SUCCESS)
   {
      return(Result);
   }

   /********************/
   /* SPI Bus training */
   /********************/

   /* Train the interfaces connected to NPU A */
   if (useNPUA == TRUE)
   {
      /* Reset NPUA's MSF */
      TrainingAttempt = 0;
      printf("Training NPUA\n");
      do
      {
         /* If this isn't the first time into the loop, */
         /* Sleep for 2 seconds to prevent immediate    */
         /* retraining                                  */
         if (TrainingAttempt != 0)
         {
            usleep(TRAINING_RETRY_DELAY);
         }

         TrainingAttempt++;
         printf("\tAttempt number %lu:\n", TrainingAttempt);
         printf("\t\tResetting NPUA MSF ... ");
         fflush(stdout);
         Result = MiscNPUReset(PPM_MASTER_PROC);
         if (Result != RC_SUCCESS)
         {
            printf("Unsuccessful, result = 0x%08lX\n", Result);
         }
         else
         {
            printf("DONE\n");

            /* Train the NPU A interface NPU -> SPI */
            Result = Train_NPU_SPI_Calendar(PPM_MASTER_PROC, NPUATxCalLen, &npuATxCalendar);
            if (Result == RC_SUCCESS)
            {
               /* Train the NPUA interface SPI -> NPU */
               Result = Train_SPI_NPU(PPM_MASTER_PROC, NPUARxCalLen);
            }
         }
      } while ((Result != RC_SUCCESS) && (TrainingAttempt < TRAINING_RETRIES));

      if (Result != RC_SUCCESS)
      {
          printf("Training NPUA failed\n");
          return (Result);
      }
   }

   /* Train the interfaces connected to NPU B */
   if (useNPUB == TRUE)
   {
      /* Reset NPUB's MSF */
      TrainingAttempt = 0;
      printf("Training NPUB\n");
      do
      {
         /* If this isn't the first time into the loop, */
         /* Sleep for 2 seconds to prevent immediate    */
         /* retraining                                  */
         if (TrainingAttempt != 0)
         {
            usleep(TRAINING_RETRY_DELAY);
         }

         TrainingAttempt++;
         printf("\tAttempt number %lu\n", TrainingAttempt);
         printf("\t\tResetting NPUB MSF ... ");
         Result = MiscNPUReset(PPM_SLAVE_PROC);
         if (Result != RC_SUCCESS)
         {
            printf("Unsuccessful, result = 0x%08lX\n", Result);
         }
         else
         {
            printf("DONE\n");

            /* Train the NPU B interface NPU -> SPI */
            Result = Train_NPU_SPI_Calendar(PPM_SLAVE_PROC, NPUBTxCalLen, &npuBTxCalendar);
            if (Result == RC_SUCCESS)
            {
               /* Train the NPU B interface SPI -> NPU */
               Result = Train_SPI_NPU(PPM_SLAVE_PROC, NPUBRxCalLen);
               if (Result != RC_SUCCESS)
               {
                  return (Result);
               }
            }
         }
      } while ((Result != RC_SUCCESS) && (TrainingAttempt < TRAINING_RETRIES));

      if (Result != RC_SUCCESS)
      {
          printf("Training NPUB failed\n");
          return (Result);
      }
   }

   /* Train the FIC interfaces, if specified */
   if (useFIC == TRUE)
   {
      TrainingAttempt = 0;
      printf("Training FIC\n");
      do
      {
         /* If this isn't the first time into the loop, */
         /* Sleep for 2 seconds to prevent immediate    */
         /* retraining                                  */
         if (TrainingAttempt != 0)
         {
            usleep(TRAINING_RETRY_DELAY);
         }

         TrainingAttempt++;
         printf("\tAttempt number %lu: ", TrainingAttempt);
         fflush(stdout);
         Result = Train_FIC(useFIC, FICLoopEnable, maxFrameSize);
      } while ((Result != RC_SUCCESS) && (TrainingAttempt < TRAINING_RETRIES));

      if (Result != RC_SUCCESS)
      {
         printf("Training FIC failed\n");
         return (Result);
      }
   }

   /* Train the interfaces connected to the RTM */
   if (useRTM == TRUE)
   {
      TrainingAttempt = 0;
      printf("Training RTM\n");
      do
      {
         /* If this isn't the first time into the loop, */
         /* Sleep for 2 seconds to prevent immediate    */
         /* retraining                                  */
         if (TrainingAttempt != 0)
         {
            usleep(TRAINING_RETRY_DELAY);
         }

         TrainingAttempt++;
         printf("\tAttempt number %lu:\n", TrainingAttempt);
         Result = Train_RTM(maxFrameSize);
      } while ((Result != RC_SUCCESS) && (TrainingAttempt < TRAINING_RETRIES));

      if (Result != RC_SUCCESS)
      {
          printf("Training RTM failed\n");
          return (Result);
      }
   }

   printf("SPI configuration and training successful!\n");
   return (Result);
}
