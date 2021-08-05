/* 
** Purpose: Manage a UDP uplink command interface.
**
** Notes:
**   None
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
#include <unistd.h>  /* Needed for close() */


#include "uplink.h"

/*
** Global File Data
*/

static UPLINK_Class*  Uplink = NULL;

/*
** Local Function Prototypes
*/

static void DestructorCallback(void);
static void ProcessMsgTunnelMap(void);

/******************************************************************************
** Function: UPLINK_ConfigMsgTunnelCmd
**
*/
bool UPLINK_ConfigMsgTunnelCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   const  UPLINK_ConfigMsgTunnelCmdMsg *ConfigMsgTunnel = (const UPLINK_ConfigMsgTunnelCmdMsg *) SbBufPtr;

   bool    RetStatus = true;
   int     Index     = ConfigMsgTunnel->Index;

   if (Index < UPLINK_MSG_TUNNEL_CNT) {

      if (ConfigMsgTunnel->Enabled == true) {

         Uplink->MsgTunnel.Map[Index].Enabled = true;
         Uplink->MsgTunnel.Map[Index].OrgMsgId = ConfigMsgTunnel->OrgMsgId;
         Uplink->MsgTunnel.Map[Index].NewMsgId = ConfigMsgTunnel->NewMsgId;
         Uplink->MsgTunnel.Enabled = true;

         CFE_EVS_SendEvent(UPLINK_CFG_MSG_TUNNEL_ENA_EID, CFE_EVS_EventType_INFORMATION,
                           "UPLINK: Enabled message tunnel map at index %d. Message 0x%04X mapped to 0x%04X",
                           Index, Uplink->MsgTunnel.Map[Index].OrgMsgId, Uplink->MsgTunnel.Map[Index].NewMsgId);

      } /* End if enabling */
      else {

         Uplink->MsgTunnel.Map[Index].Enabled = false;
         CFE_EVS_SendEvent(UPLINK_CFG_MSG_TUNNEL_DIS_EID, CFE_EVS_EventType_INFORMATION,
                           "UPLINK: Disabled message tunnel map at index %d. Message 0x%04X was mapped to 0x%04X",
                           Index, Uplink->MsgTunnel.Map[Index].OrgMsgId, Uplink->MsgTunnel.Map[Index].NewMsgId);
         Uplink->MsgTunnel.Map[Index].OrgMsgId = UPLINK_UNUSED_MSG_ID;
         Uplink->MsgTunnel.Map[Index].NewMsgId = UPLINK_UNUSED_MSG_ID;

         /* Set global flag to true if any mappings are enabled */
         for (Index=0; Index < UPLINK_MSG_TUNNEL_CNT; Index++)
      	   Uplink->MsgTunnel.Enabled |= (Uplink->MsgTunnel.Map[Index].Enabled == true);


      } /* End if disabling */

   } /* End if valid index */
   else {
	   
      RetStatus = false;
       
      CFE_EVS_SendEvent(UPLINK_CFG_MSG_TUNNEL_IDX_ERR_EID, CFE_EVS_EventType_ERROR,
                        "UPLINK: Configure message tunnel index %d exceeds maximum index of %d",
                        Index, (UPLINK_MSG_TUNNEL_CNT-1));

   } /* End if invalid index */

   return RetStatus;

} /* End of UPLINK_ConfigMsgTunnelCmd() */


