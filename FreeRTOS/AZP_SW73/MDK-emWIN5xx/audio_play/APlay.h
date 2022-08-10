#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

#include <stdint.h>

#if(AUDIO_IC_LPCDAC == USED_AUDIO_DEVICE)
	#define AUDIO_SAMPLE_FREQ_HZ		(16000)		/* MONO */
#else
	#define AUDIO_SAMPLE_FREQ_HZ		(32000)		/* STEREO */
#endif
#define AUDIO_CODEC_I2C_FREQ_HZ			(200000)
#define APLAY_DMA_SIZE_MAX				((0x1U<<12)-1)	/* Maximum 12Bit length is allowed*/
#define APLAY_DMA_SIZE					(512)
#define APLAY_WAIT_FOR_PREV_MAX_MS		(1500)


// Frequencies of Music Notes // 
#define DO1_FREQ_HZ		261
#define RE_FREQ_HZ		293
#define MI_FREQ_HZ		329
#define FA_FREQ_HZ		349
#define SOL_FERQ_HZ		392
#define LA_FREQ_HZ		440
#define SI_FREQ_HZ		493
#define DO2_FERQ_HZ		523

typedef enum {
	AUDIO_TASK_NOINIT		= 0,	// Initialization NOT COMPLETED yet // 
	AUDIO_TASK_IDLE			= 1,	// Initialization completed or previous sound play completed // 
	AUDIO_TASK_PLAYING	= 2,	// An audio file has been playing // 
	AUDIO_TASK_ERR			= 3,	// An error occured 		
	
	AUDIO_TASK_COUNT
}  aUDIO_TASK_STATEs;
#endif

extern uint8_t is_audio_playing(void);
extern void audio_play_init(void);
extern void start_dac_audio(uint8_t audio_indx, uint8_t blocking);
extern void start_i2s_audio(uint8_t audio_indx, uint8_t blocking);
extern void stop_i2s_audio(void);
extern uint8_t audio_get_state(void);
extern uint8_t APLAY_DAC_DMA_Handler(void);
extern inline uint8_t isDacAudioActive(void);

