/*
** Purpose: Implement the DIR_Class methods
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU General Public
**   Public License (GPL).
**
** References:
**   1. OpenSatKit Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "initbl.h"
#include "childmgr.h"
#include "dir.h"

/**********************/
/** Global File Data **/
/**********************/

static DIR_Class*  Dir = NULL;


/*
** Local Function Prototypes
*/

static void LoadFileEntry(const char* PathFilename, DIR_FileEntry* FileEntry, uint16* TaskBlockCount, bool IncludeSizeTime);
static bool WriteDirListToFile(const char* DirNameWithSep, osal_id_t DirId, int32 FileHandle, bool IncludeSizeTime);

/******************************************************************************
** Function: DIR_Constructor
**
*/
void DIR_Constructor(DIR_Class*  DirPtr, INITBL_Class* IniTbl)
{
 
   Dir = DirPtr;

   CFE_PSP_MemSet((void*)Dir, 0, sizeof(DIR_Class));
 
   Dir->IniTbl = IniTbl;
 
} /* End DIR_Constructor() */


/******************************************************************************
** Function:  DIR_ResetStatus
**
*/
void DIR_ResetStatus()
{
 
   Dir->CmdWarningCnt = 0;
   
} /* End DIR_ResetStatus() */


/******************************************************************************
** Function: DIR_CreateCmd
**
*/
bool DIR_CreateCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const DIR_CreateCmdMsg* CreateCmd = (const DIR_CreateCmdMsg *) SbBufPtr;
   bool RetStatus = false;
   FileUtil_FileInfo FileInfo;
   int32 SysStatus;
   
   FileInfo = FileUtil_GetFileInfo(CreateCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT) {
      
      SysStatus = OS_mkdir(CreateCmd->DirName, 0);

      if (SysStatus == OS_SUCCESS) {

         RetStatus = true;         
         CFE_EVS_SendEvent(DIR_CREATE_EID, CFE_EVS_EventType_DEBUG, "Created directory %s", CreateCmd->DirName);
         
      }
      else {
         
         CFE_EVS_SendEvent(DIR_CREATE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Create directory %s failed: Parameters validated but OS_mkdir() failed with status=%d",
            CreateCmd->DirName, (int)SysStatus);
      
      }
   }
   else {
      
      CFE_EVS_SendEvent(DIR_CREATE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Create directory command for %s failed due to %s",
         CreateCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   }
   
   return RetStatus;
   
   
} /* End DIR_CreateCmd() */



/******************************************************************************
** Function: DIR_DeleteCmd
**
*/
bool DIR_DeleteCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const DIR_DeleteCmdMsg* DeleteCmd = (const DIR_DeleteCmdMsg *) SbBufPtr;
   

   int32             SysStatus;
   osal_id_t         DirId;
   os_dirent_t       DirEntry;
   FileUtil_FileInfo FileInfo;
   
   bool RetStatus = false;
   bool RemoveDir = true;
   
   FileInfo = FileUtil_GetFileInfo(DeleteCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR) {
      
      /* Open the dir so we can see if it is empty */ 
      
      SysStatus = OS_DirectoryOpen(&DirId, DeleteCmd->DirName);
      
      if (SysStatus == OS_SUCCESS) {
         
         CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Delete directory %s failed: Unable to open dir", DeleteCmd->DirName);

         RemoveDir = false;

      }
      else {
   
        /* Look for a directory entry that is not "." or ".." */
        while (((SysStatus = OS_DirectoryRead(DirId, &DirEntry)) == OS_SUCCESS) && (RemoveDir == true)) {
           
            if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0)) {
                   
               CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Delete directory %s failed: Dir not empty", DeleteCmd->DirName);

                RemoveDir = false;
            }
         }

         OS_DirectoryClose(DirId);
      }

      if (RemoveDir) {
         
         SysStatus = OS_rmdir(DeleteCmd->DirName);

         if (SysStatus == OS_SUCCESS) {
            
            RetStatus = true;
            CFE_EVS_SendEvent(DIR_DELETE_EID, CFE_EVS_EventType_DEBUG, "Deleted directory %s", DeleteCmd->DirName);
            
         }
         else {
            
            CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
               "Delete directory %s failed: Parameters validated but OS_rmdir() failed with status=%d",
               DeleteCmd->DirName, (int)SysStatus);

         }
      }


   } /* End if file is a directory */
   else {
      
      CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Delete directory command for %s failed due to %s",
         DeleteCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
   
   
} /* End DIR_DeleteCmd() */


