/* 
** Purpose: Manage Packet Table that defines which packets will be sent from the
**          software bus to a socket.
**
** Notes:
**   1. This has some of the features of a flight app such as packet filtering but it
**      would need design/code reviews to transition it to a flight mission. For starters
**      it uses UDP sockets and it doesn't regulate output bit rates. 
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "osapi-network.h"
#include "app_cfg.h"
#include "pktmgr.h"

/*
** Global File Data
*/

static PKTMGR_Class*  PktMgr = NULL;

/*
** Local Function Prototypes
*/

static void  DestructorCallback(void);
static void  FlushTlmPipe(void);
static int32 SubscribeNewPkt(PKTTBL_Pkt* NewPkt);
static void  ComputeStats(uint16 PktsSent, uint32 BytesSent);


/******************************************************************************
** Function: PKTMGR_Constructor
**
*/
void PKTMGR_Constructor(PKTMGR_Class*  PktMgrPtr, const char* PipeName, uint16 PipeDepth)
{

   PktMgr = PktMgrPtr;

   PktMgr->DownlinkOn   = false;
   PktMgr->SuppressSend = true;
   PktMgr->TlmSockId    = 0;
   strncpy(PktMgr->TlmDestIp, "000.000.000.000", PKTMGR_IP_STR_LEN);

   PKTMGR_InitStats(KIT_TO_RUN_LOOP_DELAY_MS,PKTMGR_STATS_STARTUP_INIT_MS);

   PKTTBL_SetTblToUnused(&(PktMgr->Tbl));

   CFE_SB_CreatePipe(&(PktMgr->TlmPipe), PipeDepth, PipeName);
   
   CFE_MSG_Init(&(PktMgr->PktTlm.TlmHeader.Msg), KIT_TO_PKT_TBL_TLM_MID, PKTMGR_PKT_TLM_LEN);
   
   OS_TaskInstallDeleteHandler((&DestructorCallback)); /* Called when application terminates */

} /* End PKTMGR_Constructor() */


/******************************************************************************
** Function: PKTMGR_GetTblPtr
**
*/
const PKTTBL_Tbl* PKTMGR_GetTblPtr()
{

   return &(PktMgr->Tbl);

} /* End PKTMGR_GetTblPtr() */


/******************************************************************************
** Function:  PKTMGR_ResetStatus
**
*/
void PKTMGR_ResetStatus(void)
{

   PKTMGR_InitStats(0,PKTMGR_STATS_RECONFIG_INIT_MS);

} /* End PKTMGR_ResetStatus() */


/******************************************************************************
** Function:  PKTMGR_InitStats
**
** If OutputTlmInterval==0 then retain current stats
** ComputeStats() logic assumes at least 1 init cycle
**
*/
void PKTMGR_InitStats(uint16 OutputTlmInterval, uint16 InitDelay)
{
   
   if (OutputTlmInterval != 0) PktMgr->Stats.OutputTlmInterval = (double)OutputTlmInterval;

   PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;
   PktMgr->Stats.InitCycles = (PktMgr->Stats.OutputTlmInterval >= InitDelay) ? 1 : (double)InitDelay/PktMgr->Stats.OutputTlmInterval;
            
   PktMgr->Stats.IntervalMilliSecs = 0.0;
   PktMgr->Stats.IntervalPkts = 0;
   PktMgr->Stats.IntervalBytes = 0;
      
   PktMgr->Stats.PrevIntervalAvgPkts  = 0.0;
   PktMgr->Stats.PrevIntervalAvgBytes = 0.0;
   
   PktMgr->Stats.AvgPktsPerSec  = 0.0;
   PktMgr->Stats.AvgBytesPerSec = 0.0;

} /* End PKTMGR_InitStats() */


