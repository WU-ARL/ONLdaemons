/*****************************************************************************/
/* RadiSys ATCA-70xx Software Development Kit                                */
/* All of this software and any documentation provided with this software is */
/* Copyright 2005, 2006 RadiSys Corporation.                                 */
/*                                                                           */
/* All source code provided in this distribution is RadiSys Confidential.    */
/*                                                                           */
/* RADISYS LICENSES THIS SOFTWARE AND ANY DOCUMENTATION PROVIDED WITH THIS   */
/* SOFTWARE TO LICENSEE AS-IS, WITHOUT WARRANTY.  RADISYS DOES NOT MAKE, AND */
/* LICENSEE EXPRESSLY HEREBY WAIVES, ALL WARRANTIES, EXPRESS OR IMPLIED.     */
/* WITHOUT LIMITING THE FOREGOING, RADISYS DOES NOT MAKE AND EXPRESSLY       */
/* DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A        */
/* PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, ACCURACY OF DATA, QUIET      */
/* ENJOYMENT AND FREEDOM FROM DESIGN DEFECT.                                 */
/*                                                                           */
/* This software and any documentation provided with this software is        */
/* subject to United States export control laws and regulations.  Each       */
/* licensee must comply with all applicable United States export control     */
/* laws, including, but not limited to, the rules and regulations of the     */
/* Bureau of Industry and Secruity ("BIS") of the US Department of Commerce  */
/* and the Office of Foreign Assets Control ("OFAC") of the US Department of */
/* the Treasury.  Such compliance includes, but is not limited to,           */
/* licensee's refraining from the export or re-export, either directly or    */
/* indirectly, of this software and any documentation provided with this     */
/* software or any product made from or with such software or documentation  */
/* without first obtaining any required license or other approval from BIS   */
/* or from any other agency or department of the United States government    */
/* with jurisdiction over such export or re-export.                          */
/*                                                                           */
/* This software and any documentation provided with this software           */
/* constitute a "commericial item," as that term is defined in 48 C.F.R.     */
/* 2.101, consisting of "commercial computer software" and "commercial       */
/* computer software documentation," as such terms are used in 48 C.F.R.     */
/* 12.212.  Consistent with 48 C.F.R. 12.212 and 48 C.F.R. 227.7202-1        */
/* through 227.7202-4, all U.S. government users acquire this software and   */
/* any documentation proivided with this software with only those rights     */
/* provided in the applicable license agreement between RadiSys Corporation  */
/* and the licensee.                                                         */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*    Filename:         trainspi.c                                           */
/*                                                                           */
/*    Description:      Sample code utility to train SPI bus paths           */
/*                                                                           */
/*    Version History:                                                       */
/*                                                                           */
/*    Date                 Comment                          Author           */
/*    ====================================================================== */
/*    01/21/2005           Initial creation                 AES              */
/*    04/29/2005           Alpha version                    HS               */
/*    08/10/2005           Adapted for sample code          AES              */
/*    11/28/2005           Change example connection array  AES              */
/*    05/26/2006           Adapted for common training      Andrew Watts     */
/*    08/15/2006           Dynamic Device Training          AES              */
/*                                                                           */
/*    Functions:                                                             */
/*    s_ctrlc                       CTRL-C handler                           */
/*    Train_SPI_NPU                 Train the Rx side of the NPU interface   */
/*    Train_NPU_SPI                 Train the Tx side of the NPU interface   */
/*    Train_RTM                     Train the RTM Rx and Tx interfaces       */
/*    Train_FIC                     Train the FIC Rx and Tx interfaces       */
/*    openConnections               Create the SPI paths                     */
/*    Train                         The main training function (sets paths)  */
/*    spiMonitorEventsHandler       Process detected events                  */
/*    monitor                       Adjust connections and event handling    */
/*    ProcessCommandLine            Process the gathered user options        */
/*    printUsage                    Displays proper command line options     */
/*    printErrorAndExit             Process user option errors               */
/*    main                          The main application                     */
/*****************************************************************************/
#include "trainspi.h"

/************/
/* Literals */
/************/
#define SYSTEM_CONNECTION_COUNT         30
#define SYSTEMFIC_CONNECTION_COUNT      40
#define LOOPBACKFIC_CONNECTION_COUNT    30
#define SYSTEMFIC_10GB_CONNECTION_COUNT 24

#define ONL_CONNECTION_COUNT 20

/********************/
/* Global variables */
/********************/
FILE     *MonitorLogFile = NULL;
UINT32   FICLoop = FALSE;

