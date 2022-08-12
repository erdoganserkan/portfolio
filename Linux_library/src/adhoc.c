#include <fcntl.h>
#include <file.h>
#include <log.h>
#include <adhoc.h>

#define DEF_ADHOC_SETTINGS \
{	\
	.is_enabled = 0,		\
	.ssid = "mhotspod",		\
	.is_ssid_hidden = 0,	\
	.encryption_type = 0,	\
	.pass = "23nMVw8s23",	\
	.ip = "192.168.43.1",	\
	.ch = 6,				\
	"255.255.255.0",		\
	"192.168.43.255"		\
}


const static uint8_t ADHOC_CONFIGURATION[10][30] = { "Enabled\0", "Ssid\0", "Hidden\0",
                "Encryption\0", "Pass\0", "Ip\0" , "Ch\0", "Netmask\0", "Broadcast\0"};
const static char *wifi_drivers[] = {"8192cu.ko", "8712u.ko.wl167g"};

static void __create_default_adhoc_file(void);
static void __start_dhcp_serv(void);

static const wifi_adhoc_t def_adhoc = DEF_ADHOC_SETTINGS;
wifi_adhoc_t active_adhoc = DEF_ADHOC_SETTINGS;

/**************************************************************************
 name	: __create_adhoc_script_file
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __create_adhoc_script_file(wifi_adhoc_t *adhoc)
{
    uint8_t i = 0;
    FILE *fd = fopen(ADHOC_SCRIPT_FILE, "r");

    if (adhoc->is_enabled) {
    	INFOL(logi,"Adhoc ENABLED\n");
        fprintf(fd, "#!/bin/sh\n");
        fprintf(fd, "ifconfig %s down\n", ADHOC_INTERFACE);
        volatile uint8_t indx;
    	for(indx=0 ; indx<ARRAY_MEMBER_COUNT(wifi_drivers);indx++)
    		fprintf(fd, "rmmod /root/18/%s\n", wifi_drivers[indx]);
    	for(indx=0 ; indx<ARRAY_MEMBER_COUNT(wifi_drivers);indx++)
    		fprintf(fd, "insmod /root/18/%s\n", wifi_drivers[indx]);
        fprintf(fd, "sleep 1\n");
        fprintf(fd, "ip link set %s up\n", ADHOC_INTERFACE);
        fprintf(fd, "sleep 1\n");
        fprintf(fd, "ifconfig %s up\n", ADHOC_INTERFACE);
        fprintf(fd, "ifconfig %s %s netmask %s broadcast %s\n", ADHOC_INTERFACE, adhoc->ip, \
        	adhoc->netmask, adhoc->broadcast);
    }
    else {
    	INFOL(logi,"Adhoc DISABLED\n");
        fprintf(fd, "#!/bin/sh\n");
        {
        	char cmd[128];
        	memset(cmd, 0, sizeof(cmd));
        	snprintf(cmd, sizeof(cmd)-1, "ifconfig %s down", WIFI_IFACE_NAME);
        }
        int indx;
    	for(indx=0 ; indx<ARRAY_MEMBER_COUNT(wifi_drivers);indx++)
    		fprintf(fd, "rmmod %s/%s\n", WIFI_DRIVER_FILES_DIR, wifi_drivers[indx]);
    }

    fflush(fd);
    fclose(fd);
}

/**************************************************************************
 name	: adhoc_init
 purpose	: After adhoc script file is created,
 this function only runs only one time
 input	: none
 output	: none
 warning: system config files must be read before code reaches here
 ***************************************************************************/
void adhoc_init(void)
{
	TRACEL(logi, "%s()-> CALLED\n", __FUNCTION__);

    __create_default_adhoc_file();
    adhoc_reinit();
}

static void __start_dhcp_serv(void)
{
	DEBUGL(logi, "%s()-> CALLED\n", __FUNCTION__);
	if (file_exist("/var/lib/dhcp/dhcpd.leases") != FILE_EXIST) {
		system("mkdir -p /var/lib/dhcp");
		system("touch /var/lib/dhcp/dhcpd.leases");
	}
   	{
		char cmd[256];
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd)-1, "%s -cf %s", HOTSPOD_FILES_DIR "/dhcpd", HOTSPOD_FILES_DIR "/dhcpd_client.conf");
		system(cmd);
   	}
}

