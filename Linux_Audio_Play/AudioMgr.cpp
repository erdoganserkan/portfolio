#ifndef AUDIO_MGR_CPP
#define AUDIO_MGR_CPP

#include <math.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "logger.h"
#include "AudioMgr.h"	// audio thread definitions

const char *WTGGsoundDir="/home/SingleSounds/";
const char *WTGGtmpSoundDir="/tmp/SingleSounds/";
const char *WTGGSounds[]={"ButtonError.wav","ButtonOK.wav","PopUpCome.wav", "ResetOnay.wav","SystemClose.wav"};

using namespace std;

char buffer[2] = {'1','0'};
#if(VOL_LEVEL_PATCH_TABLE_ACTIVE == TRUE)
	int VolLevelPatchTable[VOL_LEVEL_TABLE_MEMBER_COUNT]=VOL_LEVEL_SETTING_TABLE;
#endif

AudioMgr::AudioMgr(Logger* CLogi) {
// init variables
	ThreadBusy = true;
	CLogger = NULL;
	threadRunning = true;
	LoopActive = false;
	SoundIndex = 0;
	InitStatus = true;
	outputBuffer = NULL;

//	Open thread debugging system
	CLogger = CLogi;

//	Call initializing functions
	if(initOutputBuffer() == false) {
		ERRL(CLogger,"initOutputBuffer() FAILED \n");
		InitStatus = false;
	};

	if(initAudioFiles() == false) {
		ERRL(CLogger, "initAudioFiles() FAILED \n");
		InitStatus = false;
	};

	if(initSoundDevice() == false) {
		ERRL(CLogger,"function FAILED \n");
		InitStatus = false;
	};
}


AudioMgr::~AudioMgr()		/*DECONSTRUCTOR*/
{
	TRACEL(CLogger,"started\n");
	cleanUpThread();

    if(CLogger != NULL)
    	delete CLogger;

}