/* An array for user-readable names for the SPI switch events */
char *EventNames[SPI_SWITCH_MAX_REG_EVENTS] =
{
   "CPU Overwrite",
   "CPU Underwrite",
   "RX Synch Loss",
   "RX Fatal Error",
   "RX Fifo Overflow",
   "RX S2A Failure",
   "RX Int Parity",
   "RX Ext Parity",
   "RX Non-Prov Data",
   "RX Watchdog",
   "TX Synch Loss",
   "TX Fatal Error",
   "TX Local Parity",
   "TX Clock Domain"
};

/*===================*/
/* Connection Arrays */
/*===================*/
/* "Round-the-world", without FIC */
Connection SystemConnections[SYSTEM_CONNECTION_COUNT] =
{
   /* NPUA to RTM connections */
   {0, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 0},
   {1, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 1},
   {2, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 2},
   {3, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 3},
   {4, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 4},
   {5, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 5},
   {6, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 6},
   {7, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 7},
   {8, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 8},
   {9, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 9},

   /* RTM to NPUB connections */
   {0, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 0},
   {1, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 1},
   {2, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 2},
   {3, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 3},
   {4, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 4},
   {5, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 5},
   {6, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 6},
   {7, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 7},
   {8, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 8},
   {9, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 9},

   /* NPUB to NPUA connections */
   {0, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 0},
   {1, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 1},
   {2, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 2},
   {3, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 3},
   {4, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 4},
   {5, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 5},
   {6, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 6},
   {7, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 7},
   {8, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 8},
   {9, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 9}
};

/* "Round-the-world", with FIC */
Connection SystemFICConnections[SYSTEMFIC_CONNECTION_COUNT] =
{
   /* NPUA to RTM connections */
   {0, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 0},
   {1, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 1},
   {2, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 2},
   {3, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 3},
   {4, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 4},
   {5, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 5},
   {6, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 6},
   {7, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 7},
   {8, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 8},
   {9, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 9},

   /* RTM to NPUB connections */
   {0, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 0},
   {1, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 1},
   {2, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 2},
   {3, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 3},
   {4, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 4},
   {5, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 5},
   {6, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 6},
   {7, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 7},
   {8, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 8},
   {9, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 9},

   /* NPUB to FIC connections */
   {0, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 0},
   {1, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 1},
   {2, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 2},
   {3, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 3},
   {4, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 4},
   {5, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 5},
   {6, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 6},
   {7, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 7},
   {8, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 8},
   {9, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 9},

   /* FIC to NPUA connections */
   {0, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 0},
   {1, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 1},
   {2, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 2},
   {3, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 3},
   {4, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 4},
   {5, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 5},
   {6, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 6},
   {7, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 7},
   {8, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 8},
   {9, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 9}
};

/* Loopback (RTM to NPU and back, 5 to NPU A, 5 to NPU B) */
Connection LoopbackConnections[LOOPBACKFIC_CONNECTION_COUNT] =
{
   /* NPUA to RTM connections */
   {0, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 5},
   {1, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 6},
   {2, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 7},
   {3, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 8},
   {4, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 9},

   /* NPUB to RTM connections */
   {0, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 0},
   {1, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 1},
   {2, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 2},
   {3, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 3},
   {4, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 4},

   /* RTM to NPUA connections */
   {5, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 0},
   {6, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 1},
   {7, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 2},
   {8, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 3},
   {9, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 4},

   /* RTM to NPUB connections */
   {0, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 0},
   {1, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 1},
   {2, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 2},
   {3, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 3},
   {4, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 4},

   /* FIC to FIC connections */
   {0, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 0},
   {1, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 1},
   {2, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 2},
   {3, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 3},
   {4, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 4},
   {5, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 5},
   {6, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 6},
   {7, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 7},
   {8, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 8},
   {9, SPI_SWITCH_FIC_INTERFACE, SPI_SWITCH_FIC_INTERFACE, 9}
};

