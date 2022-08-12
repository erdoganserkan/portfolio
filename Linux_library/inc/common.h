#ifndef COMMON_H_
#define COMMON_H_

#include <common.h>
#include <stdlib.h>
#include <signal.h>
#include <m_list.h>
#include <log.h>

typedef void (*__my_sighandler_t)(int, unsigned char);
typedef struct pkt_s {
	uint8_t hdr[10];
	uint8_t core[100];
} pkt_t;

/*	daemonize: make daemon the running program
 *	@return: -errno on failure
 *	@return: 0 on child process, process can go on working
 *	@return: pid of child on main process, so it can exit successfully
 */
int32_t daemonize(int8_t *path, int8_t *file);
uint32_t file_size(const int8_t *file);
uint32_t get_time(void);
unsigned long int get_msec_time(void);
uint8_t compare_strings(uint8_t *s1, uint8_t *s2, uint32_t n);
int8_t *trim(int8_t *s);
int32_t free_memory(void);
void run_exec_command(const int8_t *command);
void free_chunk_list(m_list_t *list);
void free_fec_list(m_list_t *list);
uint32_t get_parse_int_value(int8_t *buffer, uint32_t size, uint8_t idx,
    int8_t seperator);
int32_t get_key_string(char* src, char* key, int32_t size, char* seperator );
uint32_t get_parse_string_value(int8_t *source, int8_t *dest, uint32_t size,
    uint8_t idx, int8_t seperator, uint32_t maxValue);
void clear_string_new_line_character(uint8_t *line, uint32_t len);
uint32_t get_current_path(uint8_t *path);
uint32_t list_files_in_path(const int8_t *path, int8_t *all_filename,
    uint32_t file_count);
uint32_t find_path_of_file(const char *path, char *send_file, char *ret_path);
void change_existing_file_name(char *mount_folder, char *filename);
void get_date(uint8_t *ret);
static inline int32_t get_max_value(int32_t a, int32_t b) {
    if (a > b) return a;
    else return b;
}
static inline int32_t get_min_value(int32_t a, int32_t b) {
    if (a < b) return a;
    else return b;
}
pid_t get_process_id(const int8_t* name);
pid_t get_process_id2(const int8_t* name);
void set_timetolive(int32_t sock, uint32_t ttl);
void kill_pid(pid_t pid);
procps_status_t * procps_scan(int save_user_arg0, uint8_t *ret, char *process,
    int32_t *proc_pid);
void generate_random_string(uint8_t *data, uint8_t len);
void create_fifo_file(char *file);
int32_t is_writeable(int32_t fd);
int32_t is_readable(int32_t fd);
uint8_t *video_res_name(uint8_t res);
void log_array(mj_log_type logger, uint8_t log_level, uint8_t *src, uint16_t len, uint8_t hex_log);
int32_t create_and_exec_script(mj_log_type log_instance, char *script_name, char *script_cmd, uint8_t execute, uint8_t remove_on_exit);
int is_dir_exist (char *d);

extern void write_ts_to_file(uint8_t *data, uint32_t len, fsock_t *fsock_p, char *working_path);
extern void __close_video_file(fsock_t *fsock_p);
extern void __open_video_file(fsock_t *fsock_p, char *working_path, char *ID);

#endif /* COMMON_H_ */