// hostapd application is used for softAP mode related wifi internals management
void create_hostapd_conf(void) {
	FILE *fd = fopen(HOTSPOD_FILES_DIR "/hostapd.conf", "w+");
	if(NULL == fd) {
		ERRL(logi, "hostapd.conf fopen FAILED:(%s)\n", strerror(errno));
		return;
	}
	fprintf(fd, "\n");
	fprintf(fd, "ctrl_interface=/var/run/hostapd\n");
	fprintf(fd, "ctrl_interface_group=0\n");
	fprintf(fd, "macaddr_acl=0\n");
	fprintf(fd, "auth_algs=3\n");
	fprintf(fd, "ignore_broadcast_ssid=0\n");

	fprintf(fd, "# 802.11n related stuff\n");
	fprintf(fd, "ieee80211n=1\n");
	fprintf(fd, "noscan=1\n");
	fprintf(fd, "ht_capab=[HT40+][SHORT-GI-20][SHORT-GI-40]\n");

	fprintf(fd, "#WPA2 settings\n");
	fprintf(fd, "wpa=3\n");
	fprintf(fd, "wpa_key_mgmt=WPA-PSK\n");
	fprintf(fd, "wpa_pairwise=TKIP\n");
	fprintf(fd, "rsn_pairwise=TKIP\n");

	fprintf(fd, "# CHANGE THE PASSPHRASE\n");
	fprintf(fd, "wpa_passphrase=%s\n", active_adhoc.pass);

	fprintf(fd, "# Most modern wireless drivers in the kernel need driver=nl80211\n");
	fprintf(fd, "#driver=nl80211\n");
	fprintf(fd, "driver=rtl871xdrv\n");
	fprintf(fd, "max_num_sta=8\n");
	fprintf(fd, "beacon_int=100\n");
	fprintf(fd, "wme_enabled=1\n");
	fprintf(fd, "wpa_group_rekey=86400\n");
	fprintf(fd, "device_name=RTL8192CU\n");
	fprintf(fd, "manufacturer=Realtek\n");

	fprintf(fd, "# set proper interface\n");
	fprintf(fd, "interface=%s\n", ADHOC_INTERFACE);
	fprintf(fd, "bridge=lanbr0\n");
	fprintf(fd, "hw_mode=g\n");
	fprintf(fd, "# best channels are 1 6 11 14 (scan networks first to find which slot is free)\n");
	fprintf(fd, "channel=%u\n", active_adhoc.ch);
	fprintf(fd, "# this is the network name\n");
	fprintf(fd, "ssid=%s\n", active_adhoc.ssid);
	fprintf(fd, "\n");

	fflush(fd);
	fclose(fd);
}


/**************************************************************************
 name	: adhoc_reinit
 purpose	: This function is called from the event management system once
 save_adhoc_config function is called. Since running the script
 takes too much time, once a parameter is saved, an event is posted
 to the event management system.
 input	: none
 output	: none
 ***************************************************************************/
void adhoc_reinit(void)
{
    if (read_adhoc_config(&active_adhoc)) {
        if (active_adhoc.is_enabled) {
        	set_wifi_state(FALSE);
           	INFOL(logi, "Wifi HOTSPOD ENABLED\n");
            __create_adhoc_script_file(&active_adhoc);
            system(ADHOC_SCRIPT_FILE);
			surely_msleep(1000);
			{
				create_hostapd_conf();

	        	run_system_command("kill -9 $(pidof hostapd)");
	        	surely_msleep(500);

	        	char cmd[256];
				memset(cmd, 0, sizeof(cmd));
				snprintf(cmd, sizeof(cmd)-1, "%s %s -d -B", HOTSPOD_FILES_DIR "/hostapd", HOTSPOD_FILES_DIR "/hostapd.conf");
				system(cmd);	// Execute hostapd application //
			}
            surely_msleep(1000);

        	system("kill -9 $(pidof dhcpd)");
        	surely_msleep(1000);
        	__start_dhcp_serv();
        	surely_msleep(250);
        } else {
        	INFOL(logi, "Wifi Adhoc DISABLED\n");
        	system("kill -9 $(pidof hostapd)");
        	system("kill -9 $(pidof dhcpd)");
        	system(HOTSPOD_FILES_DIR "/clear_iptables.sh");
        	if(active_adhoc.is_enabled) {
        		active_adhoc.is_enabled = 0;
        		save_adhoc_config(&active_adhoc);
        	}
        }
    }
}