/* ONL (RTM 0-4 to NPU A and back, RTM 5-9 to NPU B and back) */
Connection ONLConnections[ONL_CONNECTION_COUNT] =
{
   /* NPUA to RTM connections */
   {0, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 0},
   {1, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 1},
   {2, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 2},
   {3, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 3},
   {4, SPI_SWITCH_NPUA_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 4},

   /* NPUB to RTM connections */
   {0, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 5},
   {1, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 6},
   {2, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 7},
   {3, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 8},
   {4, SPI_SWITCH_NPUB_INTERFACE, SPI_SWITCH_RTM_INTERFACE, 9},

   /* RTM to NPUA connections */
   {0, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 0},
   {1, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 1},
   {2, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 2},
   {3, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 3},
   {4, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUA_INTERFACE, 4},

   /* RTM to NPUB connections */
   {5, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 0},
   {6, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 1},
   {7, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 2},
   {8, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 3},
   {9, SPI_SWITCH_RTM_INTERFACE, SPI_SWITCH_NPUB_INTERFACE, 4},
};

/******************************************************************************/
/* Function:      s_ctrlc                                                     */
/*                                                                            */
/* Description:   This function serves as the Ctrl-C handler.                 */
/*                                                                            */
/* Input:         dwCtrlType  - The received signal number                    */
/*                                                                            */
/* Output:        None                                                        */
/******************************************************************************/
void s_ctrlc(int dwCtrlType)
{
   rpclClose();
   exit(3);
}

/******************************************************************************/
/* Function:      spiMonitorEventsHandler                                     */
/*                                                                            */
/* Description:   The event handler for SPI Switch events                     */
/*                                                                            */
/* Input:         event - The event that occurred                             */
/*                Len   - The length of the data associated with the event    */
/*                pData - A pointer to the data associated with the event     */
/*                                                                            */
/* Output:        RC_SUCCESS                                                  */
/******************************************************************************/
PPM_STATUS spiMonitorEventsHandler(PPM_EVENT event,
                                   UINT32    Len,
                                   void      *pData)
{
   EventEntryType *eventEntry;

   eventEntry = (EventEntryType *)pData;

   if (MonitorLogFile != NULL)
   {
      /* Print the event.  The format is as follows: */
      /* Interface       Interface Type  Event           Debug Status    Timestamp */
      fprintf(MonitorLogFile,
              "%-11s\t%-11s\t%-20s\t0x%08lX\t0x%08lX\n",
              InterfaceNames[eventEntry->interface],
              (eventEntry->interfaceType - 1) ? "transmit" : "receive",
              EventNames[eventEntry->event-SPI_FIRST_EVENT],
              eventEntry->debugStatus,
              eventEntry->timeStamp);

      fflush(MonitorLogFile);
   }

   return(RC_SUCCESS);
}

