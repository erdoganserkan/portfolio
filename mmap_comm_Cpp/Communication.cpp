#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/file.h>

#include "Communication.h"

Communication::Communication(unsigned char Type)
{
	ObjectType = Type;
	InitLogger();
	InitSharedFiles();
	InitSemaphore();
}
Communication::~Communication()
{
	this->CommLogger->Log(INFO,"~Communication():: Communication is being Destroyed \n");
	this->ReleaseSharedFiles();
	this->ReleaseSemaphore();
	this->ReleaseLogger();
}


bool Communication::SendCommand(const u_int16_t	CommData,const u_int16_t dataWORD)
{
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		// ---> Send data to child
		// wait until child finishes the previous command processing
		int i=0;
		for(i=0;i<30;++i)
		{
			LockOutput();
			if(OUTFileMap->Flag == OLD_COMM)
				break;
			UnlockOutput();
			usleep(100000);
		}
		if(i<30)
		{	// Old Command Processed & Child is Ready For New One
			CommLogger->Log(INFO,"SendCommand()::child is READY for new command \n");
		}
		else
		{	// Old Command Not Processed & Child is NOT Ready For New One
			CommLogger->Log(ERR,"SendCommand():: child is NOT READY for new command \n");
			returnValue = false;
			return returnValue;
		}
		OUTFileMap->Flag = OLD_COMM;
		OUTFileMap->CommData = CommData;
		OUTFileMap->AppPID=getpid();
		OUTFileMap->dataDWORD = dataWORD;
		OUTFileMap->Flag = NEW_COMM;
		UnlockOutput();
		if(VerifyCommand(CommData,dataWORD) == false)
		{
			CommLogger->Log(ERR,"SendCommand():: VerifyCommand() FAILED \n");
			returnValue = false;
		}
	}
	else if(ObjectType == CHILD)
	{
		CommLogger->Log(ERR,"SendCommand():: Child should not call this function  \n");
	}
	else
	{
		CommLogger->Log(ERR,"SendCommand()::Unknown Object Type \n");
	}
	return returnValue;
}
bool Communication::VerifyCommand(const u_int16_t	CommData,const u_int16_t dataWORD)
{
	// Wait until Flag Section is set to OLD_COMM;
	// This indicates child read the command but not performed it yet
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		// 1---> First wait for changing the NEM_COMM to OLD_COMM &&& this indicates that child received the command
		int i = 0;
		for(i=0;i<30;i++)	// wait for 3 seconds maximum
		{
			LockOutput();
			if(OUTFileMap->Flag == OLD_COMM)
				break;
			UnlockOutput();
			usleep(100000);
		}
		UnlockOutput();
		if(i<30)
		{
			CommLogger->Log(DEBUG,"VerifyCommand():: Child Command Receiving SUCCESS \n");
		}
		else
		{
			CommLogger->Log(ERR,"VerifyCommand():: Child Command Receiving FAILED \n");
			returnValue = false;
		}
#ifdef SIMPLE_VERIFY
		return returnValue;
#else
		// 2---> Waits for Child sent the COMM_DONE Data &&& this indicates that child performed the command
		u_int16_t  TempCommData = 0;
		u_int16_t  TempDataWORD = 0;
		while(IsDataReady() == false);
		if(ReceiveData(TempCommData,TempDataWORD) == false)
		{
			CommLogger->Log(ERR,"VerifyCommand():: ReceiveData()  FAILED \n");
			returnValue = false;
		}
		if(TempCommData != COMM_DONE)
		{
			CommLogger->Log(ERR,"VerifyCommand():: WRONG CommData received from Child \n");
			returnValue = false;
		}
		else
		{
			CommLogger->Log(DEBUG,"VerifyCommand():: RIGHT CommData received from Child \n");
		}
		if(TempDataWORD != CommData)
		{
			CommLogger->Log(ERR,"VerifyCommand():: WRONG dataWORD (Processed Command) received from Child \n");
			returnValue = false;
		}
		else
		{
			CommLogger->Log(DEBUG,"VerifyCommand():: RIGHT dataWORD (Processed Command) received from Child \n");
		}
#endif
	}
	else if(ObjectType == CHILD)
	{
		CommLogger->Log(ERR,"VerifyCommand():: Child should not call this function  \n");
	}
	else
	{
		CommLogger->Log(ERR,"VerifyCommand()::Unknown Object Type \n");
	}
	return returnValue;
}
bool Communication::ReceiveData(u_int16_t	&CommData,u_int16_t &dataWORD)
{
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		// ---> Parent receives the data
		LockInput();
		CommData = INPFileMap->CommData;
		dataWORD = INPFileMap->dataWORD;
		INPFileMap->Flag = OLD_DATA;			// This indicates parent has received the data
		UnlockInput();
		CommLogger->Log(DEBUG,"ReceiveData():: Data Read SUCCESS \n");
	}
	else if(ObjectType == CHILD)
	{
		CommLogger->Log(ERR,"ReceiveData():: Child should not call this function  \n");
	}
	else
	{
		CommLogger->Log(ERR,"ReceiveData()::Unknown Object Type \n");
	}
	return returnValue;
}