/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
*/
uint16 PKTMGR_OutputTelemetry(void)
{

   OS_SockAddr_t    SockAddr;
   int32            OsStatus;
   int32            SbStatus;
   size_t           MsgSize;
   CFE_SB_Buffer_t *SbBufPtr;
   CFE_MSG_ApId_t   ApId;
   uint16           NumPktsOutput  = 0;
   uint32           NumBytesOutput = 0;
   
   
   OS_SocketAddrInit(&SockAddr, OS_SocketDomain_INET);
   OS_SocketAddrSetPort(&SockAddr, KIT_TO_TLM_PORT);
   OS_SocketAddrFromString(&SockAddr, PktMgr->TlmDestIp);
   OsStatus = 0;

   do {
       
      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);

      if ( (SbStatus == CFE_SUCCESS) && (PktMgr->SuppressSend == false) ) {
           
         CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgSize);

         if(PktMgr->DownlinkOn) {
            
            CFE_MSG_GetApId(&(SbBufPtr->Msg), &ApId);
            
            if (!PktUtil_IsPacketFiltered(SbBufPtr, &(PktMgr->Tbl.Pkt[ApId].Filter))) {
               
               OsStatus = OS_SocketSendTo(PktMgr->TlmSockId, SbBufPtr, MsgSize, &SockAddr);
          
               ++NumPktsOutput;
               NumBytesOutput += MsgSize;

            } /* End if packet is not filtered */

         }
         else {
            OsStatus = 0;
         }
         
         if (OsStatus < 0) {
                
            CFE_EVS_SendEvent(PKTMGR_SOCKET_SEND_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error sending packet on socket %s, port %d, status %d. Tlm output suppressed\n",
                              PktMgr->TlmDestIp, KIT_TO_TLM_PORT, OsStatus);
            PktMgr->SuppressSend = true;
         
         }
      } /* If CFE_SB_status != CFE_SUCCESS, then no packet was received from CFE_SB_ReceiveBuffer() */
   
   } while (SbStatus == CFE_SUCCESS);

   ComputeStats(NumPktsOutput, NumBytesOutput);

   return NumPktsOutput;
   
} /* End of PKTMGR_OutputTelemetry() */


/******************************************************************************
** Function: PKTMGR_LoadTbl
**
*/
bool PKTMGR_LoadTbl(PKTTBL_Tbl* NewTbl)
{

   CFE_MSG_ApId_t   ApId;
   uint16           PktCnt = 0;
   uint16           FailedSubscription = 0;
   int32            Status;
   bool             RetStatus = true;

   CFE_SB_Buffer_t* SbBufPtr = NULL;

   PKTMGR_RemoveAllPktsCmd(NULL, SbBufPtr);  /* Both parameters are unused so OK to be NULL */

   CFE_PSP_MemCpy(&(PktMgr->Tbl), NewTbl, sizeof(PKTTBL_Tbl));

   for (ApId=0; ApId < PKTTBL_MAX_APP_ID; ApId++) {

      if (PktMgr->Tbl.Pkt[ApId].StreamId != PKTTBL_UNUSED_MSG_ID) {
         
         ++PktCnt;
         Status = SubscribeNewPkt(&(PktMgr->Tbl.Pkt[ApId])); 

         if(Status != CFE_SUCCESS) {
            
            ++FailedSubscription;
            CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_SUBSCRIBE_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error subscribing to stream 0x%04X, BufLim %d, Status %i",
                              PktMgr->Tbl.Pkt[ApId].StreamId, PktMgr->Tbl.Pkt[ApId].BufLim, Status);
         }
      }

   } /* End pkt loop */

   if (FailedSubscription == 0) {
      
      PKTMGR_InitStats(KIT_TO_RUN_LOOP_DELAY_MS,PKTMGR_STATS_STARTUP_INIT_MS);
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_INFO_EID, CFE_EVS_EventType_INFORMATION,
                        "Successfully loaded new table with %d packets", PktCnt);
   }
   else {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ERR_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to load new table with %d packets. Failed %d subscriptions",
                        PktCnt, FailedSubscription);
   }

   return RetStatus;

} /* End PKTMGR_LoadTbl() */