/******************************************************************************/
/* Function:      monitor                                                     */
/*                                                                            */
/* Description:   Monitor events and statistics                               */
/*                                                                            */
/* Input:         None                                                        */
/*                                                                            */
/* Output:        None                                                        */
/******************************************************************************/
void monitor(void)
{
   INT32  i;
   INT32  returnCode       = 0;
   INT32  stopMonitoring   = 0;
   UCHAR  buf[60];
   UCHAR  tmpStr[20];
   UINT32 eventMask        = 0;
   UINT32 interfaceType    = 0;
   UINT32 rxInt;
   UINT32 rxPort;
   UINT32 txInt;
   UINT32 txPort;

   /* Open the monitor log file */
   MonitorLogFile = fopen("/var/log/spi", "w+");
   if (MonitorLogFile == NULL)
   {
      printf("ERROR - %s when opening /var/log/spi\n", strerror(errno));
      return;
   }

   returnCode = fprintf(MonitorLogFile, "Interface  \tInterface Type\tEvent               \tDebug Status\tTimestamp\n");
   if (returnCode <= 0)
   {
      printf("write to log file failed - %s ... exiting\n", strerror(errno));
      returnCode = 1;
      goto error;
   }

   fprintf(MonitorLogFile, "-----------\t--------------\t--------------------\t------------\t---------\n");
   fflush(MonitorLogFile);

   for (i = SPI_FIRST_EVENT; i <= SPI_LAST_EVENT; i++)
   {
      /* Register the handler for events */
      returnCode = SpiSwitchRegisterEventHandler(i, (PPM_CALLBACK)spiMonitorEventsHandler);
      if (returnCode != RC_SUCCESS)
      {
         printf("Switch handler registration failed (rc=0x%lX) ... exiting\n", returnCode);
         returnCode = 1;
         goto error;
      }
   }

   /* Gather and process the user input */
   while (stopMonitoring != 1)
   {
      /* Reset the loop variables and print the menu */
      interfaceType = 0;
      eventMask = 0;

      printf("\nSPI-4.2 Switch monitoring options:\n\n");
      printf("\treg   <event mask>\n");
      printf("\tunreg <event mask>\n");
      printf("\treset <rx port> <rx interface> <tx interface> <tx port>\n");
      printf("\tstop\n");
      printf("\nEnter one of the above:  ");

      /* Get the user input */
      fgets(buf, sizeof(buf), stdin);

      /* Process the user input */
      if (strncasecmp(buf, "stop", 4) == 0)
      {
         /* Stop the application */
         printf("Exiting ... \n");
         stopMonitoring = 1;
      }
      else if (strncasecmp(buf, "unreg", 5) == 0)
      {
         /* Unregister a monitored event */
         sscanf(buf, "%s %lu\n", &(tmpStr[0]), &eventMask);
         printf("Unregistering handler for event mask 0x%lX ... \n", eventMask);

         /* Unregister the handler for the specified event(s) */
         returnCode = SpiSwitchUnregisterEventHandler(eventMask);
         if (returnCode != RC_SUCCESS)
         {
            printf("switch handler deregistration failed (rc=0x%lX) ... exiting\n",
                   returnCode);
            returnCode = 1;
            goto error;
         }
      }
      else if (strncasecmp(buf, "reg", 3) == 0)
      {
         /* Register to monitor an event */
         sscanf(buf, "%s %lu\n", &(tmpStr[0]), &eventMask);
         printf("Registering handler for event mask 0x%lX ... \n", eventMask);

         /* Register the handler for the specified event(s) */
         returnCode = SpiSwitchRegisterEventHandler(eventMask,
                                                    (PPM_CALLBACK)spiMonitorEventsHandler);
         if (returnCode != RC_SUCCESS)
         {
            printf("switch handler registration failed (rc=0x%lX) ... exiting\n",
                   returnCode);
            returnCode = 1;
            goto error;
         }
      }
      else if (strncasecmp(buf, "reset", 5) == 0)
      {
         /* Reset a SPI connection */
         sscanf(buf,
                "%s %lu %lu %lu %lu\n",
                &(tmpStr[0]),
                &rxPort,
                &rxInt,
                &txInt,
                &txPort);
         printf("Resetting connection %s-port%lu to %s-port%lu ... \n",
                InterfaceNames[rxInt],
                rxPort,
                InterfaceNames[txInt],
                txPort);

         /* Reset the specified connection */
         returnCode = SpiSwitchCloseConnection(rxPort, rxInt, txInt, txPort);
         if (returnCode != RC_SUCCESS)
         {
            printf("close connection failed (rc=0x%lX) ... exiting\n", returnCode);
            goto error;
         }

         returnCode = SpiSwitchOpenConnection(rxPort, rxInt, txInt, txPort);
         if (returnCode != RC_SUCCESS)
         {
            printf("open connection failed (rc=0x%lX) ... exiting\n",
                   returnCode);
            goto error;
         }
      }
      else
      {
         /* Unrecognized choice; go to the top of the loop */
         printf("Continuing ... \n");
      }
   }

/* Clean up resources and exit */
error:
   fclose(MonitorLogFile);
}