/**************************************************************************
 name	: read_adhoc_config
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t read_adhoc_config(wifi_adhoc_t *adhoc)
{
    uint8_t *line, *conf, *value;
    FILE *file;

    if (file_exist(ADHOC_CONFIG_FILE) == FILE_EXIST) {

    	INFOL(logi, "adhoc file EXIST\n");
        line = (uint8_t *)calloc(sizeof(uint8_t), 100);
        conf = (uint8_t *)calloc(sizeof(uint8_t), 100);
        value = (uint8_t *)alloc(sizeof(uint8_t), 100);

        *adhoc = def_adhoc;

        file = fopen(ADHOC_CONFIG_FILE, "r");
        while (fgets(line, 50, file) != NULL) {
            clear_string_new_line_character(line, 100);
            get_parse_string_value(line, conf, 100, 0, '=', 100);
            INFOL(logi, "line(%s), conf(%s)\n", line, conf);
            if (strcmp(conf, "Enabled") == 0) {
                get_parse_string_value(line, value, strlen(line), 1, '=', 50);
                adhoc->is_enabled = atoi(value);
                INFOL(logi, "New adhoc Enabled(%u) is %s\n", adhoc->is_enabled, adhoc->is_enabled ? "YES" : "NO");
            }
            else if (strcmp(conf, "Ssid") == 0) {
                get_parse_string_value(line, adhoc->ssid, strlen(line), 1, '=', MAX_SSID_NAME_LENGTH);
                DEBUGL(logi, "New adhoc Ssid is %s\n", adhoc->ssid);
            }
            else if (strcmp(conf, "Hidden") == 0) {
                get_parse_string_value(line, value, strlen(line), 1, '=', 50);
                adhoc->is_ssid_hidden = atoi(value);
                DEBUGL(logi, "New adhoc hidden is %s\n", adhoc->is_ssid_hidden ? "YES" : "NO");
            }
            else if (strcmp(conf, "Encryption") == 0) {
                get_parse_string_value(line, value, strlen(line), 1, '=', 50);
                adhoc->encryption_type = atoi(value);
                DEBUGL(logi, "New adhoc encryption_type is %d\n", adhoc->encryption_type);
            }
            else if (strcmp(conf, "Pass") == 0) {
                get_parse_string_value(line, adhoc->pass, strlen(line), 1, '=', MAX_SSID_PASS_LENGTH);
                DEBUGL(logi, "New adhoc pass is %s\n", adhoc->pass);
            }
            else if (strcmp(conf, "Ip") == 0) {
                get_parse_string_value(line, adhoc->ip, strlen(line), 1, '=', IP_LENGTH);
                DEBUGL(logi, "New adhoc ip is %s\n", adhoc->ip);
            }
            else if (strcmp(conf, "Ch") == 0) {
                get_parse_string_value(line, value, strlen(line), 1, '=', 50);
                adhoc->ch = atoi(value);
                DEBUGL(logi, "New adhoc Channel is %u\n", adhoc->ch);
            }
            else if (strcmp(conf, "Netmask") == 0) {
                get_parse_string_value(line, adhoc->netmask, strlen(line), 1, '=', IP_LENGTH);
                DEBUGL(logi, "New adhoc Netmask is %s\n", adhoc->netmask);
            }
            else if (strcmp(conf, "Broadcast") == 0) {
                get_parse_string_value(line, adhoc->broadcast, strlen(line), 1, '=', IP_LENGTH);
                DEBUGL(logi, "New adhoc Broadcast is %s\n", adhoc->broadcast);
            }

            memset(line, 0, 100);
            memset(conf, 0, 100);
            memset(value, 0, 100);
        }
        free(line);
        free(conf);
        free(value);

        fclose(file);

        return 1;
    }

    return 0;
}

/**************************************************************************
 name	: create_default_adhoc_file
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __create_default_adhoc_file(void)
{
    wifi_adhoc_t *adhoc = NULL;
    if (file_exist(ADHOC_CONFIG_FILE) == FILE_NOT_EXIST) {
        adhoc = (wifi_adhoc_t *)calloc(1, sizeof(wifi_adhoc_t));
        if(NULL != adhoc) {
			*adhoc = def_adhoc;
			save_adhoc_config(adhoc);
			free(adhoc);
        }
        else {
        	ERRL(logi, "malloc FAILED:(%s)\n", strerror(errno));
        }
    }
}

/**************************************************************************
 name	: save_adhoc_config
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void save_adhoc_config(const wifi_adhoc_t *adhoc_p)
{
    uint8_t i = 0;
    FILE *fd = fopen(ADHOC_CONFIG_FILE, "w+");

    INFOL(logi, "adhoc; enable(%u) %s\n", adhoc_p->is_enabled, adhoc_p->ip);
    /* WIFI ADHOC CONFIGURATIONS */
    fprintf(fd, "%s=%d\n", ADHOC_CONFIGURATION[i++], adhoc_p->is_enabled);
    fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], adhoc_p->ssid);
    fprintf(fd, "%s=%d\n", ADHOC_CONFIGURATION[i++], adhoc_p->is_ssid_hidden);
    fprintf(fd, "%s=%d\n", ADHOC_CONFIGURATION[i++], adhoc_p->encryption_type);
    fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], adhoc_p->pass);
    if (check_ipaddress_valid(adhoc_p->ip))
        fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], adhoc_p->ip);
    else {
    	ERRL(logi, "adhoc_ip(%s) NOT VALID\n", adhoc_p->ip);
    	fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], "192.168.42.1");
    }
    fprintf(fd, "%s=%u\n", ADHOC_CONFIGURATION[i++], adhoc_p->ch);
    if (check_ipaddress_valid(adhoc_p->netmask))
        fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], adhoc_p->netmask);
    else {
    	ERRL(logi, "adhoc_netmask(%s) NOT VALID\n", adhoc_p->netmask);
        fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], "255.255.255.0");
    }
    if (check_ipaddress_valid(adhoc_p->broadcast))
        fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], adhoc_p->broadcast);
    else {
    	ERRL(logi, "adhoc_broadcast(%s) NOT VALID\n", adhoc_p->broadcast);
        fprintf(fd, "%s=%s\n", ADHOC_CONFIGURATION[i++], "192.168.42.255");
    }

    fclose(fd);
}