bool Communication::SendData(const u_int16_t	CommData, const u_int16_t dataWORD)
{
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		CommLogger->Log(ERR,"SendData():: Parent should not call this function  \n");
	}
	else if(ObjectType == CHILD)
	{
		// ---> Child Sends data to parent
		// ---> Be sure parent is ready for new data
		int i=0;
		for(i=0;i<30;++i)
		{
			LockOutput();
			if(OUTFileMap->Flag == OLD_DATA)
				break;
			UnlockOutput();
			usleep(100000);
		}
		if(i<30)
		{	// Old Command Processed & Child is Ready For New One
			CommLogger->Log(INFO,"SendCommand()::parent is READY for new data \n");
		}
		else
		{	// Old Command Not Processed & Child is NOT Ready For New One
			CommLogger->Log(ERR,"SendCommand():: parent is NOT READY for new data \n");
			returnValue = false;
			return returnValue;
		}
		OUTFileMap->Flag = OLD_DATA;
		OUTFileMap->CommData = CommData;
		OUTFileMap->dataWORD = dataWORD;
		OUTFileMap->AppPID=getpid();
		OUTFileMap->Flag = NEW_DATA;
		UnlockOutput();
		if(VerifiyData(CommData,dataWORD) == false)
		{
			CommLogger->Log(ERR,"SendData()::VerifyData() FAILED \n");
			returnValue = false;
		}
	}
	else
	{
		CommLogger->Log(ERR,"SendData()::Unknown Object Type \n");
	}
	return returnValue;
}
bool Communication::VerifiyData(const u_int16_t	CommData,const u_int16_t dataWORD)
{
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		CommLogger->Log(ERR,"VerifiyData():: Parent should not call this function  \n");
	}
	else if(ObjectType == CHILD)
	{
		// 1---> First wait for changing the NEM_COMM to OLD_COMM &&& this indicates that Parent received the command
		int i = 0;
		for(i=0;i<30;i++)	// wait for 3 seconds maximum
		{
			LockOutput();
			if(OUTFileMap->Flag == OLD_DATA)
				break;
			UnlockOutput();
			usleep(100000);
		}
		UnlockOutput();
		if(i<30)
		{
			CommLogger->Log(DEBUG,"VerifiyData():: Parent Data Receiving SUCCESS\n");
		}
		else
		{
			CommLogger->Log(ERR,"VerifiyData():: Parent Data Receiving FAILED \n");
			returnValue = false;
		}
#ifdef SIMPLE_VERIFY
		return returnValue;
#else
		// 2---> Waits for Child sent the COMM_DONE Data &&& this indicates that child performed the command
		u_int16_t  TempCommData = 0;
		u_int16_t  TempDataWORD = 0;
		while(IsCommandReady() == false);
		if(ReceiveCommand(TempCommData,TempDataWORD) == false)
		{
			CommLogger->Log(ERR,"VerifiyData():: data receiving from Parent FAILED \n");
			returnValue = false;
		}
		if(TempCommData != DATA_DONE)
		{
			CommLogger->Log(ERR,"VerifiyData():: WRONG CommData received from Parent \n");
			returnValue = false;
		}
		else
		{
			CommLogger->Log(DEBUG,"VerifiyData():: RIGHT CommData received from Parent \n");
		}
		if(TempDataWORD != CommData)
		{
			CommLogger->Log(ERR,"VerifiyData():: WRONG dataWORD (Processed Data) received from Parent \n");
			returnValue = false;
		}
		else
		{
			CommLogger->Log(DEBUG,"VerifiyData():: RIGHT dataWORD (Processed Data) received from Parent \n");
		}
#endif
	}
	else
	{
		CommLogger->Log(ERR,"VerifyData()::Unknown Object Type \n");
	}
	return returnValue;
}
bool Communication::ReceiveCommand(u_int16_t &CommData,u_int16_t &dataWORD)
{
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		CommLogger->Log(ERR,"ReceiveCommand():: Parent should not call this function  \n");
	}
	else if(ObjectType == CHILD)
	{
		// ---> Parent receives the data
		LockInput();
		CommData = INPFileMap->CommData;
		dataWORD = INPFileMap->dataWORD;
		INPFileMap->Flag = OLD_COMM;			// This indicates parent has received the data
		UnlockInput();
		CommLogger->Log(DEBUG,"ReceiveCommand():: Command  Read SUCCESS \n");
	}
	else
	{
		CommLogger->Log(ERR,"ReceiveCommand()::Unknown Object Type \n");
	}
	return returnValue;
}