bool AudioMgr::setAudioLevel(int audioLevel)
{
	#if(VOL_LEVEL_PATCH_TABLE_ACTIVE == TRUE)
		// Read audio level real-value from Volume-Level-Setting-Patch Table //
		audioLevel = VolLevelPatchTable[audioLevel/10];
	#endif

    int     vol         = audioLevel | (audioLevel << 8);
    int mixerFd;

    DEBUGL(CLogger,"new audioLevel = %d \n",audioLevel);
    // open mixer device
    mixerFd = open(MIXER_DEVICE, O_RDONLY);
    if (mixerFd == -1)
    {
    	ERRL(CLogger,"open(%s) FAILED :R: (%s)\n", MIXER_DEVICE, strerror(errno));
        return false;
    }
    // Set the output volume
    if (ioctl(mixerFd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1)
    {
    	ERRL(CLogger,"setting the volume Level FAILED :R: (%s)\n",strerror(errno));
        close(mixerFd);
        return false;
    }

    TRACEL(CLogger,"EXITING\n");
    close(mixerFd);
    return true;
}

void AudioMgr::PlayCurrentSound()
{
	FILE *currentFileHandler;
	volatile bool eofFLAG = false;
	int readedBytesCount = 0;

	if(tempFilesOpened[SoundIndex])
	{
		currentFileHandler = tempFilesHandlerArray[SoundIndex];
	}
	else if (audioFilesOpened[SoundIndex])
	{
		currentFileHandler = audioFilesHandlerArray[SoundIndex];
	}
	else
	{
		ERRL(CLogger,"index : [%d] file does not exist,playing FAILED \n",SoundIndex);
		return;
	}

	if(fseek(currentFileHandler,0,SEEK_SET)== -1)
	{
		ERRL(CLogger, "fseek() for clear file FAILED :R: (%s)\n",strerror(errno));
		return;
	}
	LoopActive = true;

	DEBUGL(CLogger, "Entering simpleAudioLoop \n");
	while (LoopActive == true)
	{
		TRACEL(CLogger, "ftell out: %ld\n",ftell(currentFileHandler));
		readedBytesCount = (int)fread(outputBuffer, sizeof(char), BLOCKSIZE, currentFileHandler);
		if(feof(currentFileHandler))
		{
			fseek(currentFileHandler,0,SEEK_SET);
			DEBUGL(CLogger, "EOF accured\n");
			eofFLAG = true;
		}
		if(ferror(currentFileHandler))
		{
			ERRL(CLogger, "inputFile fread FAILED :(%s):",strerror(errno));
			LoopActive = false;
		}
		if (LoopActive == true)
			if(write(outputFd, outputBuffer, readedBytesCount) == -1)
			{
				ERRL(CLogger, "writintg data to device(%s) FAILED :R:(%d) \n",SOUND_DEVICE,strerror(errno));
				LoopActive = false;
			}
		if(eofFLAG==true)
		{
			LoopActive = false;
			eofFLAG = false;
		}
	}
	memset(outputBuffer,0,BLOCKSIZE);
	TRACEL(CLogger,"EXITING\n");

}


void AudioMgr::releaseAudioFiles(void)
{
	TRACEL(CLogger,"ENTERED\n");
	for(int i=0;i<AUDIO_FILES_COUNT;++i)
	{
		if(audioFilesOpened[i]==true)
		{
			if(audioFilesHandlerArray[i] != NULL)
				fclose(audioFilesHandlerArray[i]);
			audioFilesOpened[i]=false;
		}
		if(tempFilesOpened[i]==true)
		{
			if(tempFilesHandlerArray[i] != NULL)
				fclose(tempFilesHandlerArray[i]);
			tempFilesOpened[i]=false;
		}
	}
}


bool AudioMgr::initAudioFiles(void)
{
    char tempFilePtr[50];
    memset(tempFilePtr,0,sizeof(tempFilePtr));

    char *tempBuffer = NULL;
    int i=0;
	int copyBytesCount = 0;
	volatile bool eofFlag = false;

    for(i=0;i<AUDIO_FILES_COUNT;++i)
    {
		audioFilesOpened[i]=false;
		tempFilesOpened[i]=false;
	}

    bool tmpDirExist = false;
    mode_t mode = 0777;

// 1 check if temp files root is created - required for development tries
    if(0 != access(WTGGtmpSoundDir, F_OK))		// may be problem here
    {
	    if(mkdir(WTGGtmpSoundDir,mode)==-1)
	    {
	    	ERRL(CLogger,"(%s) creation FAILED :R:(%s)\n",WTGGtmpSoundDir,strerror(errno));
	    	tmpDirExist = false;
	    }
	    else
	    {
	    	DEBUGL(CLogger,"(%s) creation SUCCESS \n",WTGGtmpSoundDir);
	    	tmpDirExist = true;
	    }
	}
    else
    {
    	tmpDirExist = true;
    }

// 2 create temp buffer for copying data
	if((tempBuffer = (char *)calloc(BLOCKSIZE,sizeof(char))) == NULL)
    {
        ERRL(CLogger,"allocateion of tempBuffer (%d BYTES) FAILED :R:(%s)\n", BLOCKSIZE, strerror(errno));
    }
	else
	{
		DEBUGL(CLogger,"allocation of tempBuffer (%d BYTES) SUCCESS \n", BLOCKSIZE);
	}

// 3  copy all data of real files to temp files
	for(i=0;i<AUDIO_FILES_COUNT;++i)
    {
		// 3.1 open real audio file
    	strcpy(tempFilePtr,WTGGsoundDir);
    	strcat(tempFilePtr,WTGGSounds[i]);
        audioFilesHandlerArray[i] = fopen(tempFilePtr, "r");
        if(audioFilesHandlerArray[i] == NULL)
        {
            ERRL(CLogger, "open real file (%s) FAILED :R:(%s)\n", tempFilePtr, strerror(errno));
            audioFilesOpened[i]=false;
        }
        else
        {
            DEBUGL(CLogger,"open real file (%s) SUCCESS \n", tempFilePtr);
            audioFilesOpened[i]=true;
        }

		if(tmpDirExist == true)
		{
			// 3.2 create temp files under TmpSoundsDir
			if(audioFilesOpened[i]==true)
			{
				strcpy(tempFilePtr,WTGGtmpSoundDir);
				strcat(tempFilePtr,WTGGSounds[i]);
				tempFilesHandlerArray[i] = fopen(tempFilePtr, "w+");
				if(tempFilesHandlerArray[i] == NULL)
				{
					ERRL(CLogger,"create temp file (%s) FAILED :R:\n", tempFilePtr,strerror(errno));
					tempFilesOpened[i]=false;
				}
				else
				{
					DEBUGL(CLogger,"create temp file (%s) SUCCESS\n",tempFilePtr);
					tempFilesOpened[i]=true;
				}
			}
			// 3.3 copy real file to temp file
			if((audioFilesOpened[i])&&(tempFilesOpened[i])&&(tempBuffer != NULL))
			{
				eofFlag = false;
				while(!eofFlag)		// COPY DATA UNTIL TO EOF ACCURES
				{
					if(copyBytesCount=fread(tempBuffer,sizeof(char),BLOCKSIZE,audioFilesHandlerArray[i]))
					{
						if(feof(audioFilesHandlerArray[i])){
							DEBUGL(CLogger,"%s fread failed-EOF accured \n",WTGGSounds[i]);
							clearerr(audioFilesHandlerArray[i]);
							eofFlag = true;
						}
						if(ferror(audioFilesHandlerArray[i])){
							ERRL(CLogger,"%s fread FAILED-ferror accured :R:(%s)\n",WTGGSounds[i],strerror(errno));
							clearerr(audioFilesHandlerArray[i]);
							break;		/* if any error accures stop copying for this file */
						}
					}
					if(!fwrite(tempBuffer,sizeof(char),copyBytesCount,tempFilesHandlerArray[i]))
					{
						ERRL(CLogger,"fwrite to tempFilesHandlerArray[%d] FAILED :R:(%s)\n",i,strerror(errno));
						clearerr(tempFilesHandlerArray[i]);
						break;			/* if any error accures stop copying for this file */
					}
				}
				if(fseek(tempFilesHandlerArray[i],0,SEEK_SET)== -1)		// ready for future usage
					ERRL(CLogger,"fseek() FAILED on %d. index temp-file :R:(%s)\n",i,strerror(errno));
				if(fseek(audioFilesHandlerArray[i],0,SEEK_SET)== -1)	// ready for future usage
					ERRL(CLogger,"fseek() FAILED on %d. index real-file :R:(%s)\n",i,strerror(errno));
			}
			else
				ERRL(CLogger,"NOT ENTERED TO COPY LOOP \n");
		}
		else
			ERRL(CLogger, "tempDir is NOT EXIST\n");
    }
	free(tempBuffer);

	return true;
}


void AudioMgr::cleanUpThread(void)
{
    releaseAudioFiles();

   	if(close(outputFd) == -1)
   		ERRL(CLogger, "closing audio output device(%s) FAILED :R:(%s)\n",SOUND_DEVICE,strerror(errno));

   	if(outputBuffer!=NULL)
    	free(outputBuffer);

    TRACEL(CLogger, "EXITING\n");
}


bool AudioMgr::initOutputBuffer(void)
{
	bool RetVal = true;
    if((outputBuffer = (char *)calloc(BLOCKSIZE,sizeof(char))) == NULL)
    {
        ERRL(CLogger, "allocate outputBuffer FAILED (%d BYTES) :R:(%s)\n", BLOCKSIZE,strerror(errno));
        RetVal = false;
    }
    else
    {
    	DEBUGL(CLogger, "allocate outputBuffer SUCCESS (%d BYTES) @ address (%p)\n", BLOCKSIZE, outputBuffer);
    }

    return RetVal;
}


// this func is so critical & it must deserve full success
bool AudioMgr::initSoundDevice(void)
{
	uint32_t format = AFMT_S16_LE;
	int32_t numchannels = NUM_CHANNELS;
	uint32_t samplerate = SAMPLE_RATE;

	if ((outputFd = open(SOUND_DEVICE, O_WRONLY)) == -1)
	{
		ERRL(CLogger, "opening (%s) FAILED :(%s):\n",SOUND_DEVICE,strerror(errno));
		return false;
	}
	if (ioctl(outputFd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		ERRL(CLogger,"setting sound-format FAILED :(%s):\n",strerror(errno));
		return false;
	}
	if (ioctl(outputFd, SNDCTL_DSP_CHANNELS, &numchannels) == -1)
	{
		ERRL(CLogger,"setting mixer channels FAILED :(%s):\n", strerror(errno));
		return false;
	}
	if (ioctl(outputFd, SNDCTL_DSP_SPEED, &samplerate) == -1)
	{
		ERRL(CLogger, "setting sample rate FAILED :(%s):\n", strerror(errno));
		return false;
	}
	return true;
}

#endif
