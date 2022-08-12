#ifndef ADHOC_H_
#define ADHOC_H_

#include <stdio.h>
#include <stdint.h>

#define MAX_ENC_NAME_LENGTH     64
#define MAX_SSID_MODE_LENGTH    32
#define MAX_SSID_NAME_LENGTH        32
#define MAX_SSID_PASS_LENGTH        32
#define MAX_NUMBER_OF_WIFI_NETWORK  30
#define MAX_SSID_QUALITY_LENGTH     9
#define SSID_PASS_LENGTH            10
#define WIFI_CHECK_INTERVAL_S       15
#define IP_LENGTH                   16

typedef struct ssid_info_s
{
    uint8_t key_on; // If set to "1" encryption is active, if it is set to "0" encryption is disabled //
    uint8_t name[MAX_SSID_NAME_LENGTH];
    uint8_t encryption_type[MAX_ENC_NAME_LENGTH];
    uint8_t quality[MAX_SSID_QUALITY_LENGTH];
    uint8_t pass[MAX_SSID_PASS_LENGTH];
    uint8_t mode[MAX_SSID_MODE_LENGTH];
} __attribute__((packed)) ssid_info_t;
#endif

typedef struct ssid_connect_status_s
{
    uint8_t status;
    uint8_t name[MAX_SSID_NAME_LENGTH];
} __attribute__((packed)) ssid_connect_status_t;

typedef struct ssid_connect_s
{
    uint8_t name[MAX_SSID_NAME_LENGTH];
    uint8_t pass[MAX_SSID_PASS_LENGTH];
} __attribute__((packed)) ssid_connect_t;

typedef struct wifi_adhoc_s
{
    uint8_t is_enabled;                 // bool: {0,1}
    uint8_t ssid[MAX_SSID_NAME_LENGTH]; // null terminating string
    uint8_t is_ssid_hidden;             // Warning; device does not support! bool : {0,1}
    uint8_t encryption_type;            // one of enum wifi_enc_methode_type_e value
    uint8_t pass[MAX_SSID_PASS_LENGTH]; // null terminated string
    uint8_t ip[IP_LENGTH];              // null terminated string
    uint8_t ch;
    uint8_t netmask[IP_LENGTH];         // null terminated string
    uint8_t broadcast[IP_LENGTH];       // null terminated string
} __attribute__((packed)) wifi_adhoc_t;

#define MIN_ADHOC_BW_PROCESS_CNT 5
#define MAX_PING_ERR_CNT		3

#define ADHOC_INTERFACE	"wlan0"
#define HOTSPOD_FILES_DIR	"/root/hotspod"
#define IPTABLES_CLEAR_SCRIPT	"clear_iptables.sh"
#define START_WIFI_HOTSPOD_SCRIPT 	"set_wifi_internet_bridge.sh"
#define START_ETH_HOTSPOD_SCRIPT 	"set_eth_internet_bridge.sh"
#define ADHOC_SCRIPT_FILE		"/home/user/adhoc.sh"
#define WIFI_DRIVER_FILES_DIR	"/home/user"

void adhoc_init(void);
void adhoc_reinit(void);
uint8_t read_adhoc_config(wifi_adhoc_t *adhoc);
void save_adhoc_config(const wifi_adhoc_t *adhoc);
void send_adhoc_config_to_gui(void);
void process_hotspod(void);


#endif /* ADHOC_H_ */
