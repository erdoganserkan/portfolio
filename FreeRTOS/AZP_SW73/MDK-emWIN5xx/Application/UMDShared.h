#ifndef UMD_SHARED_H
#define UMD_SHARED_H

#include "PlatformSpec.h"	// Bu dosya herkesin projesinde farklidir ve islemciye ve projeye özgü tanimlamalar icerir. //
														// Mesela asagida kullanilan "uint8_t" türünün nasil tanimlanmasi gerektigi bu dosyada yapilmalidir. //
														// Genelde "typedef unsigned char uint8_t;" tanimi kullanilir. 

// LUTFEN, Herhangi bir degisiklik yapmadan once revizyon numarasini arttiriniz //
#define UMD_SHARED_VERSION		(107)

// CHANGE HISTORY //
// 101 -> 102 : 
	// Uart baudrate definition added // 
	// Last changes from meeting has been done // 
// 102 -> 103 :
	// Clock Generation function added 
	// Uart baudrate has been increased to 115200

// 106:
	// Detector is READY command added 

// 107:
	// AZP_TARGET_NUM_MAX	added 
	

#define UMD_UART_BAUDRATE        (19200)
#define UMD_FIXED_PACKET_LENGTH		(4)		// Her paket bu uzunlukta olmalidir. // 
#define UMD_CMD_TIMEOUT_DEFAULT_MS		1000	// Komutlarin cevabinin gelmesi icin maximum bekleme süresi //
#define UMD_PKT_TIMEOUT_DEFAULT_MS		100		// Paketlerin maksimmu toplam alma s�resi //
#define UMD_CMD_TIMEOUT_SEARCH_CMD_MS	1500	// Komutlarin cevabinin gelmesi icin maximum bekleme süresi //
#define UMD_CMD_TIMEOUT_GB_CMD_MS		1500	// Komutlarin cevabinin gelmesi icin maximum bekleme süresi //
#define UMD_CMD_TIMEOUT_DEPTH_CALC_MS	5000
#define UMD_CMD_TIMEOUT_SET_SENSE_MS	2000
#define UMD_CMD_TIMEOUT_SET_FERRO_MS	2000
#define UMD_CMD_TIMEOUT_SET_GID_MS		2000
#define CMD_RESPONSE_TIMEOUT_MAX_COUNT	1			// Bir komut icin cevap gelmedigine karar verene kadar kaç adet gönderilecegini 
																					// belirtir. Eger "1" olarak ayarlanirsa her komut sadece bir kere gönderilir. 
																					// Tekrarlama yapilmaz.
#define SYS_LOAD_TOTAL_LENGHT_MS	(12000)	
#define SYS_LOAD_NOISE_CANCELLATION_TIMEOUT_MS	(300000)	// 300 seconds  maximum wait // 
#define DETECTOR_INIT_TOTAL_TIMEOUT_MS	(SYS_LOAD_NOISE_CANCELLATION_TIMEOUT_MS + SYS_LOAD_TOTAL_LENGHT_MS)
#define WAIT_BEFORE_HANDSHAKE_MS	(4000)
#define GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS	(1500)

#define SYNC_PACKET_BYTES		{0x5A, 0x5A, 0x5A, 0x5A}
//-----------------------------------------------------------------------//
//----- SETTINGs MIN & MAX VALUEs / DEFINITIONs -------------------------//
//-----------------------------------------------------------------------//
typedef enum
{
	SMALL_COIL	 	= 0,
	COIL_TYPE_MIN	= SMALL_COIL,
	MIDDLE_COIL		= 1,
	BIG_COIL			= 2,
	NO_COIL_CONNECTED	= 3,
	COIL_TYPE_MAX	= NO_COIL_CONNECTED,
	
	COIL_COUNT
} CoilType;		// Bobin Tipleri // 

// Metal Analizi, Otomatik Arama Ekranlarindaki Target Tipleri // 
// Full target turleri liste // 
typedef enum
{
	TARGET_NOTARGET = 0,
	TARGET_CAVITY,
	TARGET_FERROs,
	TARGET_NFERROs,
	TARGET_GOLD,
	TARGET_MINERAL,
	TARGET_ID_MAX = TARGET_MINERAL,
	
	TARGET_ID_COUNT
} eTARGET_TargetIDs;

// Standart Arama ve Mineralli Bolge Arama Ekranlarindaki Target Tipleri // 
// Daraltilmis target turleri listesi // 
typedef enum
{
	RTID_NOTARGET = 0,	// YELOW Target Draw 
	RTID_CAVITY,	// Blue Target Draw 
	RTID_METAL,		// RED Target Draw 
	RTID_MINERAL,	// Orange Target Draw 
	RTID_MAX = RTID_MINERAL,
	
	RTID_COUNT
} eReduced_TIDs;

