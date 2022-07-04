#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <semaphore.h>

#define 		SHMPARENT_LOG_DEVICE			"/tmp/CommunicationPLog.txt"
#define 		SHMCHILD_LOG_DEVICE				"/tmp/CommunicationCLog.txt"

#ifndef ADVANCED_VARIFY
#define SIMPLE_VERIFY
#endif
// Create semaphores under "/" directory
#define 	SEM_PTOC		"/PTOCSHM"
#define 	SEM_CTOP		"/CTOPSHM"

//########## COMMUNICATION FILES #########
// Communication files will be created in temporary filesystem
#define 		PINPUT				"/tmp/CTOP"
#define   		POUTPUT				"/tmp/PTOC"
#define	   		CINPUT				POUTPUT
#define   		COUTPUT				PINPUT

// File mapping size as BYTES
#define   		_MMAP_SIZE_			100
// Size of each communication file
#define  		_FILE_SIZE_			200
// The type of Communication ObjectType
#define 		PARENT					0x67
#define 		CHILD						0x78

//############ COMMAND SECTION ############ PARENT TO CHILD
#define		STOP				0x3423
#define		CANCEL			0x3424
#define		PAUSE				0x3425
#define		CONTINUE		0x3426

#define		PLAY_1X2			0X3427
#define		PLAY_1X4			0x3428
#define		PLAY_NORM		0x3429

#define 	ISAPPOK					0x3431
#define		DATA_DONE		 		0x3434
//############## FLAG SECTION #############  BIDIRECTIONAL
// There is a new command waiting for child to process it
#define		NEW_COMM		0x75
// There is no new command, all previous operations done
#define 	OLD_COMM		0x76
// There is new data in file for Parent to process it
#define 	NEW_DATA		0x77
// There is no new data for parent to process it
#define 	OLD_DATA		0x78

//######## DATA SECTION ###################  CHILD TO PARENT
#define 		APPISOK						0x7529
#define 		APPISERR						0x7532
#define 		COMM_DONE				0x7534

//######## DATA OFFSETS OF SECTIONS ###########
// Address of flag section indicates new command/data is ready for processing
#define 		FLAG_OFFSET						0x0
// Addess of the new command/data waiting for processing
#define 		COM_DATA_OFFSETT			0x1
// Address of PID of sender process
#define 		PID_OFFSET							0x3
// Address of optional data section for supşementary for Command/Data
#define 		OPT_DATA_OFFSET				0x5


//########### Mapping File Layout ##########
struct FileMap
{
	u_int8_t 			Flag;
	u_int16_t			CommData;
	pid_t				AppPID;
	union
	{
		u_int32_t		dataDWORD;
		u_int16_t 		dataWORD;
		u_int8_t		dataArray[_MMAP_SIZE_-sizeof(u_int8_t)-sizeof(u_int16_t)-sizeof(pid_t)];
	};
};


class Communication
{
public:
	Communication(unsigned char ObjectType);
	~Communication();

	bool SendCommand(const u_int16_t	CommData,const u_int16_t dataWORD);		// Parent
	bool IsDataReady(void);																							// Parent
	bool ReceiveData(u_int16_t	&CommData,u_int16_t  &dataWORD);							// Parent

	bool SendData(const u_int16_t	CommData, const u_int16_t dataWORD)	;				// Child
	bool IsCommandReady(void);																						// Child
	bool ReceiveCommand(u_int16_t	&Command,u_int16_t  &dataWORD);					// Child

	bool StartChild(const char *AppName, const char *Argv[]);

private:

	logger  *CommLogger;						// this is for private usage
	struct FileMap  *INPFileMap;				// FileMap Address of Input File
	struct FileMap  *OUTFileMap;				// FileMap Address of Output File
	u_int8_t  ObjectType;							// This is Parent/Child Object

	sem_t *OutputSem;								// Semaphre for Output
	sem_t *InputSem;								// Semaphore for Input


// For to be sure that data/command received by the target
	bool VerifiyData(const u_int16_t	CommData,const u_int16_t dataWORD);
	bool VerifyCommand(const u_int16_t	CommData,const u_int16_t dataWORD);

// for internal synch between child and parent
	bool InitSemaphore(void);
	void ReleaseSemaphore(void);
	bool LockInput(void);
	bool LockOutput(void);
	bool UnlockInput(void);
	bool UnlockOutput(void);

// for communication data/command transfer
	bool InitSharedFiles(void);
	void ReleaseSharedFiles(void);

// for internal logging
	bool InitLogger(void);
	void ReleaseLogger(void);

};

#endif /* COMM_H_ */
