#ifndef AUDIO_MGR_H_
#define AUDIO_MGR_H_

#include <stdio.h>			// always include stdio.h
#include <stdlib.h>			// always include stdlib.h
#include <cstdint>
#include <fcntl.h>			// defines open, read, write methods
#include <unistd.h>			// defines close and sleep methods
#include <sys/ioctl.h>			// defines driver ioctl method
#include <linux/soundcard.h>		// defines OSS driver functions
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "logger.h"

#define SOUND_DEVICE 	"/dev/dsp"		/* OSS Main Sound Device */
#define SAMPLE_RATE 	44100			/* The sample rate of the audio codec.*/
#define BLOCKSIZE 		44100			/*  Parameters for audio thread execution */
#define MIXER_DEVICE 	"/dev/mixer"	/* Mixer Device */
#define NUM_CHANNELS 	2				/*For stereo sounds*/
#define AUDIO_FILES_COUNT 5

#if(VOL_LEVEL_PATCH_TABLE_ACTIVE == TRUE)
	// Because of there is difference between F123 and DM355 sound levels, we need a patch table for //
		// dm355 volume level setting //
	#define VOL_LEVEL_TABLE_MEMBER_COUNT	11
	#define VOL_LEVEL_SETTING_TABLE		{0,8,16,24,30,40,50,58,65,70,75}
#endif

class AudioMgr {
	public :
		AudioMgr(Logger *CLogi = NULL);
		~AudioMgr();

		volatile bool threadRunning;		// main FLAG indicates thread's BODY is executing
		volatile bool ThreadBusy;

	private:

		void PlayCurrentSound();
		bool initSoundDevice(void);
		void playSimpleAudio(char indexOfSound);
		void cleanUpThread(void);
		bool initOutputBuffer(void);
		bool createTempFiles(void);
		bool initAudioFiles(void);
		void releaseAudioFiles(void);
		bool setAudioLevel(int audioLevel);

		bool audioFilesOpened[AUDIO_FILES_COUNT];
		bool tempFilesOpened[AUDIO_FILES_COUNT];
		FILE *audioFilesHandlerArray[AUDIO_FILES_COUNT];
		FILE *tempFilesHandlerArray[AUDIO_FILES_COUNT];

		int     		outputFd;				// output driver file descriptor (i.e. handle)
		char   			*outputBuffer;			// output buffer for write data to soundCard
		volatile bool 	LoopActive;				// Flag for main audio processing loop
		volatile bool 	InitStatus;				// check for success of initializing

		volatile int SoundIndex;
		Logger *CLogger;
		void SetThreadBUSY(void);
		void SetThreadFREE(void);
};


#endif /*AUDIO_THREAD_H_*/