// Arama Tiplerinin Listesi //
// CMD_START_SEARCH komutunun tek parametresidir // 
typedef enum
{
	STD_SEARCH_TYPE	= 0,		// Standard Aramaya Basla //
	MINERALIZED_SEARCH_TYPE,	// Mineralli Bolge aramasina basla //
	METAL_ANALYSIS_TYPE,	// Metal analizi aramasina basla //
	AUTOMATIC_SEARCH_TYPE,	// Otomatik Aramaya basla // 
	AZP_TARGET_NUM_ANALYSIS,	// Jeo Finder AZ Plus target number analysis mode

	SEARCH_TYPE_COUNT
} eSearchScreen_Type;

// Response paketlerinin STATE kisminda bulunan verinin açiklamasi // 
// The "RSP_STATUS" section of RSP_XX messages // 
typedef enum
{
	CMD_DONE 		= 0,	// Komut basariyla tamamlandi //
	CMD_PENDING,			// Komut icrasi devam ediyor daha sonuclanmadi // 
	CMD_FAILED,				// Komut basarisiz oldu //
	
	CMD_STATUS_COUNT
} UMD_RSP_STATUS;

// Toprak Ayarinin BASARISIZ olma nedenleri //
typedef enum
{
	OVER_METAL	= 0,		// Metal uzerinde olmak //
	HIGH_MINERALIZATION,	// Yuksek minerallilik //
	GB_FAIL_UNKNOWN,		// Blinmeyen neden //
	
	GB_FAIL_REASON_COUNT
} eGBFailReason_Type;

// Hassasiyet Tanimlamalari //
#define SENSITIVITY_MIN			(10)
#define DEFAULT_SENSITIVITY		(70)
#define SENSITIVITY_MAX			(100)

// Ses Seviyesi Tanimlamalari //
#define VOLUME_MIN				(0)	// mute level // 
#define DEFAULT_VOLUME			(10)
#define VOLUME_MAX				(100)

// Toprak Mineralizasyon Degeri Tanimlamalari //
#define GROUND_ID_MIN				(0)
#define DEFAULT_GROUND_ID		((((uint16_t)90)<<8) | ((uint16_t)0x04))	/* Default value is 90.4 */
#define GROUND_ID_MAX				(180 * CLOCK_GENERATE_PHASE_FRACTION)

// Degerli/Degersiz Ayrimi Acik/Kapali Ayarlari //
#define FERROS_DISABLED	(0x0)						// DEGERSIZ Metallere Tepki VERILMEYECEK //
#define FERROS_ENABLED	(0x1)						// DEGERSIZ Metallere Tepki VERILECEK //
#define DEF_FERROS_STATE	FERROS_ENABLED

// DERINLIK TANIMLAMALARI //
#define DEPTH_MAX_CM				250	//:TODO: Detector Algorithm Developer will set this // 
#define DEPTH_MAX_TARGET_SIDE_CM	95
#define DEPTH_TARGET_SIDE_STEP_CM	10

#define AZP_DEPTH_MAX_TARGET_SIDE_CM	100
#define AZP_DEPTH_TARGET_SIDE_STEP_CM	5

// Gauge Degeri Tanimlamalari // 
#define UMD_GAUGE_MAX		(100)
#define UMD_GAUGE_MIN		(0)
#define A5P_AUTO_GAUGE_MAX	95
#define A5P_ALLM_GAUE_MAX	70
#define A5P_DISC_GAUE_MAX	70

// A, B ve C clock sinyali uetiminde kullanilan FAZ verisinin carpan degeri
#define DEFAULT_INPUT_CLK								(12000UL)	
#define CLOCK_GENERATE_PHASE_FRACTION					(10)

#define AZP_TARGET_NUM_MAX		99		// IND_GET_TRGET_NUM komutunun data' s�n�n maximum degeri // 
#define AZP_TARGET_NUM_MIN		10		// IND_GET_TRGET_NUM komutunun data' s�n�n minimum degeri // 

