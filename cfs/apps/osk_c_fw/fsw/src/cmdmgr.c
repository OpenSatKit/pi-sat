/* 
** Purpose: Manage command dispatching for an application.
**
** Notes:
**   1. This code must be reentrant so no global data is used. 
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

#include "cmdmgr.h"


/***********************/
/** Macro Definitions **/
/***********************/

///#define DBG_CMDMGR


/**********************/
/** Global File Data **/
/**********************/

static const char* BoolStr[] = {
   "FALSE",
   "TRUE",
   "UNDEF"
};


/******************************/
/** File Function Prototypes **/
/******************************/

static bool UnusedFuncCode(void* ObjDataPtr, const CFE_SB_Buffer_t* MsgPtr);


/******************************************************************************
** Function: CMDMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same cmdmgr instance.
**
*/
void CMDMGR_Constructor(CMDMGR_Class* CmdMgr)
{

   int i;

   CFE_PSP_MemSet(CmdMgr, 0, sizeof(CMDMGR_Class));
   for (i=0; i < CMDMGR_CMD_FUNC_TOTAL; i++) {
      
      CmdMgr->Cmd[i].FuncPtr = UnusedFuncCode;
   
   }

} /* End CMDMGR_Constructor() */


/******************************************************************************
** Function: CMDMGR_RegisterFunc
**
*/
bool CMDMGR_RegisterFunc(CMDMGR_Class* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                         CMDMGR_CmdFuncPtr ObjFuncPtr, uint16 UserDataLen)
{

   bool    RetStatus = false;
   
   if (FuncCode < CMDMGR_CMD_FUNC_TOTAL) {

      if (DBG_CMDMGR) OS_printf("CMDMGR_RegisterFunc(): FuncCode %d, DataLen %d\n", FuncCode, UserDataLen);
      CmdMgr->Cmd[FuncCode].DataPtr = ObjDataPtr;
      CmdMgr->Cmd[FuncCode].FuncPtr = ObjFuncPtr;
      CmdMgr->Cmd[FuncCode].UserDataLen = UserDataLen;

      CmdMgr->Cmd[FuncCode].AltCnt.Enabled = false   ;
      CmdMgr->Cmd[FuncCode].AltCnt.Valid   = 0;
      CmdMgr->Cmd[FuncCode].AltCnt.Invalid = 0;
      
      RetStatus = true   ;

   }
   else {
      
      CFE_EVS_SendEvent (CMDMGR_REG_INVALID_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Attempt to register function code %d which is greater than max %d",
         FuncCode,(CMDMGR_CMD_FUNC_TOTAL-1));
   }

   return RetStatus;
   
} /* End CMDMGR_RegisterFunc() */


/******************************************************************************
** Function: CMDMGR_RegisterFuncAltCnt
**
*/
bool CMDMGR_RegisterFuncAltCnt(CMDMGR_Class* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                           CMDMGR_CmdFuncPtr ObjFuncPtr, uint16 UserDataLen)
{

   bool    RetStatus = false;

   if (CMDMGR_RegisterFunc(CmdMgr, FuncCode, ObjDataPtr, ObjFuncPtr, UserDataLen)) {
      
      CmdMgr->Cmd[FuncCode].AltCnt.Enabled = true   ;      

      RetStatus = true;

   }

   return RetStatus;
   
} /* End CMDMGR_RegisterFuncAltCnt() */


/******************************************************************************
** Function: CMDMGR_ResetStatus
**
** Keep logic simple and clear all alternate counters regardless of whether 
** they're enabled. 
**
*/
void CMDMGR_ResetStatus(CMDMGR_Class* CmdMgr)
{

   int i;

   CmdMgr->ValidCmdCnt   = 0;
   CmdMgr->InvalidCmdCnt = 0;

   for (i=0; i < CMDMGR_CMD_FUNC_TOTAL; i++) {
      
      CmdMgr->Cmd[i].AltCnt.Valid   = 0;
      CmdMgr->Cmd[i].AltCnt.Invalid = 0;
   
   }
   
} /* End CMDMGR_ResetStatus() */