/******************************************************************************
** Function: PKTMGR_LoadTblEntry
**
*/
bool PKTMGR_LoadTblEntry(CFE_MSG_ApId_t ApId, PKTTBL_Pkt* PktArray)
{

   int32       Status;
   bool        RetStatus = true;
   PKTTBL_Pkt* NewPkt = &(PktMgr->Tbl.Pkt[ApId]); 

   CFE_PSP_MemCpy(NewPkt,&PktArray[ApId],sizeof(PKTTBL_Pkt));

   Status = SubscribeNewPkt(NewPkt);
   
   if(Status == CFE_SUCCESS) {
      
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ENTRY_INFO_EID, CFE_EVS_EventType_INFORMATION,
                        "Loaded table entry %d with stream 0x%04X, BufLim %d",
                        ApId, NewPkt->StreamId, NewPkt->BufLim);
   }
   else {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ENTRY_SUBSCRIBE_ERR_EID,CFE_EVS_EventType_ERROR,
                        "Error subscribing to stream 0x%04X, BufLim %d, Status %i",
                        NewPkt->StreamId, NewPkt->BufLim, Status);
   }

   return RetStatus;

} /* End PKTMGR_LoadTblEntry() */


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
*/
bool PKTMGR_EnableOutputCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const PKTMGR_EnableOutputCmdMsg *EnableOutputCmd = (const PKTMGR_EnableOutputCmdMsg *) SbBufPtr;
   bool   RetStatus = false;
   int32  OsStatus;
   
   strncpy(PktMgr->TlmDestIp, EnableOutputCmd->DestIp, PKTMGR_IP_STR_LEN);

   PktMgr->SuppressSend = false;

   /*
   ** If disabled then create the socket and turn it on. If already
   ** enabled then destination address is changed in the existing socket
   */
   if(PktMgr->DownlinkOn == false) { 


      OsStatus = OS_SocketOpen(&PktMgr->TlmSockId , OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
      if (OsStatus == OS_SUCCESS) {
       
         PKTMGR_InitStats(KIT_TO_RUN_LOOP_DELAY_MS,PKTMGR_STATS_STARTUP_INIT_MS);
         PktMgr->DownlinkOn = true;
         RetStatus = true;
         
         CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_EventType_INFORMATION,
                           "Telemetry output enabled for IP %s", PktMgr->TlmDestIp);
      }
      else {
      
         CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_SOCKET_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Telemetry output enable socket error. Status = %d", OsStatus);
      }

   } /* End if downlink disabled */

   return RetStatus;

} /* End PKTMGR_EnableOutputCmd() */