/******************************************************************************
** Function: DIR_DeleteAllCmd
**
*/
bool DIR_DeleteAllCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const DIR_DeleteAllCmdMsg* DeleteAllCmd = (const DIR_DeleteAllCmdMsg *) SbBufPtr;
   
   FileUtil_FileInfo FileInfo;
   int32             SysStatus;
   osal_id_t         DirId;
   os_dirent_t       DirEntry;
   
   uint32  NameLength = 0;
   uint32  DeleteCount = 0;
   uint32  FileNotDeletedCount = 0;
   uint32  DirSkippedCount = 0;
   char    DirWithSep[OS_MAX_PATH_LEN] = "\0";
   char    Filename[OS_MAX_PATH_LEN] = "\0"; 
   bool    RetStatus = false;


   FileInfo = FileUtil_GetFileInfo(DeleteAllCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR) {
      
      strcpy(DirWithSep, DeleteAllCmd->DirName);
      if (FileUtil_AppendPathSep(DirWithSep, OS_MAX_PATH_LEN)) {
      
         SysStatus = OS_DirectoryOpen(&DirId, DeleteAllCmd->DirName);
         
         if (SysStatus == OS_SUCCESS) {
            
            while ((SysStatus = OS_DirectoryRead(DirId, &DirEntry)) == OS_SUCCESS) {
               
               /* Ignore the "." and ".." directory entries */
               
               if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                   (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0)) {
                      
                  /* Construct full path filename */
                  NameLength = strlen(DirWithSep) + strlen(OS_DIRENTRY_NAME(DirEntry));

                  if (NameLength < OS_MAX_PATH_LEN) {
                     
                     strncpy(Filename, DirWithSep, OS_MAX_PATH_LEN - 1);
                     Filename[OS_MAX_PATH_LEN - 1] = '\0';
                          
                     strncat(Filename, OS_DIRENTRY_NAME(DirEntry), (NameLength - strlen(DirWithSep)));

                     FileInfo = FileUtil_GetFileInfo(Filename, OS_MAX_PATH_LEN, false);

                     switch (FileInfo.State) {
                              
                        case FILEUTIL_FILE_CLOSED:                   
                        
                           if ((SysStatus = OS_remove(Filename)) == OS_SUCCESS) {
                              
                              SysStatus = OS_DirectoryRewind(DirId);  /* Rewind prevents file system from getting confused */

                              ++DeleteCount;
                           }
                           else {
                           
                              ++FileNotDeletedCount;
                           
                           }
                           break;
                        
                        case FILEUTIL_FILE_IS_DIR:
                        
                           ++DirSkippedCount;
                           break;
         
                        case FILEUTIL_FILENAME_INVALID:
                        case FILEUTIL_FILE_NONEXISTENT:
                        case FILEUTIL_FILE_OPEN:
                        default:
                           ++FileNotDeletedCount;
                           break;
                  
                     } /* End FileInfo.State switch */
                  
                  } /* End if valid Filename length */
                  else {

                     ++FileNotDeletedCount;
                  
                  } /* End if invalid Filename length */
               } /* End if "." or ".." directory entries */
            } /* End while read dir */

            OS_DirectoryClose(DirId);
             
            CFE_EVS_SendEvent(DIR_DELETE_ALL_EID, CFE_EVS_EventType_DEBUG,
                  "Deleted %d files in %s", (int)DeleteCount, DeleteAllCmd->DirName);

            if (FileNotDeletedCount > 0) {
               CFE_EVS_SendEvent(DIR_DELETE_ALL_WARN_EID, CFE_EVS_EventType_INFORMATION,
                  "Delete all files in %s warning: %d could not be deleted",
                  DeleteAllCmd->DirName, FileNotDeletedCount);
            }

            if (DirSkippedCount > 0) {
               CFE_EVS_SendEvent(DIR_DELETE_ALL_WARN_EID, CFE_EVS_EventType_INFORMATION,
                  "Delete all files in %s warning: %d directories skipped",
                  DeleteAllCmd->DirName, DirSkippedCount);
            }
         
            if ((FileNotDeletedCount > 0) || (DirSkippedCount > 0)) ++Dir->CmdWarningCnt;
          
         } /* End if open directory */
         else {

            CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
               "Delete all files in %s failed: Call to OS_opendir() failed",
               DeleteAllCmd->DirName);
             
         } /* End if DirId == NULL */
      
      
      } /* End if append path separator */
      else {
      
         CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
            "Delete all files in %s failed: Max path length %d exceeded",
            DeleteAllCmd->DirName, OS_MAX_PATH_LEN);
      
      } /* End if couldn't append path separator */ 
      
   } /* End if file is a directory */ 
   else {
   
      CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
         "Delete all files in %s failed due to %s",
         DeleteAllCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
   
   } /* End if file is not a directory */
   
   return RetStatus;

} /* End of DIR_DeleteAllCmd() */