//-----------------------------------------------------------------------//
//----- ENUMARATION of COMMANDs & RESPONSEs & INDICATIONS ---------------//
//-----------------------------------------------------------------------//
typedef enum
{
	UMD_CMD_PRERESERVED_AREA_START	= 0,	// NOT USED // 
	UMD_CMD_MIN = UMD_CMD_PRERESERVED_AREA_START,
	UMD_CMD_PRERESERVED_AREA_END		= 10,	
	
	UMD_CMD_DISPLAY_DETECTOR_START	= 11,	// USED FOR DETECTOR & DISPLAY COMMUNICATION // 
	UMD_CMD_DISPLAY_DETECTOR_END		= 100,
	
	//TODO: Add any new cmd areas here // 
	
	UMD_CMD_POSTRESERVED_AREA_START	= 245,	// NOT USED // 
	UMD_CMD_POSTRESERVED_AREA_END		= 255,
	UMD_CMD_MAX = UMD_CMD_POSTRESERVED_AREA_END
} UMD_CMD_RANGEs;


// Mesaj tipleri ve aciklamalari // 
	// 1- COMMAND  // 
	// CMD; Bu mesaj turu eger bir isin hemen yapilmasi gerekiyorsa ve isin yapilip yapilmadigi bilgisinin alinmasi onemliyse kullanilir. 
		// Mesela Arama ekranlarina girilmesi onemli oldugundan Display Dedektor'e bu komutlari CMD_xx olarak gonderir ve cevabini da 
		// kesinlikle ister //
	// 2- RESPONSE //
	// RSP; Ilgili komutun yapilma durumunun sonucunun komutu gonderene belirtilmesi icin gonderilir. Mesela Arama ekranlarina girme komutlarinin
		// cevaplari icin RSP_xx isimlendirmesi kullanilmaktadir.
	// 3- INDICATION // 
	// IND;	Ayni mesaj surekli tekrarlanacaksa ve gerekli islemin yapilip yapilmadiginin geri donusu onemli degilse kullanilir. Mesela Metal 
		// sinyali siddeti (Gauge Degeri) surekli gonderildigi ve karsi taraf tarafindan alinip alinmamasi her mesaj icin onemli olmadigindan 
		// IND_xx olarak isimlendirilmistir. 
#pragma pack(1)
// Genel Paket Tipi Tanimlamasi //
typedef struct 
{
	uint8_t cmd;		// Tun mesajlarda kullaniliyor //
	uint8_t length;	// Tum mesajlarda var ama simdilik kullanilmiyor // 
	union {	// her komut response icin gerekli olan data bu bolgede bulunur. // 
		uint8_t sensitivity;	// CDM_SET_SENSITIVITY icin // 
		uint8_t coil_type;		// CMD_SET_COIL_TYPE icin // 
		uint8_t volume;				// CMD_SET_VOLUME icin // 
		uint16_t ground_id;		// CMD_SET_GROUND_ID icin // 
		uint8_t ferros_state;	// CMD_SET_FERROS_STATE icin // 
		uint8_t gb_fail_reason;	// CMD_GB_FAILED icin // 
		uint8_t search_type;	// CMD_START_SEARCH icin // 
		uint8_t gauge;			// IND_GET_GAUGE	icin //
		uint8_t target_id;	// IND_GET_TARGET_ID icin // 
		uint8_t target_num;	// IND_GET_TARGET_NUM // 
		uint8_t cmd_status;	// Tum RSP_xx komutlari icin //
		uint8_t gb_type;	// eGBTypes // 
		struct {
			int8_t tnum_left;
			int8_t tnum_right;
		} TnumExt;
		struct {
			uint8_t width;
			uint8_t Height;
		} Depth_Params;	// CMD_START_DEPTH_CALCULATION icin //
		struct {
			uint8_t cmd_status;
			uint8_t depth;	
		} Depth_Rsp;	// RSP_START_DEPTH_CALCULATION icin // 
		uint8_t data[2];
	}data;
} UmdPkt_Type;

typedef enum {
	GB_TYPE_LONG = 0,
	GB_TYPE_SHORT,
	GB_TYPE_AZP_AUTO,
	GB_TYPE_AZP_MANUAL,
	
	GB_TYPEc_COUNT = 2
} eGBTypes;

#pragma pack()