/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Notes:
**   1. Command rejected if table has existing entry for commanded Stream ID
**   2. Only update the table if the software bus subscription successful.  
** 
*/
bool PKTMGR_AddPktCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const PKTMGR_AddPktCmdMsg *AddPktCmd = (const PKTMGR_AddPktCmdMsg *) SbBufPtr;
   PKTTBL_Pkt     NewPkt;
   bool           RetStatus = true;
   int32          Status;
   CFE_MSG_ApId_t ApId;

   
   ApId = AddPktCmd->StreamId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->Tbl.Pkt[ApId].StreamId == PKTTBL_UNUSED_MSG_ID) {
      
      NewPkt.StreamId     = AddPktCmd->StreamId;
      NewPkt.Qos          = AddPktCmd->Qos;
      NewPkt.BufLim       = AddPktCmd->BufLim;
      NewPkt.Filter.Type  = AddPktCmd->FilterType;
      NewPkt.Filter.Param = AddPktCmd->FilterParam;
   
      Status = SubscribeNewPkt(&NewPkt);
   
      if (Status == CFE_SUCCESS) {

         PktMgr->Tbl.Pkt[ApId] = NewPkt;
      
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Added packet 0x%04X, QoS (%d,%d), BufLim %d",
                           NewPkt.StreamId, NewPkt.Qos.Priority, NewPkt.Qos.Reliability, NewPkt.BufLim);
      }
      else {
   
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                           "Error adding packet 0x%04X. Software Bus subscription failed with return status 0x%8x",
                           AddPktCmd->StreamId, Status);
      }
   
   } /* End if packet entry unused */
   else {
   
      CFE_EVS_SendEvent(PKTMGR_ADD_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                        "Error adding packet 0x%04X. Packet already exists in the packet table",
                        AddPktCmd->StreamId);
   }
   
   return RetStatus;

} /* End of PKTMGR_AddPktCmd() */


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The cFE to_lab code unsubscribes the command and send HK MIDs. I'm not
**      sure why this is done and I'm not sure how the command is used. This 
**      command is intended to help manage TO telemetry packets.
*/
bool PKTMGR_RemoveAllPktsCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   CFE_MSG_ApId_t ApId;
   uint16         PktCnt = 0;
   uint16         FailedUnsubscribe = 0;
   int32          Status;
   bool           RetStatus = true;

   for (ApId=0; ApId < PKTTBL_MAX_APP_ID; ApId++) {
      
      if (PktMgr->Tbl.Pkt[ApId].StreamId != PKTTBL_UNUSED_MSG_ID ) {
          
         ++PktCnt;

         Status = CFE_SB_Unsubscribe(PktMgr->Tbl.Pkt[ApId].StreamId, PktMgr->TlmPipe);
         if(Status != CFE_SUCCESS) {
             
            FailedUnsubscribe++;
            CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_ERROR_EID, CFE_EVS_EventType_ERROR,
                              "Error removing stream id 0x%04X at table packet index %d. Unsubscribe status 0x%8X",
                              PktMgr->Tbl.Pkt[ApId].StreamId, ApId, Status);
         }

         PKTTBL_SetPacketToUnused(&(PktMgr->Tbl.Pkt[ApId]));

      } /* End if packet in use */

   } /* End ApId loop */

   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, "PKTMGR_RemoveAllPktsCmd() - About to flush pipe\n");
   FlushTlmPipe();
   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, "PKTMGR_RemoveAllPktsCmd() - Completed pipe flush\n");

   if (FailedUnsubscribe == 0) {
      
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                        "Removed %d table packet entries", PktCnt);
   }
   else {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_ERROR_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to remove %d packet entries. Failed %d unsubscribes",
                        PktCnt, FailedUnsubscribe);
   }

   return RetStatus;

} /* End of PKTMGR_RemoveAllPktsCmd() */


/*******************************************************************
** Function: PKTMGR_RemovePktCmd
**
*/
bool PKTMGR_RemovePktCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const PKTMGR_RemovePktCmdMsg *RemovePktCmd = (const PKTMGR_RemovePktCmdMsg *) SbBufPtr;
   CFE_MSG_ApId_t ApId;
   int32          Status;
   bool           RetStatus = true;
  
   
   ApId = RemovePktCmd->StreamId & PKTTBL_APP_ID_MASK;
  
   if ( PktMgr->Tbl.Pkt[ApId].StreamId != PKTTBL_UNUSED_MSG_ID) {

      PKTTBL_SetPacketToUnused(&(PktMgr->Tbl.Pkt[ApId]));
      
      Status = CFE_SB_Unsubscribe(RemovePktCmd->StreamId, PktMgr->TlmPipe);
      if(Status == CFE_SUCCESS)
      {
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Succesfully removed stream id 0x%04X from the packet table",
                           RemovePktCmd->StreamId);
      }
      else
      {
         RetStatus = false;
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                           "Removed packet 0x%04X from packet table, but SB unsubscribefailed with return status 0x%8x",
                           RemovePktCmd->StreamId, Status);
      }

   } /* End if found stream ID in table */
   else
   {

      CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_ERROR_EID, CFE_EVS_EventType_ERROR,
                        "Error removing stream id 0x%04X. Packet not defined in packet table.",
                        RemovePktCmd->StreamId);

   } /* End if didn't find stream ID in table */

   return RetStatus;

} /* End of PKTMGR_RemovePktCmd() */