/******************************************************************************
** Function: DIR_SendListPktCmd
**
** Notes:
**   1. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
** 
*/
bool DIR_SendListPktCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const DIR_SendListPktCmdMsg* SendListPktCmd = (const DIR_SendListPktCmdMsg *) SbBufPtr;
   
   int32             SysStatus;
   osal_id_t         DirId;
   os_dirent_t       DirEntry;
   FileUtil_FileInfo FileInfo;
   
   uint16 TaskBlockCnt = 0;    /* See prologue */
   uint16 FilenameLen;
   uint16 DirWithSepLen;
   char   DirWithSep[OS_MAX_PATH_LEN] = "\0";
   char   PathFilename[OS_MAX_PATH_LEN] = "\0";
   
   bool RetStatus = false;
   bool CreatingPkt;
   
   FileInfo = FileUtil_GetFileInfo(SendListPktCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR) {
      
      /* Clears counters and nulls strings */
      CFE_MSG_Init(&Dir->ListPkt.TlmHeader.Msg, (CFE_SB_MsgId_t)INITBL_GetIntConfig(Dir->IniTbl,CFG_DIR_LIST_TLM_MID), sizeof(DIR_ListPkt));
      
      strcpy(DirWithSep, SendListPktCmd->DirName);
      if (FileUtil_AppendPathSep(DirWithSep, OS_MAX_PATH_LEN)) {
      
         DirWithSepLen = strlen(DirWithSep);
         
         SysStatus = OS_DirectoryOpen(&DirId, SendListPktCmd->DirName);
         if (SysStatus == OS_SUCCESS) {
            
            strncpy(Dir->ListPkt.DirName, SendListPktCmd->DirName, OS_MAX_PATH_LEN);
            Dir->ListPkt.DirListOffset = SendListPktCmd->DirListOffset;

            CreatingPkt = true;
            while (CreatingPkt) {
        
               SysStatus = OS_DirectoryRead(DirId, &DirEntry);
               if (SysStatus != OS_SUCCESS) {
            
                  CreatingPkt = false;
                  
               }
               else if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                        (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0)){
            
                  /* 
                  ** General logic 
                  ** - Do not count the "." and ".." directory entries 
                  ** - Start packet listing at command-specified offset
                  ** - Stop when telemetry packet is full
                  */      
                  ++Dir->ListPkt.DirFileCnt;

                  if ((Dir->ListPkt.DirFileCnt > Dir->ListPkt.DirListOffset) &&
                      (Dir->ListPkt.PktFileCnt < FILEMGR_DIR_LIST_PKT_ENTRIES)) {
                
                     DIR_FileEntry* PktFileEntry = &Dir->ListPkt.File[Dir->ListPkt.PktFileCnt];

                     FilenameLen = strlen(OS_DIRENTRY_NAME(DirEntry));

                     /* Verify combined directory plus filename length */
                     if ((FilenameLen < sizeof(PktFileEntry->Name)) &&
                        ((DirWithSepLen + FilenameLen) < OS_MAX_PATH_LEN)) {

                        strcpy(PktFileEntry->Name, OS_DIRENTRY_NAME(DirEntry));

                        strcpy(PathFilename, DirWithSep);
                        strcat(PathFilename, OS_DIRENTRY_NAME(DirEntry));

                        LoadFileEntry(PathFilename, PktFileEntry, &TaskBlockCnt, SendListPktCmd->IncludeSizeTime);

                        ++Dir->ListPkt.PktFileCnt;
                        
                     }
                     else {
                        
                        Dir->CmdWarningCnt++;

                        CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_WARN_EID, CFE_EVS_EventType_INFORMATION,
                                          "Send dir list path/file len too long: Dir %s, File %s",
                                          DirWithSep, OS_DIRENTRY_NAME(DirEntry));
                     }
                     
                  } /* End if in range to fill packet */         
               } /* End if not current or parent directory */
            
            } /* End while creating packet */

            OS_DirectoryClose(DirId);

            CFE_SB_TimeStampMsg(&(Dir->ListPkt.TlmHeader.Msg));
            CFE_SB_TransmitMsg(&(Dir->ListPkt.TlmHeader.Msg), true);

            CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_EID, CFE_EVS_EventType_DEBUG,
                              "Send dir list pkt cmd complete: offset = %d, dir = %s",
                              (int)Dir->ListPkt.DirListOffset, Dir->ListPkt.DirName);
            
            RetStatus = true;
            
         } /* DirPtr != NULL */
         else {
            
            CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Send dir list pkt cmd failed: OS_opendir failed for %s",
                              SendListPktCmd->DirName);

         } /* DirPtr == NULL */
      
      } /* DirWithSep length okay */
      else {
         
         CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Send dir list pkt cmd failed: %s with path separator is too long",
                           SendListPktCmd->DirName);

         
      } /* DirWithSep length too long */
   } /* End if file is a directory */
   else {
      
      CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Send dir list pkt cmd failed: %s is not a directory. It's state is %s",
                        SendListPktCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
      
} /* End DIR_SendListPktCmd() */