/******************************************************************************/
/* Function:      ProcessCommandLine                                          */
/*                                                                            */
/* Description:   The main ppmtest task for the master processor (socket clnt)*/
/*                                                                            */
/* Input:         connections    - Array of data path connections to set      */
/*                numConns       - Number of connections in the array         */
/*                useFIC         - Flag denoting whether to include a FIC in  */
/*                                 the configuration                          */
/*                rxLoWaterMark  - The low watermark value to use on the      */
/*                                 receive side of the SPI Switch interfaces. */
/*                rxHiWaterMark  - The high watermark value to use on the     */
/*                                 receive side of the SPI Switch interfaces. */
/*                rtmClkMult     - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch RTM I/F    */
/*                ficClkMult     - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch FIC I/F    */
/*                npuAClkMult    - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch NPU A I/F  */
/*                npuBClkMult    - The clock multiplier to use on the         */
/*                                 transmit side of the SPI Switch NPU A I/F  */
/*                maxFrameSize   - Maximum frame size to support              */
/*                monitorMode    - Flag denoting whether to monitor events or */
/*                                 train data paths                           */
/*                useFicCalLen   - Flag denoting whether to use the default   */
/*                                 calendar length or one for the FIC         */
/*                                                                            */
/* Output:        SUCCESS                                                     */
/******************************************************************************/
int ProcessCommandLine(Connection *connections,
                       UINT32 numConns,
                       UINT32 useFIC,
                       UINT32 rxLoWaterMark,
                       UINT32 rxHiWaterMark,
                       UINT32 rtmClkMult,
                       UINT32 ficClkMult,
                       UINT32 npuAClkMult,
                       UINT32 npuBClkMult,
                       UINT32 maxFrameSize,
                       UINT32 monitorMode,
                       UINT32 useFicCalLen)
{
   PPM_STATUS       RetVal;
   struct sigaction sa;
   UINT32           DeviceMask = TRAIN_NPUA | TRAIN_NPUB | TRAIN_RTM;
   RSYS_enetIO_XGMAC_Cfg_t RsysXGMACCfg;
   int i;

   /* Register for a ctrl-c handler */
   memset (&sa, 0, sizeof (sa));
   sa.sa_handler = s_ctrlc;
   sigaction(SIGINT, &sa, NULL);
   sigaction(SIGABRT, &sa, NULL);

   /* Either monitor events or train the data paths */
   if (monitorMode == TRUE)
   {
      /* Monitor events */
      monitor();
   }
   else
   {
      if (useFIC == TRUE)
         DeviceMask |= TRAIN_FIC;

      if(DeviceInfo.RTM == RTM_NOT_PRESENT)
		 DeviceMask &= ~(TRAIN_RTM);

      /* Train the data paths */
      RetVal = Train(connections,
                     numConns,
                     DeviceMask,
                     rxLoWaterMark,
                     rxHiWaterMark,
                     rtmClkMult,
                     ficClkMult,
                     npuAClkMult,
                     npuBClkMult,
                     maxFrameSize,
                     FICLoop,
                     useFicCalLen);

      if (DeviceInfo.RTM == IS_2x10GbE_RTM_PRESENT)
      {
         for(i = 1; i <= 2; i++)
         {
            memset(&RsysXGMACCfg, 0, sizeof(RsysXGMACCfg));
            RsysXGMACCfg.portNo = i;
            RetVal = RSYS_EnetIO_XGMAC_CfgGet(RSYS_ENET_IO_10GbE_RTM, &RsysXGMACCfg);
            if(RetVal != RSYS_ENET_IO_RC_SUCCESS)
            {
               printf("Unsuccessful\n\t\tRTM could not obtain XGMAC configs = 0x%08lX\n", RetVal);
               return(RetVal);
            }
            //The microcode commonly used with trainspi expects that the RTM will
            //  not strip the CRC.
            RsysXGMACCfg.rxStripCrcEnable = RSYS_DISABLE;
            RetVal = RSYS_EnetIO_XGMAC_CfgSet(RSYS_ENET_IO_10GbE_RTM, &RsysXGMACCfg);
            if(RetVal != RSYS_ENET_IO_RC_SUCCESS)
            {
               printf("Unsuccessful\n\t\tRTM could not set XGMAC configs = 0x%08lX\n", RetVal);
               return(RetVal);
            }
         }
      }

   }

   /* Exit from the utility */
   rpclClose();
   return(SUCCESS);
}

/******************************************************************************/
/* Function:      printUsage                                                  */
/*                                                                            */
/* Description:   Print the usage of this command                             */
/*                                                                            */
/* Input:         command  - The name of the utility                          */
/*                                                                            */
/* Output:        None                                                        */
/******************************************************************************/
void printUsage(char *command)
{
   printf("Usage: %s -i <Remote_Proxy_Server_IP_Address> [options]\n", command);
   printf("Options:\n");
   printf("-p <mode>\t\tSPI switch path mode, can be \"system\", \"loopback\", or \"onl\" (default)\n");
   printf("-f\t\tUse the FIC interface when testing\n");
   printf("-o\t\tPut the FIC in loopback mode (1x10 GbE FIC only)\n");
   printf("-u\t\tUse the FIC calendar length when testing\n");
   printf("-l <low_watermark>\t\tFM1010 RX low watermark\n");
   printf("-h <high_watermark>\t\tFM1010 RX high watermark\n");
   printf("-a <multiplier>\t\tFM1010 TX clock multiplier for interface\n");
   printf("\t\t\tconnected to NPUA, valid range [6..9]\n");
   printf("-b <multiplier>\t\tFM1010 TX clock multiplier for interface\n");
   printf("\t\t\tconnected to NPUB, valid range [6..9]\n");
   printf("-r <multiplier>\t\tFM1010 TX clock multiplier for interface\n");
   printf("\t\t\tconnected to the RTM, valid range [6..9]\n");
   printf("-c <multiplier>\t\tFM1010 TX clock multiplier for interface\n");
   printf("\t\t\tconnected to the FIC, valid range [6..9]\n");
   printf("-j <max frame size>\t\tRTM Maximum Frame Size\n");
   printf("-m\t\tMonitor events and statistics\n");
}