/*******************************************************************
** Function: PKTMGR_SendPktTblTlmCmd
**
** Any command 
*/
bool PKTMGR_SendPktTblTlmCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const PKTMGR_SendPktTblTlmCmdMsg *SendPktTblTlmCmd = (const PKTMGR_SendPktTblTlmCmdMsg *) SbBufPtr;
   CFE_MSG_ApId_t ApId;
   PKTTBL_Pkt*    PktPtr;
   int32          SbStatus;
  
  
   ApId   = SendPktTblTlmCmd->StreamId & PKTTBL_APP_ID_MASK;
   PktPtr = &(PktMgr->Tbl.Pkt[ApId]);
   
   PktMgr->PktTlm.StreamId = PktPtr->StreamId;
   PktMgr->PktTlm.Qos      = PktPtr->Qos;
   PktMgr->PktTlm.BufLim   = PktPtr->BufLim;

   PktMgr->PktTlm.FilterType  = PktPtr->Filter.Type;
   PktMgr->PktTlm.FilterParam = PktPtr->Filter.Param;

   CFE_SB_TimeStampMsg(&(PktMgr->PktTlm.TlmHeader.Msg));
   SbStatus = CFE_SB_TransmitMsg(&(PktMgr->PktTlm.TlmHeader.Msg), true);
   
   return (SbStatus == CFE_SUCCESS);

} /* End of PKTMGR_SendPktTblTlmCmd() */


/******************************************************************************
** Function: PKTMGR_UpdateFilterCmd
**
** Notes:
**   1. Command rejected if ApId packet entry has not been loaded 
**   2. The filter type is verified but the filter parameter values are not 
** 
*/
bool PKTMGR_UpdateFilterCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const PKTMGR_UpdateFilterCmdMsg *UpdateFilterCmd = (const PKTMGR_UpdateFilterCmdMsg *) SbBufPtr;
   bool           RetStatus = false;
   CFE_MSG_ApId_t ApId;

   
   ApId = UpdateFilterCmd->StreamId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->Tbl.Pkt[ApId].StreamId != PKTTBL_UNUSED_MSG_ID) {
      
      if (PktUtil_IsFilterTypeValid(UpdateFilterCmd->FilterType)) {
        
         PktUtil_Filter* TblFilter = &(PktMgr->Tbl.Pkt[ApId].Filter);
         
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Successfully changed 0x%04X's filter (Type,N,X,O) from (%d,%d,%d,%d) to (%d,%d,%d,%d)",
                           UpdateFilterCmd->StreamId,
                           TblFilter->Type, TblFilter->Param.N, TblFilter->Param.X, TblFilter->Param.O,
                           UpdateFilterCmd->FilterType,   UpdateFilterCmd->FilterParam.N,
                           UpdateFilterCmd->FilterParam.X,UpdateFilterCmd->FilterParam.O);
                           
         TblFilter->Type  = UpdateFilterCmd->FilterType;
         TblFilter->Param = UpdateFilterCmd->FilterParam;         
        
         RetStatus = true;
      
      } /* End if valid packet filter type */
      else {
   
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error updating filter for packet 0x%04X. Invalid filter type %d",
                           UpdateFilterCmd->StreamId, UpdateFilterCmd->FilterType);
      }
   
   } /* End if packet entry unused */
   else {
   
      CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error updating filter for packet 0x%04X. Packet not in use",
                        UpdateFilterCmd->StreamId);
   }
   
   return RetStatus;

} /* End of PKTMGR_UpdateFilterCmd() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the app is killed. This should
** never occur but if it does this will close the network socket.
*/
static void DestructorCallback(void)
{

   CFE_EVS_SendEvent(PKTMGR_DESTRUCTOR_INFO_EID, CFE_EVS_EventType_INFORMATION, "Destructor callback -- Closing TO Network socket. Downlink on = %d\n", PktMgr->DownlinkOn);
   
   if (PktMgr->DownlinkOn) {
      
      close(PktMgr->TlmSockId);
   
   }

} /* End DestructorCallback() */