/******************************************************************************
** Function: DIR_WriteListFileCmd
**
** Notes:
**   1. Target file will be overwritten if it exists and is closed.
** 
*/
bool DIR_WriteListFileCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const DIR_WriteListFileCmdMsg* WriteListFileCmd = (const DIR_WriteListFileCmdMsg *) SbBufPtr;
   bool RetStatus = false;
      
   osal_id_t    FileHandle;
   int32        SysStatus;
   osal_id_t    DirId;
   char DirNameWithSep[OS_MAX_PATH_LEN] = "\0";
   char Filename[OS_MAX_PATH_LEN] = "\0";
   FileUtil_FileInfo FileInfo;

   
   FileInfo = FileUtil_GetFileInfo(WriteListFileCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR) {
      
      if (WriteListFileCmd->Filename[0] == '\0') {
         strncpy(Filename, INITBL_GetStrConfig(Dir->IniTbl,CFG_DIR_LIST_FILE_DEFNAME), OS_MAX_PATH_LEN - 1);
         Filename[OS_MAX_PATH_LEN - 1] = '\0';
      }
      else {
         CFE_PSP_MemCpy(Filename, WriteListFileCmd->Filename, OS_MAX_PATH_LEN);
      }
      
      FileInfo = FileUtil_GetFileInfo(Filename, OS_MAX_PATH_LEN, false);

      if ((FileInfo.State == FILEUTIL_FILE_CLOSED) ||
          (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)) {
              
         strcpy(DirNameWithSep, WriteListFileCmd->DirName);
         if (FileUtil_AppendPathSep(DirNameWithSep, OS_MAX_PATH_LEN)) {
      
            SysStatus = OS_DirectoryOpen(&DirId, WriteListFileCmd->DirName);
      
            if (SysStatus == OS_SUCCESS) {
               
               SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
                   
               if (SysStatus == OS_SUCCESS) {
                  
                  RetStatus = WriteDirListToFile(DirNameWithSep, DirId, FileHandle, WriteListFileCmd->IncludeSizeTime);
                 
                  OS_close(FileHandle);
                  
                  CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_EID, CFE_EVS_EventType_DEBUG, 
                                    "Directory %s written to %s with status %d", 
                                    WriteListFileCmd->DirName, WriteListFileCmd->Filename, RetStatus);
                  
               }
               else {
                  
                  CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                    "Send dir list pkt cmd failed: Error creating file %s, Status=%d",
                                    Filename, SysStatus);
           
               } /* End if error opening output file */
            
               OS_DirectoryClose(DirId);
            
            } /* Open dir */
            else {
               
               CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Send dir list pkt cmd failed: OS_opendir failed for %s",
                                 WriteListFileCmd->DirName);
            
            } /* DirPtr == NULL */
      
      
         } /* DirWithSep length okay */
         else {
         
            CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Send dir list pkt cmd failed: %s with path separator is too long",
                              WriteListFileCmd->DirName);

         
         } /* DirWithSep length too long */
      
      } /* Target file okay to write */
      else {
         
         CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Write dir list file cmd failed: %s must be closed or nonexistent. File state is %s",
                           Filename, FileUtil_FileStateStr(FileInfo.State));
         
      } /* Target file not okay to write */
   
   } /* End if file is a directory */
   else {
      
      CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Write dir list file cmd failed: %s is not a directory. It's state is %s",
                        WriteListFileCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
      
} /* End DIR_WriteListFileCmd() */