/******************************************************************************/
/* Function:      printErrorAndExit                                           */
/*                                                                            */
/* Description:   Print an error message, the command usage, and then exit    */
/*                                                                            */
/* Input:         command        - The name of the command that failed        */
/*                errorMessage   - The reason for the failure                 */
/*                errorArgument  - The argument causing the failure           */
/*                errorCode      - The resulting internal error code          */
/*                                                                            */
/* Output:        None                                                        */
/******************************************************************************/
void printErrorAndExit(UCHAR *command,
                       UCHAR *errorMessage,
                       UCHAR *errorArgument,
                       UINT32 errorCode)
{
   /* Print the error message */
   printf("%s:  %s", command, errorMessage);
   if ( errorArgument != NULL )
   {
      printf(" - %s", errorArgument);
   }
   printf("\n");

   /* Print the Usage */
   printUsage(command);

   /* Exit */
   exit(errorCode);
}

/******************************************************************************/
/* Function:      main                                                        */
/*                                                                            */
/* Description:   The main configuration parser task                          */
/*                                                                            */
/* Input:         argc        - The number of command line parameters         */
/*                argv        - The array of command line parameters          */
/*                                                                            */
/* Output:        RPP_SUCCESS, or RPP_ERR on failure                          */
/******************************************************************************/

typedef enum {pathSystem, pathLoop, pathONL} path_t;

