/*
** Purpose: Implement the File Manager application
**
** Notes:
**   1. See header notes. 
**
** References:
**   1. OpenSat Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
*/

/*
** Includes
*/

#include <string.h>
#include "filemgr_app.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (&(FileMgr.IniTbl))
#define  CMDMGR_OBJ   (&(FileMgr.CmdMgr))
#define  TBLMGR_OBJ   (&(FileMgr.TblMgr))
#define  CHILDMGR_OBJ (&(FileMgr.ChildMgr))
#define  DIR_OBJ      (&(FileMgr.Dir))
#define  FILE_OBJ     (&(FileMgr.File))
#define  FILESYS_OBJ  (&(FileMgr.FileSys))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);

/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  


/*****************/
/** Global Data **/
/*****************/

FILEMGR_Class  FileMgr;


/******************************************************************************
** Function: FILEMGR_AppMain
**
*/
void FILEMGR_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;


   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS) {  /* Performs initial CFE_ES_PerfLogEntry() call */
   
      RunStatus = CFE_ES_RunStatus_APP_RUN;
      
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus)) {

      RunStatus = ProcessCommands(); /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */

   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("FILEMGR App terminating, err = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(FILEMGR_EXIT_EID, CFE_EVS_EventType_CRITICAL, "FILEMGR App terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of FILEMGR_AppMain() */


/******************************************************************************
** Function: FILEMGR_NoOpCmd
**
*/

bool    FILEMGR_NoOpCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   CFE_EVS_SendEvent (FILEMGR_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for FILEMGR App version %d.%d.%d",
                      FILEMGR_MAJOR_VER, FILEMGR_MINOR_VER, FILEMGR_PLATFORM_REV);

   return true;


} /* End FILEMGR_NoOpCmd() */


/******************************************************************************
** Function: FILEMGR_ResetAppCmd
**
*/

bool FILEMGR_ResetAppCmd(void* ObjDataPtr, const CFE_SB_Buffer_t* SbBufPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   CHILDMGR_ResetStatus(CHILDMGR_OBJ);
   
   DIR_ResetStatus();
   FILE_ResetStatus();
   FILESYS_ResetStatus();
	  
   return true;

} /* End FILEMGR_ResetAppCmd() */


/******************************************************************************
** Function: FILEMGR_SendHousekeepingPkt
**
*/
void FILEMGR_SendHousekeepingPkt(void)
{
   
   FileMgr.HkPkt.CommandCounter    = FileMgr.CmdMgr.ValidCmdCnt;
   FileMgr.HkPkt.CommandErrCounter = FileMgr.CmdMgr.InvalidCmdCnt;

   FileMgr.HkPkt.NumOpenFiles = FileUtil_GetOpenFileList(&FileMgr.OpenFileList);

   FileMgr.HkPkt.ChildCmdCounter     = FileMgr.ChildMgr.ValidCmdCnt;
   FileMgr.HkPkt.ChildCmdErrCounter  = FileMgr.ChildMgr.InvalidCmdCnt;
   FileMgr.HkPkt.ChildCmdWarnCounter = FileMgr.File.CmdWarningCnt + FileMgr.Dir.CmdWarningCnt;
 
   FileMgr.HkPkt.ChildQueueCount = FileMgr.ChildMgr.CmdQ.Count;

   FileMgr.HkPkt.ChildCurrentCC  = FileMgr.ChildMgr.CurrCmdCode;
   FileMgr.HkPkt.ChildPreviousCC = FileMgr.ChildMgr.PrevCmdCode;

   CFE_SB_TimeStampMsg(&(FileMgr.HkPkt.TlmHeader.Msg));
   CFE_SB_TransmitMsg(&(FileMgr.HkPkt.TlmHeader.Msg), true);
   
} /* End FILEMGR_SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = OSK_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit ChildTaskInit;
   
   /*
   ** Initialize objects 
   */

   if (INITBL_Constructor(&FileMgr.IniTbl, FILEMGR_INI_FILENAME, &IniCfgEnum)) {
   
      FileMgr.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      FileMgr.CmdMid    = (CFE_SB_MsgId_t)INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_MID);
      FileMgr.SendHkMid = (CFE_SB_MsgId_t)INITBL_GetIntConfig(INITBL_OBJ, CFG_SEND_HK_MID);
      CFE_ES_PerfLogEntry(FileMgr.PerfId);

      /* Constructor sends error events */    
      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CHILD_TASK_PERF_ID);
      Status = CHILDMGR_Constructor(CHILDMGR_OBJ, 
                                    ChildMgr_TaskMainCmdDispatch,
                                    NULL, 
                                    &ChildTaskInit); 
  
   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS) {

      DIR_Constructor(DIR_OBJ, &FileMgr.IniTbl);
      FILE_Constructor(FILE_OBJ, &FileMgr.IniTbl);
      FILESYS_Constructor(FILESYS_OBJ, &FileMgr.IniTbl);


      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&FileMgr.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(FileMgr.CmdMid,    FileMgr.CmdPipe);
      CFE_SB_Subscribe(FileMgr.SendHkMid, FileMgr.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,   NULL, FILEMGR_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,  NULL, FILEMGR_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, DIR_CREATE_CMD_FC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, DIR_CREATE_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, DIR_DELETE_CMD_FC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, DIR_DELETE_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, DIR_DELETE_ALL_CMD_FC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, DIR_DELETE_ALL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, DIR_SEND_LIST_PKT_CMD_FC,   CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, DIR_SEND_LIST_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, DIR_WRITE_LIST_FILE_CMD_FC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, DIR_WRITE_LIST_FILE_CMD_DATA_LEN);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, DIR_CREATE_CMD_FC,          DIR_OBJ, DIR_CreateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, DIR_DELETE_CMD_FC,          DIR_OBJ, DIR_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, DIR_DELETE_ALL_CMD_FC,      DIR_OBJ, DIR_DeleteAllCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, DIR_SEND_LIST_PKT_CMD_FC,   DIR_OBJ, DIR_SendListPktCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, DIR_WRITE_LIST_FILE_CMD_FC, DIR_OBJ, DIR_WriteListFileCmd);


      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_CONCAT_CMD_FC,    CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_CONCATENATE_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_COPY_CMD_FC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_COPY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_DECOMPRESS_CMD_FC,CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_DECOMPRESS_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_DELETE_CMD_FC,    CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_DELETE_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MOVE_CMD_FC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_MOVE_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_RENAME_CMD_FC,    CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_RENAME_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_SEND_INFO_CMD_FC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_SEND_INFO_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_SET_PERMISSIONS_CMD_FC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_SET_PERMISSIONS_CMD_DATA_LEN);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_CONCAT_CMD_FC,    FILE_OBJ, FILE_ConcatenateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_COPY_CMD_FC,      FILE_OBJ, FILE_CopyCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_DECOMPRESS_CMD_FC,FILE_OBJ, FILE_DecompressCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_DELETE_CMD_FC,    FILE_OBJ, FILE_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MOVE_CMD_FC,      FILE_OBJ, FILE_MoveCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_RENAME_CMD_FC,    FILE_OBJ, FILE_RenameCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_SEND_INFO_CMD_FC, FILE_OBJ, FILE_SendInfoPktCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_SET_PERMISSIONS_CMD_FC, FILE_OBJ, FILE_SetPermissionsCmd);

      /* 
      ** Alternative commands don't increment the main command counters. They do increment the child command counters which mimics
      ** the original FM app behavior, but I'm not sure that's desirable since the child counters are also used by ground ops.
      */
      CMDMGR_RegisterFuncAltCnt(CMDMGR_OBJ, FILE_DELETE_ALT_CMD_FC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, FILE_DELETE_CMD_DATA_LEN);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_DELETE_ALT_CMD_FC, FILE_OBJ, FILE_DeleteCmd);
 
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILESYS_SEND_OPEN_FILES_PKT_CMD_FC,  FILESYS_OBJ, FILESYS_SendOpenFilesPktCmd,  FILESYS_SEND_OPEN_FILES_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILESYS_SEND_TBL_PKT_CMD_FC,         FILESYS_OBJ, FILESYS_SendTblPktCmd,        FILESYS_SEND_TBL_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILESYS_SET_TBL_STATE_CMD_FC,        FILESYS_OBJ, FILESYS_SetTblStateCmd,       FILESYS_SET_TBL_STATE_CMD_DATA_LEN);

      CFE_MSG_Init(&FileMgr.HkPkt.TlmHeader.Msg, (CFE_SB_MsgId_t)INITBL_GetIntConfig(INITBL_OBJ, CFG_HK_TLM_MID), FILEMGR_TLM_HK_LEN);

	   TBLMGR_Constructor(TBLMGR_OBJ);
   
      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(FILEMGR_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "FILEMGR App Initialized. Version %d.%d.%d",
                        FILEMGR_MAJOR_VER, FILEMGR_MINOR_VER, FILEMGR_PLATFORM_REV);
                        
   } /* End if CHILDMGR constructed */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