/******************************************************************************
** Function: CMDMGR_DispatchFunc
**
** Notes:
**   1. Considered sending an event message for alternate counter commands, but
**      decided this is the client's responsibility. CmdMgr is a dispatcher and
**      if an app wants a message response then it should publish the format. 
**   2. cFE 7.0 removed CFE_SB_GetChecksum() so it is not reported in the event
**      message. 
**
*/
bool CMDMGR_DispatchFunc(CMDMGR_Class* CmdMgr, const CFE_SB_Buffer_t*  SbBufPtr)
{

   bool   ValidCmd = false;
   bool   ChecksumValid;
   size_t UserDataLen; 
   CFE_MSG_FcnCode_t FuncCode;

   UserDataLen = CFE_SB_GetUserDataLength(&SbBufPtr->Msg);

   CFE_MSG_GetFcnCode(&SbBufPtr->Msg, &FuncCode);
   CFE_MSG_ValidateChecksum(&SbBufPtr->Msg, &ChecksumValid);
   
   if (DBG_CMDMGR) OS_printf("CMDMGR_DispatchFunc(): [0]=0x%X, [1]=0x%X, [2]=0x%X, [3]=0x%X\n",
                             ((uint8*)SbBufPtr)[0],((uint8*)SbBufPtr)[1],((uint8*)SbBufPtr)[2],((uint8*)SbBufPtr)[3]);
   if (DBG_CMDMGR) OS_printf("CMDMGR_DispatchFunc(): [4]=0x%X, [5]=0x%X, [6]=0x%X, [7]=0x%X\n",
                             ((uint8*)SbBufPtr)[4],((uint8*)SbBufPtr)[5],((uint8*)SbBufPtr)[6],((uint8*)SbBufPtr)[7]);
   if (DBG_CMDMGR) OS_printf("CMDMGR_DispatchFunc(): FuncCode %d, DataLen %d\n", FuncCode,UserDataLen);

   if (FuncCode < CMDMGR_CMD_FUNC_TOTAL) {

      if (UserDataLen == CmdMgr->Cmd[FuncCode].UserDataLen) {

         if (ChecksumValid) {

            ValidCmd = (CmdMgr->Cmd[FuncCode].FuncPtr)(CmdMgr->Cmd[FuncCode].DataPtr, SbBufPtr);

         } /* End if valid checksum */
         else {

            CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_CHECKSUM_ERR_EID, CFE_EVS_EventType_ERROR,
                               "Invalid command checksum");
         
         }
      } /* End if valid length */
      else {

         CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                            "Invalid command user data length %d, expected %d",
                            UserDataLen, CmdMgr->Cmd[FuncCode].UserDataLen);

      }

   } /* End if valid function code */
   else {
      
      CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
                         "Invalid command function code %d is greater than max %d",
                         FuncCode, (CMDMGR_CMD_FUNC_TOTAL-1));

   } /* End if invalid function code */

   if (CmdMgr->Cmd[FuncCode].AltCnt.Enabled) {
   
      ValidCmd ? CmdMgr->Cmd[FuncCode].AltCnt.Valid++ : CmdMgr->Cmd[FuncCode].AltCnt.Invalid++;
   
   } 
   else {
   
      ValidCmd ? CmdMgr->ValidCmdCnt++ : CmdMgr->InvalidCmdCnt++;
   
   }
   
   return ValidCmd;

} /* End CMDMGR_DispatchFunc() */


/******************************************************************************
** Function: UnusedFuncCode
**
*/
static bool UnusedFuncCode(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   CFE_MSG_FcnCode_t FuncCode;
   
   CFE_MSG_GetFcnCode(&SbBufPtr->Msg, &FuncCode);
   CFE_EVS_SendEvent (CMDMGR_DISPATCH_UNUSED_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
                      "Unused command function code %d received",FuncCode);

   return false;

} /* End UnusedFuncCode() */


/******************************************************************************
** Function: CMDMGR_ValidBoolArg
**
*/
bool CMDMGR_ValidBoolArg(uint16 BoolArg)
{
   
   return ((BoolArg == true) || (BoolArg == false));

} /* CMDMGR_ValidBoolArg() */


/******************************************************************************
** Function: CMDMGR_BoolStr
**
** Purpose: Return a pointer to a string describing a bool   
**
** Notes:
**   Assumes false=0 and true=1
*/
const char* CMDMGR_BoolStr(bool BoolArg)
{
   
   uint8 i = 2;
   
   if ( BoolArg == true || BoolArg == false) {
   
      i = BoolArg;
   
   }
        
   return BoolStr[i];

} /* End CMDMGR_BoolStr() */