int main(int argc, char *argv[])
{
   Connection *Connections = NULL;
   int    optchar;
   UCHAR  UserResponse[MAX_USER_INPUT];
   UCHAR  ipAddr[30];
   UINT32 gotIPAddr     = FALSE;
   UINT32 i;
   UINT32 NumConns = 0;
   UINT32 NumFicConns;
   UINT32 NumRtmConns;

   /* Default command line option values */
   path_t  pathMode      =   pathONL;         /* default is ONL */
   UINT32 useFIC        = FALSE;
   UINT32 useFicCalLen  = FALSE;
   UINT32 rxLoWaterMark =  0x80;
   UINT32 rxHiWaterMark = 0x100;
   UINT32 rtmClkMult    =     8;
   UINT32 ficClkMult    =     8;
   UINT32 npuAClkMult   =     8;
   UINT32 npuBClkMult   =     8;
   UINT32 monitorMode   = FALSE;
   UINT32 maxFrameSize  =  1518;

   /* The commented out variable below will print  */
   /* Remote Proxy trace statements to screen      */
   /*RppClientOpts  Options = {50, TRUE, TRUE, ""}; */
   RppClientOpts  Options = {50, FALSE, FALSE, ""};
   UINT32         RetVal;

   /* Initialize the IP address */
   memset(ipAddr, 0, 30);
   strcpy(ipAddr, LOCALHOST);

   /* Parse the command line arguments */
   while ((optchar = getopt(argc,argv,"i:p:fl:h:a:b:r:c:mj:ou")) != -1)
   {
      switch(optchar)
      {
         /* Remote Proxy Server's IP address */
         case 'i':
            if (strlen(optarg) > 30)
            {
               printErrorAndExit(argv[0],
                                 "invalid IP address argument",
                                 optarg,
                                 FAILURE);
            }
            strncpy(ipAddr, optarg, strlen(optarg));
            gotIPAddr = TRUE;
            break;

         /* FM1010 path mode */
         case 'p':
            if (strlen(optarg) > 30)
            {
               printErrorAndExit(argv[0],
                                 "invalid path mode argument",
                                 optarg,
                                 FAILURE);
            }

            if (strcasecmp(optarg, "system") == 0)
            {
               /* Use "round-the-world" */
               pathMode = pathSystem;
            }
            else if (strcasecmp(optarg, "loopback") == 0)
            {
               /* Use loopback */
               pathMode = pathLoop;
            }
            else if (strcasecmp(optarg, "onl") == 0)
            {
               /* Use loopback */
               pathMode = pathONL;
            }
            else
            {
               /* Invalid option */
               printErrorAndExit(argv[0],
                                 "invalid path mode argument",
                                 optarg,
                                 FAILURE);
            }
            break;

         /* Use the FIC interface */
         case 'f':
            useFIC = TRUE;
            break;

         /* Set the FIC in loopback  */
         case 'o':
            FICLoop = TRUE;
            break;

         /* FM1010 Receive interface low watermark */
         case 'l':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid low watermark argument",
                                 optarg,
                                 FAILURE);
            }
            rxLoWaterMark = strtoul(optarg, NULL, 10);
            break;

         /* FM1010 Receive interface high watermark */
         case 'h':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid high watermark argument",
                                 optarg,
                                 FAILURE);
            }
            rxHiWaterMark = strtoul(optarg, NULL, 10);
            break;

         /* FM1010 Transmit interface, connected to NPUA, clock multiplier */
         case 'a':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid clock multiplier argument",
                                 optarg,
                                 FAILURE);
            }
            npuAClkMult = strtoul(optarg, NULL, 10);
            break;

         /* FM1010 Transmit interface, connected to NPUB, clock multiplier */
         case 'b':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid clock multiplier argument",
                                 optarg,
                                 FAILURE);
            }
            npuBClkMult = strtoul(optarg, NULL, 10);
            break;

         /* FM1010 Transmit interface, connected to the RTM, clock multiplier */
         case 'r':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid clock multiplier argument",
                                 optarg,
                                 FAILURE);
            }
            rtmClkMult = strtoul(optarg, NULL, 10);
            break;

         /* FM1010 Transmit interface, connected to the FIC, clock multiplier */
         case 'c':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid clock multiplier argument",
                                 optarg,
                                 FAILURE);
            }
            ficClkMult = strtoul(optarg, NULL, 10);
            break;

         /* RTM Max Frame Size */
         case 'j':
            if (strspn(optarg, "0123456789") != strlen(optarg))
            {
               printErrorAndExit(argv[0],
                                 "invalid frame size argument",
                                 optarg,
                                 FAILURE);
            }
            maxFrameSize = strtoul(optarg, NULL, 10);
            break;

         /* Monitor events and statistics */
         case 'm':
            monitorMode = TRUE;
            break;

         /* Use the FIC interface */
         case 'u':
            useFicCalLen = TRUE;
            break;

         /* Unknown option */
         default:
            fprintf(stderr, "invalid option: %c\n", optchar);
            return(FAILURE);
      }  /* End switch */
   }  /* End while */

   /* Make sure only training or monitoring was specified, not both */
   if ((gotIPAddr == TRUE) && (monitorMode == TRUE))
   {
      printErrorAndExit(argv[0],
                        "choose either monitor or training",
                        NULL,
                        FAILURE);
   }

   /* Open a connection to the Remote Proxy Server */
   RetVal = rpclOpen(ipAddr, &Options);
   if (RetVal != RC_SUCCESS)
   {
      printf("%s failed to initialize Remote Proxy Client, error 0x%X\n",
             argv[0],
             (int)RetVal);
      return (FAILURE);
   }

   //RetVal = MiscIsMaster();
   //printf("%x\n",(int)RetVal);

   RetVal = MiscGetDeviceInfo(&DeviceInfo);
   if (RetVal != RC_SUCCESS)
   {
      printf("%s failed to obtain device information, error code: 0x%x\n",
             argv[0],
             (int)RetVal);
      rpclClose();
      return (FAILURE);
   }

   if (DeviceInfo.RTM == RTM_NOT_PRESENT)
   {
      printf("Could not detect an RTM.\n");
      printf("Continue anyways (no/yes)? ");
      fgets(UserResponse, MAX_USER_INPUT - 1, stdin);
      if  (strncmp(UserResponse, "yes", 4) != 0)
      {
         printf("\n%s failed to detect an RTM, exiting.\n",
                argv[0]);
         rpclClose();
         return (FAILURE);
      }
   }

   if ((DeviceInfo.FIC == FIC_NOT_PRESENT) &&
       (useFIC == TRUE))
   {
      printf("FIC requested, but no FIC was detected.\n");
      printf("Continue anyways (no/yes)? ");
      fgets(UserResponse, MAX_USER_INPUT - 1, stdin);
      if  (strncmp(UserResponse, "yes", 4) != 0)
      {
         printf("\n%s requested to run with FIC, but no FIC was detected, exiting.\n",
                argv[0]);
         rpclClose();
         return (FAILURE);
      }
   }

   if (pathMode == pathSystem)
   {
      NumConns = 0;

      //Set up connections to and from the FIC
      switch(DeviceInfo.FIC)
      {
         case IS_10GbE_FIC_PRESENT:
            NumFicConns = 2;
            break;
         case IS_4GbE_FIC_PRESENT:
            NumFicConns = 8;
            break;
         case FIC_NOT_PRESENT:
         default:
            /* To support the FIC, the sample microcode presumes two          */
            /* connections for Tx on NPU B and Rx on NPU A; set accordingly.  */
            NumFicConns = 2; //10;
            break;
      }
      if (useFIC == TRUE)
      {
         for (i = 0; i < NumFicConns; i++)
         {
            SystemFICConnections[i + NumConns].rxPort = i;
            SystemFICConnections[i + NumConns].rxInterface = SPI_SWITCH_NPUB_INTERFACE;
            SystemFICConnections[i + NumConns].txInterface = SPI_SWITCH_FIC_INTERFACE;
            SystemFICConnections[i + NumConns].txPort = i;
            SystemFICConnections[i + NumConns + NumFicConns].rxPort = i;
            SystemFICConnections[i + NumConns + NumFicConns].rxInterface = SPI_SWITCH_FIC_INTERFACE;
            SystemFICConnections[i + NumConns + NumFicConns].txInterface = SPI_SWITCH_NPUA_INTERFACE;
            SystemFICConnections[i + NumConns + NumFicConns].txPort = i;
         }

         //One connection for each direction on the FIC
         NumConns += (NumFicConns * 2);
      }
      else
      {
         for(i = 0; i < 2; i++) //Max of 2 connections without FIC
         {
            SystemFICConnections[i + NumConns].rxPort = i;
            SystemFICConnections[i + NumConns].rxInterface = SPI_SWITCH_NPUB_INTERFACE;
            SystemFICConnections[i + NumConns].txInterface = SPI_SWITCH_NPUA_INTERFACE;
            SystemFICConnections[i + NumConns].txPort = i;
         }
         NumConns += 2;
      }

      //Set up connections to and from the RTM
      switch(DeviceInfo.RTM)
      {
         case IS_2x10GbE_RTM_PRESENT:
            NumRtmConns = 2;
            break;
         case RTM_NOT_PRESENT:
			NumRtmConns = 0;
			break;
         case IS_10x1GbE_RTM_PRESENT:
         default :
            NumRtmConns = 10;
            break;
      }

      for (i = 0; i < NumRtmConns; i++)
      {
         SystemFICConnections[i + NumConns].rxPort = i;
         SystemFICConnections[i + NumConns].rxInterface = SPI_SWITCH_NPUA_INTERFACE;
         SystemFICConnections[i + NumConns].txInterface = SPI_SWITCH_RTM_INTERFACE;
         SystemFICConnections[i + NumConns].txPort = i;
         SystemFICConnections[i + NumConns + NumRtmConns].rxPort = i;
         SystemFICConnections[i + NumConns + NumRtmConns].rxInterface = SPI_SWITCH_RTM_INTERFACE;
         SystemFICConnections[i + NumConns + NumRtmConns].txInterface = SPI_SWITCH_NPUB_INTERFACE;
         SystemFICConnections[i + NumConns + NumRtmConns].txPort = i;
      }

      //One connection for each direction on the RTM
      NumConns += (NumRtmConns * 2);

	  //If there is no RTM, set up 10 connections between NPUA and NPUB
	  if(DeviceInfo.RTM == RTM_NOT_PRESENT)
	  {
		 for(i = 0; i < 10; i++)
		 {
			SystemFICConnections[i + NumConns].rxPort = i;
            SystemFICConnections[i + NumConns].rxInterface = SPI_SWITCH_NPUA_INTERFACE;
            SystemFICConnections[i + NumConns].txInterface = SPI_SWITCH_NPUB_INTERFACE;
            SystemFICConnections[i + NumConns].txPort = i;
		 }
		 NumConns += 10;
	  }

      Connections = SystemFICConnections;
   }
   else if (pathMode == pathLoop)
   {
      Connections = LoopbackConnections;
      NumConns = sizeof(LoopbackConnections) / sizeof(*Connections);
   }
   else if (pathMode == pathONL)
   {
      Connections = ONLConnections;
      NumConns = sizeof(ONLConnections) / sizeof(*Connections);
   }
   return (ProcessCommandLine(Connections,
                              NumConns,
                              useFIC,
                              rxLoWaterMark,
                              rxHiWaterMark,
                              rtmClkMult,
                              ficClkMult,
                              npuAClkMult,
                              npuBClkMult,
                              maxFrameSize,
                              monitorMode,
                              useFicCalLen));
}