*/
static int32 ProcessCommands(void)
{

   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;
   int32  MsgInt;

   CFE_SB_Buffer_t* SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;
   

   CFE_ES_PerfLogExit(FileMgr.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, FileMgr.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(FileMgr.PerfId);

   if (SysStatus == CFE_SUCCESS) {
      
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
   
      if (SysStatus == OS_SUCCESS) {

         MsgInt = CFE_SB_MsgIdToValue(MsgId);
         
         if (MsgInt == FileMgr.CmdMid) {
            
            CMDMGR_DispatchFunc(CMDMGR_OBJ, SbBufPtr);
         
         } 
         else if (MsgInt == FileMgr.SendHkMid) {
            
            FILESYS_ManageTbl();
            FILEMGR_SendHousekeepingPkt();
         
         }
         else {
            
            CFE_EVS_SendEvent(FILEMGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X", MsgInt);
         }

      }
      else {
         
         CFE_EVS_SendEvent(FILEMGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                           "CFE couldn't retrieve message ID from the message, Status = %d", SysStatus);
      }
      
   } /* Valid SB receive */ 
   else {
   
         CFE_ES_WriteToSysLog("FILEMGR software bus error. Status = 0x%08X\n", SysStatus);   /* Use SysLog, events may not be working */
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   }  
      
   return RetStatus;
   
} /* ProcessCommands() */