/******************************************************************************
** Function: UPLINK_Constructor
**
*/
void UPLINK_Constructor(UPLINK_Class*  UplinkPtr, uint16 Port)
{
 
   int   i;
   int32 Status;
   
   
   Uplink = UplinkPtr;
   
   /*
   Uplink->Connected     = false;
   Uplink->RecvMsgIdx    = 0;
   Uplink->RecvMsgCnt    = 0;
   Uplink->RecvMsgErrCnt = 0;
   Uplink->RecvSbBufPtr  = NULL;
   */
   memset(Uplink,0,sizeof(UPLINK_Class));

   Uplink->MsgTunnel.Enabled = false;
   for (i=0; i < UPLINK_MSG_TUNNEL_CNT; i++) {
      
      Uplink->MsgTunnel.Map[i].Enabled  = false;
      Uplink->MsgTunnel.Map[i].OrgMsgId = UPLINK_UNUSED_MSG_ID;
      Uplink->MsgTunnel.Map[i].NewMsgId = UPLINK_UNUSED_MSG_ID;
   }
   Uplink->MsgTunnel.MappingsPerformed    = 0;
   Uplink->MsgTunnel.LastMapping.Index    = UPLINK_MSG_TUNNEL_CNT;
   Uplink->MsgTunnel.LastMapping.OrgMsgId = UPLINK_UNUSED_MSG_ID;
   Uplink->MsgTunnel.LastMapping.NewMsgId = UPLINK_UNUSED_MSG_ID;


   Status = OS_SocketOpen(&Uplink->SocketId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
   if (Status == OS_SUCCESS) {
      
      OS_SocketAddrInit(&Uplink->SocketAddress, OS_SocketDomain_INET);
      OS_SocketAddrSetPort(&Uplink->SocketAddress, Port);

      Status = OS_SocketBind(Uplink->SocketId, &Uplink->SocketAddress);

      if (Status == OS_SUCCESS) {
           
         Uplink->Connected = true;
         CFE_EVS_SendEvent(UPLINK_SOCKET_CREATED_EID,CFE_EVS_EventType_INFORMATION,
                           "UPLINK: Listening on UDP port: %u", (unsigned int)Port);
      
      }
      else {
         
         CFE_EVS_SendEvent(UPLINK_SOCKET_BIND_ERR_EID,CFE_EVS_EventType_ERROR,
                           "UPLINK: Socket bind failed. Status = %d", (int)Status);
      }

   } /* End if open socket */
   else {
      
      CFE_EVS_SendEvent(UPLINK_SOCKET_CREATE_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "UPLINK: Socket creation failed. Status = %d", (int)Status);
   }
   
   OS_TaskInstallDeleteHandler(&DestructorCallback); /* Call when application terminates */

} /* End UPLINK_Constructor() */


/******************************************************************************
** Function: UPLINK_Read
**
*/
int UPLINK_Read(uint16 MaxMsgRead)
{

   int  i = 0;
   int  Status;
   uint8* MsgBytes;
   UPLINK_SocketRecvCmdMsg *RecvCmdMsg; 
   
    
   if (Uplink->Connected == false) return i;

   
   for (i = 0; i < MaxMsgRead; i++) {

      RecvCmdMsg = &(Uplink->RecvMsg[Uplink->RecvMsgIdx]);
      
      /*
      if (Uplink->RecvSbBufPtr == NULL) {
      
         Uplink->RecvSbBufPtr = CFE_SB_AllocateMessageBuffer(UPLINK_RECV_BUFF_LEN);
      
         if (Uplink->RecvSbBufPtr == NULL) {
         
            CFE_EVS_SendEvent(UPLINK_RECV_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Failed to allocate SB buffer of length %d", UPLINK_RECV_BUFF_LEN);
            
            break;
         }
      }   
      */
      Status = OS_SocketRecvFrom(Uplink->SocketId, RecvCmdMsg, sizeof(UPLINK_SocketRecvCmdMsg),
                                 &(Uplink->SocketAddress), OS_CHECK);
      
      
      //~Status = OS_SocketRecvFrom(Uplink->SocketId, Uplink->RecvSbBufPtr, UPLINK_RECV_BUFF_LEN,
      //~                           &(Uplink->SocketAddress), OS_CHECK);
      
      Uplink->RecvMsgIdx++;
      if (Uplink->RecvMsgIdx >= UPLINK_RECV_BUFF_CNT) Uplink->RecvMsgIdx = 0;
                                 
      if (Status >= (int32)sizeof(CFE_MSG_CommandHeader_t) && Status <= ((int32)UPLINK_RECV_BUFF_LEN)) {
         
         CFE_EVS_SendEvent(UPLINK_DEBUG_EID, CFE_EVS_EventType_INFORMATION, "UPLINK: Read %d bytes from socket",Status);
         Uplink->RecvMsgCnt++;
         if (Uplink->MsgTunnel.Enabled) ProcessMsgTunnelMap();

         Status = CFE_SB_TransmitMsg( (void *)&(RecvCmdMsg->CmdHeader.Msg), false); //~TODO - cfe7.0 (CFE_SB_Buffer_t*)
         //~Status = CFE_SB_TransmitBuffer( Uplink->RecvSbBufPtr, false); 

         if (Status == CFE_SUCCESS) {
           
            Uplink->RecvSbBufPtr = NULL;
         
         }   
         else {

            CFE_EVS_SendEvent(UPLINK_SEND_SB_MSG_ERR_EID, CFE_EVS_EventType_ERROR,
                             "UPLINK CFE_SB_TransmitBuffer() failed, Status = 0x%X", (int)Status);
         }
      
      } /* End if valid len range */
      else if (Status > 0) {
         
         Uplink->RecvMsgErrCnt++;
         MsgBytes = Uplink->RecvSbBufPtr->Msg.Byte;
         CFE_EVS_SendEvent(UPLINK_RECV_ERR_EID, CFE_EVS_EventType_ERROR,
                           "UPLINK: Command dropped, Bad length %d. Bytes: 0x%02x%02x 0x%02x%02x ", (int)Status,
                            MsgBytes[0], MsgBytes[1], MsgBytes[2], MsgBytes[3]);
         
         //~ TODO - cfe7.0 Format header
         /*
         CFE_EVS_SendEvent(UPLINK_RECV_ERR_EID,CFE_EVS_EventType_ERROR,
                           "UPLINK: Command dropped, Bad length %d. Header: 0x%02x%2x 0x%02x%2x 0x%02x%2x 0x%02x%2x", (int)Status,
                		         Uplink->RecvBuff[0], Uplink->RecvBuff[1], Uplink->RecvBuff[2], Uplink->RecvBuff[3],
                              Uplink->RecvBuff[4], Uplink->RecvBuff[5], Uplink->RecvBuff[6], Uplink->RecvBuff[7]);
         */
      }
      else {
         break; /* no (more) messages */
      }

   } /* End receive loop */

   return i;

} /* End UPLINK_Read() */


/******************************************************************************
** Function:  UPLINK_ResetStatus
**
*/
void UPLINK_ResetStatus()
{

   Uplink->RecvMsgCnt = 0;
   Uplink->RecvMsgErrCnt = 0;
   Uplink->MsgTunnel.MappingsPerformed = 0;
   Uplink->MsgTunnel.LastMapping.Index = UPLINK_MSG_TUNNEL_CNT;
   Uplink->MsgTunnel.LastMapping.OrgMsgId = UPLINK_UNUSED_MSG_ID;
   Uplink->MsgTunnel.LastMapping.NewMsgId = UPLINK_UNUSED_MSG_ID;

} /* End UPLINK_ResetStatus() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the uplink is terminated. This should
** never occur but if it does this will close the network socket.
*/
static void DestructorCallback(void)
{

   CFE_EVS_SendEvent(UPLINK_DESTRUCTOR_EID, CFE_EVS_EventType_INFORMATION, 
                     "UPLINK: UPLINK deleting callback. Closing Network socket.");

   OS_close(Uplink->SocketId);

} /* End DestructorCallback() */

/******************************************************************************
** Function: ProcessMsgTunnelMap
**
** This function should only be called if message tunneling is enabled in order
** to save processing time. It loops thru the message tunnel map and if the
** current input message idea is matched and the mapping is enabled then the
** message ID is replaced.
**
*/
static void ProcessMsgTunnelMap(void)
{

   int  i;
   CFE_SB_MsgId_t     OrgMsgId;
   CFE_MSG_Message_t* MsgPtr;
   

   MsgPtr = &((Uplink->RecvMsg[Uplink->RecvMsgIdx]).CmdHeader.Msg);
   
   CFE_MSG_GetMsgId(MsgPtr, &OrgMsgId);
   
   for (i=0; i < UPLINK_MSG_TUNNEL_CNT; i++) {

      if (Uplink->MsgTunnel.Map[i].Enabled) {

    	   if (OrgMsgId == Uplink->MsgTunnel.Map[i].OrgMsgId) {
          
            CFE_MSG_SetMsgId(MsgPtr, Uplink->MsgTunnel.Map[i].NewMsgId);
            CFE_MSG_GenerateChecksum(MsgPtr);
            Uplink->MsgTunnel.MappingsPerformed++;
            Uplink->MsgTunnel.LastMapping.Index = i;
            Uplink->MsgTunnel.LastMapping.OrgMsgId = OrgMsgId;
            Uplink->MsgTunnel.LastMapping.NewMsgId = Uplink->MsgTunnel.Map[i].NewMsgId;
            break;
         }
      } /* End if tunnel enabled */

   } /* End map loop */

} /* End ProcessMsgTunnelMap() */