/******************************************************************************
** Function: FlushTlmPipe
**
** Remove all of the packets from the input pipe.
**
*/
static void FlushTlmPipe(void)
{

   int32             Status;
   CFE_SB_Buffer_t*  SbBufPtr;

   do {
      
      Status = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);

   } while(Status == CFE_SUCCESS);

} /* End FlushTlmPipe() */


/******************************************************************************
** Function: SubscribeNewPkt
**
*/
static int32 SubscribeNewPkt(PKTTBL_Pkt* NewPkt)
{

   int32 Status;

   Status = CFE_SB_SubscribeEx(NewPkt->StreamId, PktMgr->TlmPipe, NewPkt->Qos, NewPkt->BufLim);

   return Status;

} /* End SubscribeNewPkt(() */


/******************************************************************************
** Function:  ComputeStats
**
** Called each output telemetry cycle
*/
static void ComputeStats(uint16 PktsSent, uint32 BytesSent)
{

   uint32 DeltaTimeMicroSec;   
   CFE_TIME_SysTime_t CurrTime = CFE_TIME_GetTime();
   CFE_TIME_SysTime_t DeltaTime;
   
   if (PktMgr->Stats.InitCycles > 0) {
   
      --PktMgr->Stats.InitCycles;
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;

   }
   else {
      
      DeltaTime = CFE_TIME_Subtract(CurrTime, PktMgr->Stats.PrevTime);
      DeltaTimeMicroSec = CFE_TIME_Sub2MicroSecs(DeltaTime.Subseconds); 
      
      PktMgr->Stats.IntervalMilliSecs += (double)DeltaTime.Seconds*1000.0 + (double)DeltaTimeMicroSec/1000.0;
      PktMgr->Stats.IntervalPkts      += PktsSent;
      PktMgr->Stats.IntervalBytes     += BytesSent;

      if (PktMgr->Stats.IntervalMilliSecs >= PKTMGR_COMPUTE_STATS_INTERVAL_MS) {
      
         double Seconds = PktMgr->Stats.IntervalMilliSecs/1000;
         
         CFE_EVS_SendEvent(PKTMGR_DEBUG_EID, CFE_EVS_EventType_DEBUG,
                           "IntervalSecs=%f, IntervalPkts=%d, IntervalBytes=%d\n",
                           Seconds,PktMgr->Stats.IntervalPkts,PktMgr->Stats.IntervalBytes);
        
         PktMgr->Stats.AvgPktsPerSec  = (double)PktMgr->Stats.IntervalPkts/Seconds;
         PktMgr->Stats.AvgBytesPerSec = (double)PktMgr->Stats.IntervalBytes/Seconds;
         
         /* Good enough running average that avoids overflow */
         if (PktMgr->Stats.State == PKTMGR_STATS_INIT_CYCLE) {
     
            PktMgr->Stats.State = PKTMGR_STATS_INIT_INTERVAL;
       
         }
         else {
            
            PktMgr->Stats.State = PKTMGR_STATS_VALID;
            PktMgr->Stats.AvgPktsPerSec  = (PktMgr->Stats.AvgPktsPerSec  + PktMgr->Stats.PrevIntervalAvgPkts) / 2.0; 
            PktMgr->Stats.AvgBytesPerSec = (PktMgr->Stats.AvgBytesPerSec + PktMgr->Stats.PrevIntervalAvgBytes) / 2.0; 
  
         }
         
         PktMgr->Stats.PrevIntervalAvgPkts  = PktMgr->Stats.AvgPktsPerSec;
         PktMgr->Stats.PrevIntervalAvgBytes = PktMgr->Stats.AvgBytesPerSec;
         
         PktMgr->Stats.IntervalMilliSecs = 0.0;
         PktMgr->Stats.IntervalPkts      = 0;
         PktMgr->Stats.IntervalBytes     = 0;
      
      } /* End if report cycle */
      
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      
   } /* End if not init cycle */
   

} /* End ComputeStats() */