/******************************************************************************
** Function: WriteDirListToFile
**
** Notes:
**   1. The command handler took care of validating inputs and opening the
**      input directory and output file. This function is responsible for 
**      writing all of the content to the output file including the header.
**   2. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
**
*/
static bool WriteDirListToFile(const char* DirNameWithSep, osal_id_t DirId, int32 FileHandle, bool IncludeSizeTime)
{
   
   bool ReadingDir   = true;
   bool FileWriteErr = false;
   bool CreatedFile  = false;
     
   uint16 DirWithSepLen = strlen(DirNameWithSep);
   uint16 DirEntryLen   = 0;               /* Length of each directory entry */
   uint16 DirEntryCnt   = 0;
   uint16 FileEntryCnt  = 0;
   uint16 FileEntryMax  = INITBL_GetIntConfig(Dir->IniTbl, CFG_DIR_LIST_FILE_ENTRIES);
   uint16 TaskBlockCnt  = 0;               /* See prologue */
   int32  BytesWritten;
   char   PathFilename[OS_MAX_PATH_LEN] = "\0";   /* Combined directory path and entry filename */
 
   int32            SysStatus;  
   CFE_FS_Header_t  FileHeader;
   os_dirent_t      DirEntry;
   uint16           DirFileStatsLen = sizeof(DIR_ListFilesStats);
   uint16           DirFileEntryLen = sizeof(DIR_FileEntry);
   DIR_FileEntry    DirFileEntry;
   

   /*
   ** Create and write standard cFE file header
   */
   
   CFE_PSP_MemSet(&FileHeader, 0, sizeof(CFE_FS_Header_t));
   FileHeader.SubType = INITBL_GetIntConfig(Dir->IniTbl, CFG_DIR_LIST_FILE_SUBTYPE);
   strncpy(FileHeader.Description, "Directory Listing", sizeof(FileHeader.Description) - 1);
   FileHeader.Description[sizeof(FileHeader.Description) - 1] = '\0';

   BytesWritten = CFE_FS_WriteHeader(FileHandle, &FileHeader);
   if (BytesWritten == sizeof(CFE_FS_Header_t)) {
      
      /* 
      ** Create initial stats structure and write it to the file. After the
      ** directory is written to the file, the stats fields will be updated.
      */

      CFE_PSP_MemSet(&Dir->ListFileStats, 0, DirFileStatsLen);
      strncpy(Dir->ListFileStats.DirName, DirNameWithSep, OS_MAX_PATH_LEN - 1);
	   Dir->ListFileStats.DirName[OS_MAX_PATH_LEN - 1] = '\0';
      
      BytesWritten = OS_write(FileHandle, &Dir->ListFileStats, DirFileStatsLen);      
      if (BytesWritten == DirFileStatsLen) {
      
         while (ReadingDir && !FileWriteErr) {
        
            SysStatus = OS_DirectoryRead(DirId, &DirEntry);
            
            if (SysStatus != OS_SUCCESS) {
                  
               ReadingDir = false;  /* Normal loop end - no more directory entries */
               
            }
            else if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                     (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR) != 0)) {
            
               /* 
               ** General logic 
               ** - Do not count the "." and ".." directory entries 
               ** - Count all files, but limit file to FILEMGR_INI_DIR_LIST_FILE_ENTRIES
               */      
                  
               ++DirEntryCnt;

               if (FileEntryCnt < FileEntryMax) {
                     
                  DirEntryLen = strlen(OS_DIRENTRY_NAME(DirEntry));

                  /* Verify combined directory plus filename length */
                  if ((DirEntryLen < sizeof(DirFileEntry.Name)) &&
                     ((DirWithSepLen + DirEntryLen) < OS_MAX_PATH_LEN)) {

                     strncpy(PathFilename, DirNameWithSep, DirWithSepLen);
	        	         PathFilename[DirWithSepLen] = '\0';
                     strncat(PathFilename, OS_DIRENTRY_NAME(DirEntry), (OS_MAX_PATH_LEN - DirWithSepLen));

                     /* Populate directory list file entry */
                     strncpy(DirFileEntry.Name, OS_DIRENTRY_NAME(DirEntry), DirEntryLen);
		               DirFileEntry.Name[DirEntryLen] = '\0';
          
                     LoadFileEntry(PathFilename, &DirFileEntry, &TaskBlockCnt, IncludeSizeTime);
          
                     BytesWritten = OS_write(FileHandle, &DirFileEntry, DirFileEntryLen);
                     if (BytesWritten == DirFileEntryLen) {
                
                        ++FileEntryCnt;
                     
                     } /* End if sucessful file write */
                     else {
                        
                        FileWriteErr = true;
                     
                        CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                          "Write dir list file cmd failed: OS_write entry failed: result = %d, expected = %d",
                                          (int)BytesWritten, (int)DirFileEntryLen);
                       
                    } /* End if file write error */
         
                  } /* End if file name lengths valid */
                  else {
                     
                     ++Dir->CmdWarningCnt;
                        
                     CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_WARN_EID, CFE_EVS_EventType_ERROR,
                                       "Write dir list file cmd warning: Combined dir/entry name too long: dir = %s, entry = %s",
                                       DirNameWithSep, OS_DIRENTRY_NAME(DirEntry));
                     
                  } /* End if invalid file entry name lengths */ 
         

               } /* End if within file entry write limit */
            } /* End if not current/parent directory */ 
         } /* End Reading Dir & Writing file loop */

         /*
         ** Update directory statistics in output file
         ** - Update local stats data structure
         ** - Back up file pointer to the start of the statisitics data
         ** - Write local stats data to the file
         */
   
         if ( (DirEntryCnt > 0) && !FileWriteErr) {

            Dir->ListFileStats.DirFileCnt      = DirEntryCnt;
            Dir->ListFileStats.FilesWrittenCnt = FileEntryCnt;

            OS_lseek(FileHandle, sizeof(CFE_FS_Header_t), OS_SEEK_SET);
        
            BytesWritten = OS_write(FileHandle, &Dir->ListFileStats, DirFileStatsLen);

            if (BytesWritten == DirFileStatsLen) {
            
               CreatedFile = true;
            
            } /* End if no stats write error */
            else {

               CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                "Write dir list file cmd failed: OS_write stats failed: result = %d, expected = %d",
                                (int)BytesWritten, (int)DirFileStatsLen);

            } /* End if stats write error */
 
         } /* End if sucessful dir read & file write loop */
               
      } /* End if no initial stats write error */
      else {

         CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Write dir list file cmd failed: OS_write stats failed: result = %d, expected = %d",
                          (int)BytesWritten, (int)DirFileStatsLen);
      
      } /* End if initial stats write error */         
   } /* End if cFE file header write error */      
   else {

      CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                       "Write dir list file cmd failed: OS_write cFE header failed: result = %d, expected = %d",
                       (int)BytesWritten, (int)sizeof(CFE_FS_Header_t));
      
   } /* End if cFE header write error */         

     
   return  CreatedFile;

   
} /* End WriteDirListToFile() */