typedef enum 
{
	CMD_UMD_FIRST 		= 11,
	// Hassasiyet Seviyesinin Set Edilmesi (KOMUT) //
	CMD_SET_SENSITIVITY = 11,	// Serkan -> Ali; "CMD_xx(U8) + LENGTH(U8) + Data(U8, SENSITIVITY) + NULL(U8)" //
	RSP_SET_SENSITIVITY = 12,	// Ali -> Serkan; "RSP_xx(U8) + LENGTH(U8) + Data(U8, RSP_STATUS) + NULL(U8)" //	
	
	// Bobin Tipinin Set Edilmesi (KOMUT) //
	CMD_SET_COIL_TYPE = 13,	// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + Data(U8, COIL_TYPE) + NULL(U8)" //
	RSP_SET_COIL_TYPE	= 14,	// Serkan -> Ali; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS) + NULL(U8)"  //	

	// Ses Seviyesinin Set Edilmesi (KOMUT) // 
	// BU KOMUT ARTIK GECERLI DEGIL. SES KISMI KOMPLE DIGITAL KARTA TASINMISTIR // 
	CMD_SET_VOLUME = 15,	// Serkan -> Ali; "CMD_xx(U8) + LENGTH(U8) + Data(U8, VOLUME) + NULL(U8)" //
	RSP_SET_VOLUME = 16,	// Ali -> Serkan; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS) + NULL(U8)" // 
	
	// Toprak icin ID Degerinin Set Edilmesi, GB Ekraninda "Ali->Serkan" yonunde ve ilk acilista "Serkan->Ali" yonunde //
		// gonderilir. // 
	CMD_SET_GROUND_ID	= 17,	// Ali <-> Serkan; "CMD_xx(U8) + LENGTH(U8) + Data(U8, MINERALIZATION ID, MSB) + Data(U8, MINERALIZATION ID: LSB)" //
   RSP_SET_GROUND_ID   = 18,   // Serkan <-> Ali; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS)" + NULL(U8) //
		
	// Degersiz Metallerin Gormezden Gelinmesi Ayarinin uretilmesi (KOMUT) //
	CMD_SET_FERROS_STATE	= 19,	// Serkan -> Ali; CMD_xx(U8) + Data(FERROS_IGNORENCE_STATE, U8) // 
	RSP_SET_FERROS_STATE	= 20,	// Ali -> Serkan; RSP_xx(U8) + Data(CMD_STATUS, U8) //

	// SEARCH & GROUND BALANCE Related CMDs & RSPs // 
	// Toprak Ayarina Girilmesi (KOMUT) //
	CMD_START_GROUND_BALANCE	= 21,	// Serkan -> Ali ; "CMD_xx(U8) + LENGTH(U8) + GB_TYPE(U8): eGBTypes + NULL(U8)" //
	RSP_START_GROUND_BALANCE	= 22,	// Ali -> Serkan ; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS) + NULL(U8)" //
		
	// Toprak Ayari Hesaplamalarinin BASARILI olarak Bitirilmesi (KOMUT) //
	CMD_GB_COMPLETED	= 23,	// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + NULL(U8) + NULL(U8)"
	RSP_GB_COMPLETED	= 24,	// Serkan -> Ali; RSP_xx(U8) + LENGTH(U8) + Data(CMD_STATUS, U8) + NULL(U8) // 
	
	// Toprak Ayari Hesaplamalarinin BASARISIZ olarak Bitirilmesi (KOMUT) //
	CMD_GB_FAILED	= 25,	// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + FAIL_REASON(U8) + NULL(U8)"
	RSP_GB_FAILED	= 26,	// Serkan -> Ali; RSP_xx(U8) + LENGTH(U8) + Data(CMD_STATUS, U8) + NULL(U8) // 

	// Toprak Ayarindan Cikilmasi (KOMUT), Toprak ayarinin icinde herhangi bir anda gelebilir //
	CMD_STOP_GROUND_BALANCE	= 27,	// Serkan -> Ali ; "CMD_xx(U8) + LENGTH(U8) + NULL(U8) + NULL(U8)" //
	RSP_STOP_GROUND_BALANCE	= 28,	// Ali -> Serkan ; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS) + NULL(U8)" //

	// Arama Ekranina Gir (KOMUT) //
	CMD_START_SEARCH = 29,	// Serkan -> Ali ; "CMD_xx(U8) + LENGTH(U8) + Data(U8, SEARCH_TYPE) + NULL(U8)" //
	RSP_START_SEARCH = 30,	// Ali -> Serkan ; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS)" //
		
	// Target Siddetinin Update Edilmesi (INDIKASYON), Arama ekranlarinda surekli gonderilir. //
	IND_GET_GAUGE	= 31,		// Ali -> Serkan; "IND_xx(U8) + LENGTH(U8) + Data(Gauge, U8) + NULL(U8)" //
	// Target ID Degerinin Update Edilmesi (INDIKASYON), Arama ekranlarinda surekli update edilir. // 
	IND_GET_TARGET_ID	= 32,	// Ali -> Serkan; "IND_xx(U8) + LENGTH(U8) + Data(TargetID, U8) + NULL(U8)" //
	
	// Arama Ekranindan Cikilmasi (KOMUT), Arama ekrani icindeyken herhangi bir anda gelebilir. // 
	CMD_STOP_SEARCH	= 33,	// Serkan -> Ali ; "CMD_xx(U8) + LENGTH(U8) + NULL(U8) + NULL(U8)" //
	RSP_STOP_SEARCH	= 34,	// Ali -> Serkan ; "RSP_xx(U8) LENGTH(U8)+ Data(U8, CMD_STATUS) + NULL(U8)" //
	
	// Derinlik Hesaplama Ekranina girilmesi (KOMUT) //
	CMD_START_DEPTH_CALCULATION = 35, 	// Serkan -> Ali; "CMD_xx(U8) + LENGTH(U8) + Width(U8) + Height(U8)" // 
	RSP_START_DEPTH_CALCULATION = 36,	// Ali -> Serkan; "RSP_xx(U8) + LENGTH(U8) + Data(U8, CMD_STATUS) + U8(Depth,CM)) // 
		
	// Dedektorun Yazilim Versiyonunun Alinmasi (KOMUT) //
	CMD_SEND_SOFT_VERSION	= 37,	// Serkan -> Ali; "CMD_xx(U8) + LENGTH(U8) + NULL(U8) + NULL(U8)"
	RSP_SEND_SOFT_VERSION	= 38,	// Ali -> Serkan; "RSP_xx(U8) + Data(U8,SOFTWARE_VERSION)"
	
	// Dedektorun Hardware Versiyonunun Alinmasi (KOMUT) //
	CMD_SEND_HARD_VERSION	= 39,	// Serkan -> Ali; "CMD_xx(U8) + LRNGTH(U8) + NULL(U8) + NULL(U8)"
	RSP_SEND_HARD_VERSION	= 40,	// Ali -> Serkan; "RSP_xx(U8) + Data(U8,HARDWARE_VERSION)"
	
	CMD_SET_A_CLOCK_DELAY	= 41,		// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + PHASE_MSB(U8) + PHASE_LSB(U8)"
	RSP_SET_A_CLOCK_DELAY	= 42,		// Serkan -> Ali; "RSP_xx(U8) LENGTH(U8)+ Data(U8, CMD_STATUS) + NULL(U8)" 
	
	CMD_SET_B_CLOCK_DELAY	= 43,		// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + PHASE_MSB(U8) + PHASE_LSB(U8)"
	RSP_SET_B_CLOCK_DELAY	= 44,		// Serkan -> Ali; "RSP_xx(U8) LENGTH(U8)+ Data(U8, CMD_STATUS) + NULL(U8)" 
	
	CMD_SET_C_CLOCK_DELAY	= 45,		// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + PHASE_MSB(U8) + PHASE_LSB(U8)"
	RSP_SET_C_CLOCK_DELAY	= 46,		// Serkan -> Ali; "RSP_xx(U8) LENGTH(U8)+ Data(U8, CMD_STATUS) + NULL(U8)" 

	CMD_SET_REF_CLOCK_FREQ	= 47,		// Ali -> Serkan; "CMD_xx(U8) + LENGTH(U8) + FREQ_MSB(U8) + FREQ_LSB(U8)"
	RSP_SET_REF_CLOCK_FREQ	= 48,		// Serkan -> Ali; "RSP_xx(U8) LENGTH(U8)+ Data(U8, CMD_STATUS) + NULL(U8)" 

	IND_DEDECTOR_IS_READY	= 49,	// Ali -> Serkan: "CMD_xx(U8) + LENGTH(U8) + NULL(U8) + NULL(U8)"
	
	// Target ID' nin Rakam ile verlmesi (INDIKASYON), FAST ekran�nda gerekti�inde g�nderilir //
	IND_GET_TARGET_NUM		= 50,		// Ali -> Serkan; "IND_xx(U8) + LENGTH(U8) + Data(Target Number, U8) + NULL(U8)" //

	IND_ANALOG_RESET		= 51,		// Serkan -> Ali; "IND_xx(U8) + LENGTH(U8) + Data(NULL(U8) + NULL(U8)" //
	
	IND_GET_TARGET_NUM_EXT	= 52,		// Ali -> Serkan; "IND_xx(U8) + LENGTH(U8) + Data(Target Number1, U8) + Data(Target Number2, U8)" //

	
	CMD_UMD_LAST = IND_ANALOG_RESET
	
} UMD_CMD_DISPLAY_DETECTOR;

#endif