bool Communication::IsCommandReady(void)
{
	bool returnValue = false;
	LockInput();
	if(INPFileMap->Flag == NEW_COMM)
		returnValue = true;
	else
		returnValue = false;
	UnlockInput();
	return returnValue;
}
bool Communication::IsDataReady(void)
{
	bool returnValue = false;
	LockInput();
	if(INPFileMap->Flag == NEW_DATA)
		returnValue = true;
	else
		returnValue = false;
	UnlockInput();
	return returnValue;
}

bool Communication::InitSharedFiles(void)
{
	int inpFD,outFD;
	bool returnValue = true;
	if(ObjectType == PARENT)
	{
		inpFD = open (PINPUT,O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if(inpFD == -1)
		{
			CommLogger->Log(ERR,"InitSharedFiles():: %s File Open FAILED (%s)\n",PINPUT,strerror(errno));
			returnValue = false;
		}
		outFD = open (POUTPUT, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if(outFD == -1)
		{
			CommLogger->Log(ERR,"InitSharedFiles():: %s File Open FAILED (%s)\n",POUTPUT,strerror(errno));
			returnValue = false;
		}
	}
	else if(ObjectType == CHILD)
	{
		inpFD = open (CINPUT,O_RDWR, S_IRUSR | S_IWUSR);
		if(inpFD == -1)
		{
			CommLogger->Log(ERR,"InitSharedFiles():: %s File Open FAILED (%s)\n",CINPUT,strerror(errno));
			returnValue = false;
		}
		outFD = open (COUTPUT, O_RDWR, S_IRUSR | S_IWUSR);
		if(outFD == -1)
		{
			CommLogger->Log(ERR,"InitSharedFiles():: %s File Open FAILED (%s)\n",COUTPUT,strerror(errno));
			returnValue = false;
		}
	}
	else
	{
		CommLogger->Log(ERR,"InitSharedFiles():: Unknown ObjectType !!!\n");
	}

// Truncate files to _FILE_SIZE_ & file must be opened for write also to be success
	if(ftruncate (inpFD, _FILE_SIZE_) == -1)
	{
		CommLogger->Log(ERR,"InitSharedFiles():: truncate on input file FAILED (%s)\n",strerror(errno));
		returnValue = false;
	}
	if(ftruncate (outFD, _FILE_SIZE_) == -1)
	{
		CommLogger->Log(ERR,"InitSharedFiles():: truncate on output file FAILED (%s)\n",strerror(errno));
		returnValue = false;
	}
// mmap input and output files
	INPFileMap = (FileMap *)mmap (0, _MMAP_SIZE_, PROT_READ | PROT_WRITE, MAP_SHARED, inpFD, 0);
	if(INPFileMap == NULL)
	{
		CommLogger->Log(ERR,"InitSharedFiles():: mmap for Input file FAILED (%s)\n",strerror(errno));
		returnValue = false;
	}
	else
	{
		CommLogger->Log(DEBUG,"InitSharedFiles():: mmap adress of INPUT file  = %p \n",INPFileMap);
	}
	OUTFileMap = (FileMap *)mmap (0, _MMAP_SIZE_,PROT_READ | PROT_WRITE, MAP_SHARED, outFD, 0);
	if(OUTFileMap == NULL)
	{
		CommLogger->Log(ERR,"InitSharedFiles():: mmap for Output file FAILED (%s)\n",strerror(errno));
		returnValue = false;
	}
	else
	{
		CommLogger->Log(DEBUG,"InitSharedFiles():: mmap adress of OUTPUT file  = %p \n",OUTFileMap);
	}
//	PARENT Set shared files to initial state
	if(ObjectType == PARENT)
	{
		INPFileMap->Flag = OLD_DATA;
		OUTFileMap->Flag = OLD_COMM;
	}
//
// close unnecessary file descriptors, mmap return address will be used for file operations now from on
	if(inpFD != -1)
		close(inpFD);
	if(outFD != -1)
		close(outFD);

	return returnValue;
}
void Communication::ReleaseSharedFiles(void)
{
	if(ObjectType == PARENT)
	{
		//munmap Communication files
		if(INPFileMap != NULL)
			munmap(INPFileMap,_MMAP_SIZE_);
		if(OUTFileMap != NULL)
			munmap(OUTFileMap,_MMAP_SIZE_);
	// Removing created Communication Files
		 if(remove(PINPUT) != 0)
		 {
			 CommLogger->Log(ERR,"ReleaseSharedFiles():: %s remove FAILED (%s) \n",PINPUT,strerror(errno));
		 }
		 if(remove(POUTPUT) != 0)
		 {
			 CommLogger->Log(ERR,"ReleaseSharedFiles():: %s remove FAILED (%s) \n",POUTPUT,strerror(errno));
		 }

	}
}

bool Communication::InitSemaphore(void)
{
	bool returnValue = true;

	if(ObjectType == PARENT)
	{
		// Initialize Output Semaphore
		// last parameter is only used when semaphore created... It indicates semaphore count...
		OutputSem = sem_open(SEM_PTOC, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
		if (OutputSem != SEM_FAILED)
		{
			int val;
			sem_getvalue(OutputSem, &val);
			CommLogger->Log(DEBUG,"InitSemaphore():: creation of %s semaphore SUCCESS with %d value \n",SEM_PTOC,val);
		}
		else
		{
			CommLogger->Log(ERR,"InitSemaphore():: %s creation of new semaphore FAILED (%s) \n",SEM_PTOC,strerror(errno));
			 if (errno != EEXIST)
				 returnValue = false;
		}
		// Initialize Input Semaphore
		InputSem = sem_open(SEM_CTOP, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
		if (InputSem != SEM_FAILED)
		{
			int val;
			sem_getvalue(InputSem, &val);
			CommLogger->Log(DEBUG,"InitSemaphore():: creation of %s semaphore SUCCESS with %d value \n",SEM_CTOP,val);
		}
		else
		{
			CommLogger->Log(ERR,"InitSemaphore():: %s creation of new semaphore FAILED (%s)\n",SEM_CTOP,strerror(errno));
			 if (errno != EEXIST)
				 returnValue = false;
		}
	}
	else if(ObjectType == CHILD)
	{
		// Initialize Output Semaphore
		// last parameter is only used when semaphore created... It indicates semaphore count...
		OutputSem = sem_open(SEM_CTOP, 0);
		if (OutputSem != SEM_FAILED)
		{
			CommLogger->Log(DEBUG,"InitSemaphore():: %s opening of new semaphore SUCCESS \n",SEM_CTOP);
		}
		else
		{
			CommLogger->Log(ERR,"InitSemaphore():: %s opening of new semaphore FAILED (%s)\n",SEM_CTOP,strerror(errno));
			returnValue = false;
		}
		// Initialize Input Semaphore
		InputSem = sem_open(SEM_PTOC, 0);
		if (InputSem != SEM_FAILED)
		{
			CommLogger->Log(DEBUG,"InitSemaphore():: %s creation of new semaphore SUCCESS \n",SEM_PTOC);
		}
		else
		{
			CommLogger->Log(ERR,"InitSemaphore():: %s creation of new semaphore FAILED (%s)\n",SEM_PTOC,strerror(errno));
			returnValue = false;
		}
	}
	else
	{
		CommLogger->Log(ERR,"InitSemaphore():: Unknown ObjectType !!!\n");
	}
	return returnValue;
}
void Communication::ReleaseSemaphore(void)
{
	if(ObjectType == CHILD)
	{

		sem_close(OutputSem);			// This function does not removes file system entry of semophore under /dev/shm
		sem_close(InputSem);
	}
	else if (ObjectType == PARENT)
	{
		sem_unlink(SEM_PTOC);			// This function also removes file system entry of semophore under /dev/shm
		sem_unlink(SEM_CTOP);
	}
}


bool Communication::LockInput(void)
{
	bool returnValue = true;
	// decrement ınputSEM before reading from input
	sem_wait(InputSem);
	return returnValue;
}
bool Communication::LockOutput(void)
{
	bool returnValue = true;
	// decrement InputSEM before writing to output
	sem_wait(OutputSem);
	return returnValue;
}


bool Communication::UnlockInput(void)
{
	bool returnValue = true;
	// increment OutputSEM after reading from input
	sem_post(InputSem);
	return returnValue;
}
bool Communication::UnlockOutput(void)
{
	bool returnValue = true;
	// increment OutputSEM after writing to output
	sem_post(OutputSem);
	return returnValue;
}



bool Communication::InitLogger(void)
{
	bool returnValue = true;
	if(ObjectType == CHILD)
	{
		CommLogger = new logger(SHMCHILD_LOG_DEVICE);
		CommLogger->Log(DEBUG,"InitLogger():: ObjectType = %s \n",(ObjectType == PARENT)?"PARENT":"CHILD");
	}
	else if(ObjectType == PARENT)
	{
		CommLogger = new logger(SHMPARENT_LOG_DEVICE);
		CommLogger->Log(DEBUG,"InitLogger():: ObjectType = %s \n",(ObjectType == PARENT)?"PARENT":"CHILD");
	}
	else
	{
		CommLogger->Log(ERR,"InitLogger():: Unknown ObjectType !!!\n");
	}
	return returnValue;
}
void Communication::ReleaseLogger(void)
{
	CommLogger->Log(INFO,"ReleaseLogger():: CommLogger will be closed \n");
	delete CommLogger;
}