/******************************************************************************
** Function: LoadFileEntry
**
** Notes:
**   1. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
** 
*/
static void LoadFileEntry(const char* PathFilename, DIR_FileEntry* FileEntry, uint16* TaskBlockCount, bool    IncludeSizeTime)
{
   
   int32       SysStatus; 
   os_fstat_t  FileStatus;
   
   
   if (IncludeSizeTime) {
      
      CHILDMGR_PauseTask(TaskBlockCount, INITBL_GetIntConfig(Dir->IniTbl, CFG_TASK_FILE_STAT_CNT), 
                         INITBL_GetIntConfig(Dir->IniTbl, CFG_TASK_FILE_STAT_DELAY), 
                         INITBL_GetIntConfig(Dir->IniTbl, CFG_CHILD_TASK_PERF_ID));
      
      CFE_PSP_MemSet(&FileStatus, 0, sizeof(os_fstat_t));
      SysStatus = OS_stat(PathFilename, &FileStatus);
      
      if (SysStatus == OS_SUCCESS) {
         
         FileEntry->Size = FileStatus.FileSize;
         FileEntry->Mode = FileStatus.FileModeBits;
         FileEntry->Time = OS_GetLocalTime(&FileStatus.FileTime);
        
      }
      else {
         
         FileEntry->Size = 0;
         FileEntry->Time = 0;
         FileEntry->Mode = 0;
      
      }
      
   } /* End if include Size & Time */
   else {
         
      FileEntry->Size = 0;
      FileEntry->Time = 0;
      FileEntry->Mode = 0;
      
   } /* End if don't include Size & Time */
      
      
} /* End LoadFileEntry() */

