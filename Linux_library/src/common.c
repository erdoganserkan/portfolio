#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <common.h>

const char *wifi_drivers[4] = {"8192cu.ko", "8712u.ko", "8712u.ko.wl167g", "8188eu.ko" /*tl-wn725n*/};

static __my_sighandler_t  __handler;

void log_array(mj_log_type logger, uint8_t log_level, uint8_t *src, uint16_t len, uint8_t hex_log)
{
    char sbuf[32];
    char lbuf[4096];
	volatile uint16_t indx;
	uint16_t max_log = (len > sizeof(lbuf) ? sizeof(lbuf) : len);

    memset(lbuf, 0, sizeof(lbuf));
    for (indx = 0; indx < max_log; indx++) {
    	memset(sbuf, 0, sizeof(sbuf));
    	if(hex_log)
    		snprintf(sbuf, sizeof(sbuf), "%02X ", src[indx]);
    	else
    		snprintf(sbuf, sizeof(sbuf), "%02u ", src[indx]);
    	strcat(lbuf, sbuf);
    }
    strcat(lbuf, "\n");

	switch(log_level) {
		case LOG_ERR:
			ERRL2(logger, "%s", lbuf);
			break;
		case LOG_WARN:
			WARNL2(logger, "%s", lbuf);
			break;
		case LOG_INFO:
			INFOL2(logger, "%s", lbuf);
			break;
		case LOG_DBG:
			DEBUGL2(logger, "%s", lbuf);
			break;
		case LOG_TRACE:
			TRACEL2(logger, "%s", lbuf);
			break;
		default:
			break;
	};
}

/**************************************************************************
 name	: daemonize
 purpose	:
 input	: path : working path
 output	:
 *************************************************************************/
int daemonize(int8_t *path, int8_t *file)
{
    int8_t lock_file[100];
#ifndef DEBUG
    /*	Our process ID and Session ID	*/
    pid_t pid, sid;

    /*	Fork off the parent process	*/
    pid = fork();

    /*	If we got a good PID, then we can exit the parent process.	*/
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (pid < 0) {
        ERRL(logi, "%s\n", strerror(errno));
        exit(EXIT_SUCCESS);
    }

    memset(lock_file, 0, 100);
    sprintf(lock_file, "%s/%s.lock", path, file);

    /*	Change the file mode mask	*/
    umask(0);

    /*	Create a new SID for the child process	*/
    sid = setsid();
    if (sid < 0) {
//		ERRL(logi, "%s\n", strerror(errno));
        return -errno;
    }

    /*	Change the current working directory	*/
    if (chdir(path) < 0) {
        return -errno;
    }
    /*	Close out the standard file descriptors	*/
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

#endif

    return SUCCESS;
}

/**************************************************************************
 name	: file_exist
 purpose	: Check whether file exists
 input	:
 output	:
 *************************************************************************/
int8_t file_exist(const int8_t *file) /*If file not exist, return -1*/
{
    struct stat st;
    int stResult;
    stResult = stat(file, &st);

    return stResult;
}

/**************************************************************************
 name	: file_size
 purpose	: Return size of file
 input	:
 output	:
 *************************************************************************/
uint32_t file_size(const int8_t *file)
{
    struct stat st;
    int stResult;
    stResult = stat(file, &st);

    if (stResult == FILE_EXIST)
        return st.st_size;
    else
        return 0;
}

/**************************************************************************
 name	: get_time
 purpose	: return system time as second
 input	:
 output	:
 *************************************************************************/
uint32_t get_time(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
    }
    return ts.tv_sec;
}

/**************************************************************************
 name	: get_msec_time
 purpose	: return system time as millisecond
 input	:
 output	:
 *************************************************************************/
unsigned long int get_msec_time(void)
{
    struct timespec ts;
    unsigned long int msec = 0;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
    }
    msec = ((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
    return msec;
}

/**************************************************************************
 name	: compare_strings
 purpose	: Compare whether two strings are equal
 input	:
 output	:
 *************************************************************************/
uint8_t compare_strings(uint8_t *s1, uint8_t *s2, uint32_t n)
{
    for (; n && (*s1 == *s2); s1++, s2++, n--);
    if (!n)
        return 1;

    return 0;
}

/**************************************************************************
 name	: trim
 purpose	:
 input	:
 output	:
 *************************************************************************/
int8_t *trim(int8_t *s)
{
    int8_t *t;

    while (isspace(*s)) {
        t = s;
        for (; *t != '\0'; t++) {
            *t = *(t + 1);
        }
        *t = '\0';
    }
    t = s + strlen(s) - 1;
    for (; isspace(*t); t--)
        *t = '\0';
    return s;
}

/**************************************************************************
 name	: free_memory
 purpose	: return system free memory
 input	:
 output	:
 *************************************************************************/
int32_t free_memory(void)
{
    struct sysinfo info;
    sysinfo(&info);

    /* Kernels prior to 2.4.x will return info.mem_unit==0, so cope... */
    if (info.mem_unit == 0) {
        info.mem_unit = 1;
    }
    if (info.mem_unit == 1) {
        info.mem_unit = 1024;

        /* TODO:  Make all this stuff not overflow when mem >= 4 Gib */
        info.totalram /= info.mem_unit;
        info.freeram /= info.mem_unit;
#ifndef __uClinux__
        info.totalswap /= info.mem_unit;
        info.freeswap /= info.mem_unit;
#endif
        info.sharedram /= info.mem_unit;
        info.bufferram /= info.mem_unit;
    } else {
        info.mem_unit /= 1024;
        /* TODO:  Make all this stuff not overflow when mem >= 4 Gib */
        info.totalram *= info.mem_unit;
        info.freeram *= info.mem_unit;
#ifndef __uClinux__
        info.totalswap *= info.mem_unit;
        info.freeswap *= info.mem_unit;
#endif
        info.sharedram *= info.mem_unit;
        info.bufferram *= info.mem_unit;
    }

    return info.freeram;
}

/**************************************************************************
 name	: signal_ignore
 purpose	:
 input	:
 output	:
 *************************************************************************/
void signal_ignore(void)
{
    int i;
    for (i = SIGHUP; i < SIGWINCH; i++)
        signal(i, SIG_IGN);
}
/**************************************************************************
 name	: __sig_handler
 purpose	:
 input	:
 output	:
 *************************************************************************/
static void __sig_handler(int signum)
{
	INFOL(logi, "Received signal(%d: %s)\n", signum, strsignal(signum));

	signal(signum, SIG_IGN);	// Ignore this signal until our job finishes //
    switch (signum) {
		case SIGTRAP:
		case SIGHUP:
		case SIGINT:
		case SIGQUIT:
		case SIGABRT:
		case SIGKILL:
		case SIGSEGV:
		case SIGTERM:
			INFOL(logi, "Handled Signal (%d) %s\n", signum, strsignal(signum));
			__handler(signum, 0);
			break;
		case SIGCHLD:{
			signal(signum, SIG_IGN);	// Ignore this signal until our job finishes //
			pid_t pidc = waitpid(-1, NULL, WNOHANG);
			if(0 < pidc) {
				fprintf(stdout, "child pid(%d) DIED\n", pidc);
				DEBUGL(logi, "child pid(%d) DIED\n", pidc);
			} else if(0 == pidc) {
				fprintf(stdout, "there is NO DIED child to wait\n");
				INFOL(logi, "there is NO DIED child to wait\n");
			} else if(0 > pidc) {
				fprintf(stdout, "waitpid() FAILED:(%s)\n", strerror(errno));
				ERRL(logi, "waitpid() FAILED:(%s)\n", strerror(errno));
			}
		}
		break;
		case SIGUSR2:
			fprintf(stdout, "SIGUSR2 received, exiting\n");
			__handler(signum, 0);
			break;
		default:
			ERRL(logi, "UNHandled Signal (%d) %s\n",
				signum, strsignal(signum));
			break;
    }
	signal(signum, __sig_handler);	// Restore back signal handler for current signal //
}

/**************************************************************************
 name	: signal_init
 purpose	:
 input	:
 output	:
 *************************************************************************/
void signal_init(__my_sighandler_t  __func)
{
    int i;
    __handler = __func;

    for (i = SIGHUP; i < SIGWINCH; i++)
        signal(i, __sig_handler);
}

/**************************************************************************
 name	: system
 purpose	:
 input	:
 output	:
 *************************************************************************/
void system(const int8_t *command)
{
    int32_t ret;
    signal(SIGCHLD, SIG_IGN);
    ret = system(command);
    signal(SIGCHLD, __sig_handler);
}

/**************************************************************************
 name	: run_exec_command
 purpose	:
 input	:
 output	:
 *************************************************************************/
void run_exec_command(const int8_t *command)
{
    int32_t ret;
    signal(SIGCHLD, SIG_IGN);
    ret = system(command);
    signal(SIGCHLD, __sig_handler);
}

/**************************************************************************
 name   : run_exec_real_command
 purpose    :
 input  :
 output :
 *************************************************************************/
int32_t run_exec_real_command(const char *path, char *const argv[], uint8_t do_chdir)
{
    int32_t ret = 0;
    pid_t pid;

    if(-1 != file_exist(path)) {
	    signal(SIGCHLD, SIG_IGN);

	    pid = fork ();
	    if (0 == pid) { // Child process //
            if(do_chdir == 1 ) {
                char *ptr = NULL;
                char path2[128];
                memset(path2, 0, sizeof(path2));
                strncpy(path2, path, sizeof(path2));
                if(NULL != (ptr = strstr(path2, argv[0]))) {
                    *(ptr-1)='\0';	// The char before app_name is "/" character and so we delete it to get directory path //
                    if(-1 == chdir(path2)) {
                        fprintf (stderr, "chdir(%s) FAILED\n", path);
                        ret = -1;
                    }
                } else {
                    ret = -1;
                    fprintf (stderr, "path(%s) NOT includes application name(%s)\n", path, argv[0]);
                }
            }
            if(-1 != ret) {
                ret = execvp (path, argv);
                if (ret == -1) {
                    fprintf (stderr, "execvp(%s) FAILED\n", path);
                }
            }
	    }
	    else if (-1 == pid) {
		ret = pid;
		fprintf (stderr, "fork() FAILED\n");
	    }

	    signal(SIGCHLD, __sig_handler);
    } else {
	ret = -1;
        fprintf (stderr, "path(%s) NOT EXIST\n", path);
    }

    return ret;
}

/**************************************************************************
 name	: free_pkt_list
 purpose	:
 input	:
 output	:
 *************************************************************************/
void free_pkt_list(m_list_t *list)
{
    m_list_t *temp = NULL;
    struct send_elem_s *elem;

    while (!m_list_empty(list)) {
        temp = m_list_next_elem(list);
        elem = m_list_elem_data(temp);
        if (elem == NULL) {
            ERRL(logi, "Very Big problem\n");
            m_list_del(temp);
            continue;
        }
        if (elem->pkt != NULL) {
            ctech_free(elem->pkt);
            elem->pkt = NULL;
        }
        if (elem != NULL) {
            ctech_free(elem);
            elem = NULL;
        }
        m_list_del(temp);
    }
}

/**************************************************************************
 name	: free_fec_list
 purpose	:
 input	:
 output	:
 *************************************************************************/
void free_fec_list(m_list_t *list)
{
    m_list_t *temp = NULL;
    pkt_t *pkt;
    while (!m_list_empty(list)) {
        temp = m_list_next_elem(list);
        pkt = m_list_elem_data(temp);
        if (pkt != NULL) {
            ctech_free(pkt);
            pkt = NULL;
        }
        m_list_del(temp);
    }
}

/**************************************************************************
 name	: get_parse_int_value
 purpose	: Returns the desired value in the string
 input	: none
 output	: none
 ***************************************************************************/
uint32_t get_parse_int_value(int8_t *buffer, uint32_t size, uint8_t idx,
    int8_t seperator)
{
    uint32_t start, end, counter, i, result;

    start = end = counter = 0;
    for (i = 0; i < size; i++) {
        if (buffer[i] == seperator)
            counter++;

        if ((counter > idx) || (i == (size - 1))) {
            end = i;
            break;
        }
        if (buffer[i] == seperator)
            start = i + 1;
    }

    for (i = start; i < end; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9')
            break;
        else
            start++;
    }

    for (i = end - 1; i >= start; i--) {
        if (buffer[i] >= '0' && buffer[i] <= '9')
            break;
        else
            end--;
    }

    if (start > end)
        end = start;

    result = atoi(&buffer[start]);
    return result;
}

// seperator must like "=", ".", "_", can not be like "ab, "abc", "abcd"
// because seperator casted to int8_t* and searched in text as single byte.
int32_t get_key_string(char* src, char* key, int32_t size, char* seperator )
{

	int32_t retval = -1;
	int32_t sep_index = 0;

	int8_t* INT_sep = (int8_t*)seperator;
	int8_t* INT_src = (int8_t*)src;

	for ( sep_index = 0; sep_index < size; sep_index++  )
	{
		if ( INT_src[sep_index] == INT_sep[0] )
		{
			break;
		}
	}

	if ( sep_index < size )
	{
		retval = 0;
		memcpy(key, src, sep_index);
		key[sep_index] = '\0';
	}

	return retval;
}

/**************************************************************************
 name	: get_parse_string_value
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint32_t get_parse_string_value(int8_t *source, int8_t *dest, uint32_t size,
    uint8_t idx, int8_t seperator, uint32_t maxValue)
{
    uint32_t start = 0;
    uint32_t end = 0;
    uint8_t counter = 0;
    uint32_t i;

    for (i = 0; i < size; i++) {
        if (source[i] == seperator) {
            counter++;
        }

        if (counter > idx) {
            end = i;
            break;
        }
        if (i == (size - 1)) {
            end = i + 1;
            break;
        }

        if (source[i] == seperator)
            start = i + 1;
    }

    if (maxValue) {
        if ((end - start) > maxValue) {
            memcpy(dest, &source[start], maxValue);
            return maxValue;
        }
        else {
            memcpy(dest, &source[start], end - start);
            return end - start;
        }
    }
    else {
        memcpy(dest, &source[start], end - start);
        return end - start;
    }
}

/**************************************************************************
 name	: clear_string_new_line_character
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void clear_string_new_line_character(uint8_t *line, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        if (line[i] == '\n') {
            if ((i > 0) &&
                (line[i - 1] == '\r')) {
                line[i - 1] = 0;
            }
            line[i] = 0;
            break;
        }
    }

    for (i = 0; i < len; i++) {
        if (line[i] == '\r') {
            line[i] = 0;
            break;
        }
    }
}

/**************************************************************************
 name	: get_current_path
 purpose	: It returns path of running application
 input	:
 output	: returns length of path
 *************************************************************************/
uint32_t get_current_path(uint8_t *path)
{
    uint32_t i, idx;
    int8_t buf[PATH_MAX + 1];
    memset(buf, 0, PATH_MAX + 1);
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
        return 0;

    idx = 0;
    for (i = 0; i < PATH_MAX + 1; i++)
        if (buf[i] == '/')
            idx = i;
    if (idx > 0)
        memcpy(path, buf, idx);

    return idx - 1;
}

/**************************************************************************
 name	: list_files_in_path
 purpose	: list file names which are in the path
 input	:
 output	: return total file count in the path
 *************************************************************************/
uint32_t list_files_in_path(const int8_t *path, int8_t *all_filename,
    uint32_t file_count)
{
    struct dirent *direntp = NULL;
    struct stat fstat;
    DIR *dir;
    int32_t path_len;
    char *full_name;

    full_name = ctech_malloc(sizeof(char) * MAX_PATH_LIMIT);
    path_len = strlen(path);

    dir = opendir(path);
    if (dir != NULL) {
        while ((direntp = readdir(dir)) != NULL) {

            memset(full_name, 0, MAX_PATH_LIMIT);
            strcpy(full_name, path);
            if (full_name[path_len - 1] != '/')
                strcat(full_name, "/");
            strcat(full_name, direntp->d_name);
            DEBUGL(logi, "PATH : %s Full Name : %s\n", path,
                full_name);

            if ((strcmp(direntp->d_name, ".") == 0) ||
                (strcmp(direntp->d_name, "..") == 0) ||
                (direntp->d_name[0] == '.'))
                continue;

            if (stat(full_name, &fstat) < 0)
                continue;

            if (S_ISDIR(fstat.st_mode)) {
                DEBUGL(logi, "Full Name : %s is a directory\n",
                    full_name);
                //file_count= list_files_in_path(full_name, all_filename/*+(file_count*(MAX_FILENAME_LENGTH+1))*/, file_count);
                //INFOL(logi,"fip-4\n");
            }
            else {
                if (file_count < MAX_FILE_LIMIT) {
                    memcpy(
                        all_filename + file_count * (MAX_FILENAME_LENGTH + 1),
                        direntp->d_name, MAX_FILENAME_LENGTH);
                    file_count++;
                }
                else
                    break;
            }
        }
    }
    closedir(dir);

    ctech_free(full_name);
    return file_count;
}

/**************************************************************************
 name	: find_path_of_file
 purpose	:
 input	:
 output	:
 *************************************************************************/
uint32_t find_path_of_file(const char *path_tmp, char *send_file,
    char *ret_path)
{
    struct dirent *direntp = NULL;
    DIR *dir;
    struct stat fstat;
    int path_len;
    uint32_t file_count = 0;
    uint32_t file_found = 0;
    char *full_name;
    full_name = ctech_malloc(sizeof(char) * MAX_PATH_LIMIT);

    path_len = strlen(path_tmp);

    dir = opendir(path_tmp);
    if (dir != NULL) {

        while ((direntp = readdir(dir)) != NULL) {

            memset(full_name, 0, MAX_PATH_LIMIT);
            strcpy(full_name, path_tmp);
            if (full_name[path_len - 1] != '/')
                strcat(full_name, "/");
            strcat(full_name, direntp->d_name);

            if ((strcmp(direntp->d_name, ".") == 0) ||
                (strcmp(direntp->d_name, "..") == 0))
                continue;

            if (stat(full_name, &fstat) < 0)
                continue;
            if (S_ISDIR(fstat.st_mode))
                {
                find_path_of_file(full_name, send_file, ret_path);
            }
            else {
                if (file_count < MAX_FILE_LIMIT) {
                    if (strstr(direntp->d_name, send_file) != NULL) {
                        DEBUGL(logi, " PATH : %s \n", full_name);
                        memcpy(ret_path, full_name, MAX_PATH_LIMIT);
                        file_found = 1;
                        break;
                    }
                    file_count++;
                }
                else
                    break;
            }
        }
    }

    if (dir != NULL)
        closedir(dir);

    ctech_free(full_name);
    return file_found;
}

/**************************************************************************
 name	: change_existing_file_name
 purpose	:
 input	:
 output	:
 *************************************************************************/
void change_existing_file_name(char *mount_folder, char *filename)
{
    int i = 0;
    char filename_new[MAX_FILENAME_LENGTH + 10];
    char filepath_new[250];
    char filepath_old[250];

    snprintf(filepath_old, 250, "%s%s", mount_folder, filename);

    if (file_exist(filepath_old) != FILE_EXIST) {
        DEBUGL(logi, "File copy not needed! %s\n", filename);
    }
    else {

        DEBUGL(logi, "File copy needed! %s\n", filename);
        for (i = 1; i < 20; i++) {

            snprintf(filename_new, MAX_FILENAME_LENGTH + 10, "(copy%d)%s", i,
                filename);
            snprintf(filepath_new, 250, "%s%s", mount_folder, filename_new);

            if (file_exist(filepath_new) != FILE_EXIST) {
                break;
            }
        }
        memcpy(filename, filename_new, MAX_FILENAME_LENGTH + 1);
    }
}

/**************************************************************************
 name	: get_date
 purpose	:
 input	:
 output	:
 *************************************************************************/
void get_date(uint8_t *ret)
{
    struct tm date;
    time_t lt;

    lt = time(&lt);
    localtime_r(&lt, &date);
    asctime_r(&date, ret);
    ret[strlen(ret) - 1] = '\0';
}

/**************************************************************************
 name	: get_process_id
 purpose	:
 input	:
 output	:
 *************************************************************************/
pid_t get_process_id(const int8_t* name)
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                //if (!strcmp(first, name)) {
                //printf("COMPARE: first(%s) name(%s)\n", first, name);
                if (strstr(first, name) != NULL) {	// Search in the only first part (usually application name) of cmdline //
                    //printf("FOUND\n\n");
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }
    }

    closedir(dir);
    return -1;
}

/**************************************************************************
 name	: get_process_id2
 purpose	: returns the process pid whose "name or any of it's parameter" is given by "any_param"
 input	: any_param: application name or one of it's parameters
 output	:
 *************************************************************************/
pid_t get_process_id2(const int8_t* any_param)
{
	TRACEL(logi, , "%s()-> CALLED \n", __FUNCTION__);
    char fname[256];
    char buf[2048];

    uint32_t random = rand();
    char cmd[2048];
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd)-1, "ps aux > /tmp/psaux_%u.txt", random);
    system(cmd);

    // read temporary file that holding "ps aux" command output //
    memset(fname, 0, sizeof(fname));
    snprintf(fname, sizeof(fname)-1, "/tmp/psaux_%u.txt", random);
    FILE *fd = fopen(fname, "r");
    if(NULL == fd){
    	ERRL(logi, "fopen(%s,w) FAILED:(%s)\n", fname, strerror(errno));
    	return -1;
    }

    while(!feof(fd)) {
    	if(NULL == fgets(buf, sizeof(buf)-1, fd))
    		break;
        TRACEL(logi, , "cmdline(%s)\n", buf);
        if (strstr(buf, any_param) != NULL) {	// Search parameter in all of the cmdline //
            fclose(fd);	// close file we don't need it  any more //
            remove(fname);	// remove it //

            // Get pid of process via parsing 2nd parameter of line //
            pid_t pid = -1;
            sscanf (buf,"%*s %d", &pid);
            TRACEL(logi, , "FOUND: any_param(%s), pid(%d)\n", any_param, pid);
            return pid;
        }
    }

    TRACEL(logi, , "any_param(%s) NOT FOUND in ACTIVE PROCESS LIST\n", any_param);
    fclose(fd);	// close file we don't need it  any more //
    remove(fname);	// remove it //

    return -1;
}

/**************************************************************************
 name	: __exit_wd
 purpose	:
 input	:
 output	: none
 *************************************************************************/
void kill_pid(pid_t pid)
{
    int ret = 0;
    ret = kill(pid, SIGKILL);

    if (ret == 0) {
        ERRL(logi, "pid[%d] is killed successfully\n", pid);
    } else {
        ERRL(logi, "Can not kill pid:%d\n", pid);
    }
}

/**************************************************************************
 name	: read_to_buf
 purpose	:
 input	:
 output	: none
 *************************************************************************/
static int __read_to_buf(const int8_t *filename, void *buf)
{
    int32_t fd;
    ssize_t ret;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return -1;
    ret = read(fd, buf, PROCPS_BUFSIZE);
    close(fd);
    return ret;
}

/**************************************************************************
 name	: procps_scan
 purpose	:
 input	:
 output	: none
 *************************************************************************/
procps_status_t * procps_scan(int save_user_arg0, uint8_t *ret, char *process,
    int32_t *proc_pid)
{
    static DIR *dir;
    struct dirent *entry;
    static procps_status_t ret_status;
    int8_t *name;
    int32_t n;
    int8_t status[32];
    int8_t *status_tail;
    int8_t buf[PROCPS_BUFSIZE];
    procps_status_t curstatus;
    int32_t pid;
    long tasknice;
    struct stat sb;

    if (!dir) {
        dir = opendir("/proc");
    }
    for (;;) {
        if ((entry = readdir(dir)) == NULL) {
            closedir(dir);
            dir = 0;
            return 0;
        }
        name = entry->d_name;
        if (!(*name >= '0' && *name <= '9'))
            continue;

        memset(&curstatus, 0, sizeof(procps_status_t));
        pid = atoi(name);
        curstatus.pid = pid;

        status_tail = status + sprintf(status, "/proc/%d", pid);
        if (stat(status, &sb))
            continue;
        //		bb_getpwuid(curstatus.user, sb.st_uid, sizeof(curstatus.user));

        strcpy(status_tail, "/stat");
        //	printf("status=%s\r\n",status);
        n = __read_to_buf(status, buf);
        if (n < 0)
            continue;
        name = strrchr(buf, ')'); /* split into "PID (cmd" and "<rest>" */
        if (name == 0 || name[1] != ' ')
            continue;
        *name = 0;
        sscanf(buf, "%*s (%15c", curstatus.short_cmd);
        n = sscanf(name + 2,
            "%c %d "
                "%*s %*s %*s %*s " /* pgrp, session, tty, tpgid */
                "%*s %*s %*s %*s %*s " /* flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
#ifdef CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE
            "%lu %lu "
#else
            "%*s %*s "
#endif
            "%*s %*s %*s " /* cutime, cstime, priority */
            "%ld "
            "%*s %*s %*s " /* timeout, it_real_value, start_time */
            "%*s " /* vsize */
            "%ld",
            curstatus.state, &curstatus.ppid,
            #ifdef CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE
            &curstatus.utime, &curstatus.stime,
#endif
            &tasknice,
            &curstatus.rss);
#ifdef CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE
        if(n != 6)
#else
        if (n != 4)
#endif
            continue;

        if (curstatus.rss == 0 && curstatus.state[0] != 'Z')
            curstatus.state[1] = 'W';
        else
            curstatus.state[1] = ' ';
        if (tasknice < 0)
            curstatus.state[2] = '<';
        else if (tasknice > 0)
            curstatus.state[2] = 'N';
        else
            curstatus.state[2] = ' ';

#ifdef PAGE_SHIFT
        curstatus.rss <<= (PAGE_SHIFT - 10); /* 2**10 = 1kb */
#else
        curstatus.rss *= (getpagesize() >> 10); /* 2**10 = 1kb */
#endif

        if (save_user_arg0) {
            strcpy(status_tail, "/cmdline");
            n = __read_to_buf(status, buf);
            if (n > 0) {
                if (buf[n - 1] == '\n')
                    buf[--n] = 0;
                name = buf;
                while (n) {
                    if (((uint8_t) * name) < ' ')
                        *name = ' ';
                    name++;
                    n--;
                }
                *name = 0;
                if (buf[0])
                    curstatus.cmd = strdup(buf);

                if (strstr(curstatus.cmd, process) != NULL) {
                    DEBUGL(logi, "proc:%s\n", process);
                    *proc_pid = pid;
                    if (curstatus.state[0] == 'Z') {
                        ERRL(logi, "%s S:%s P:%d\r\n", curstatus.cmd,
                            curstatus.state, curstatus.pid);
                        kill_pid(pid);
                    }
                    else
                        *ret = 1;
                }
                /* if NULL it work true also */
            }
        }
        if (curstatus.cmd != NULL)
            ctech_free(curstatus.cmd);
        curstatus.cmd = NULL;
        return memcpy(&ret_status, &curstatus, sizeof(procps_status_t));
    }
}

/**************************************************************************
 name	: generate_random_string
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void generate_random_string(uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t c;
    FILE *file = fopen("/dev/urandom", "r");
    for (i = 0; i < len;) {
        size_t s = fread(&c, 1, 1, file);
        if ((c >= 48 && c <= 57) ||
            (c >= 65 && c <= 90) ||
            (c >= 97 && c <= 122)) {
            data[i++] = c;
        }
    }
    fclose(file);
}

/**************************************************************************
 name   : create_fifo_file
 purpose    :
 input  : none
 output : none
 ***************************************************************************/
void create_fifo_file(char *file)
{
    int32_t fifo;
    if (access((char *)file, F_OK) == -1) {
        fifo = mkfifo((char *)file, 0777);
        if (fifo == -1)
            ERRL(logi, "File Error:%s, err:%s\n", file,
                strerror(errno));
    }
}

/**************************************************************************
 name   : is_writeable
 purpose    : Check whether fifo is full
 input  : none
 output : none
 *************************************************************************/
int32_t is_writeable(int32_t fd)
{
    struct pollfd p;
    int32_t ret;

    p.fd = fd;
    p.events = POLLOUT;

    ret = poll(&p, 1, 0);

    if (ret < 0) {
        TRACEL(logi, , "poll failed\n");
    }

    return p.revents & POLLOUT;
}


/**************************************************************************
 name   : is_writeable
 purpose    : Check whether fifo is full
 input  : none
 output : none
 *************************************************************************/
int32_t is_readable(int32_t fd)
{
    struct pollfd p;
    int32_t ret;

    p.fd = fd;
    p.events = POLLIN | POLLERR;

    ret = poll(&p, 1, 0);

    if (ret < 0) {
        TRACEL(logi, , "poll failed\n");
    }

    if (p.revents & POLLERR)
        printf("poll err\n");

    return p.revents & (POLLIN);
}


/**************************************************************************
 name   : inform_wifi_state
 purpose    : informs "check_wifi.sh script about last wifi state"
 input  : none
 output : none
 ***************************************************************************/
void inform_wifi_pause_state(uint8_t state)
{
    INFOL(logi, "new wifi PAUSE state(%s)\n", state ? "EANBLED" : "DISABLED");
    if(state) {
        // create a file to indicate wpa_supplicant stopped by gui //
        FILE *fd = fopen("/tmp/wpa_stopped_by_gui", "w+");  // it is for check_wifi.sh script //
        if(NULL != fd) {
            DEBUGL(logi, "\"/tmp/wpa_stopped_by_gui\" file creation OK(%s)\n", strerror(errno));
            fsync(fileno(fd));
            fclose(fd);
        } else {
            ERRL(logi, "\"/tmp/wpa_stopped_by_gui\" file creation FAILED(%s)\n", strerror(errno));
        }
    } else {
        remove("/tmp/wpa_stopped_by_gui");  // it is for check_wifi.sh script //
        DEBUGL(logi, "\"/tmp/wpa_stopped_by_gui\" file DELETION DONE\n");
    }
}

/**************************************************************************
 name   : inform_wifi_state
 purpose    : informs "check_wifi.sh script about last wifi state"
 input  : none
 output : none
 ***************************************************************************/
void load_unload_wifi_drivers(uint8_t state)
{
	extern const char *wifi_drivers[];
	char cmd[128];
	volatile uint8_t indx;
    if(state) {
    	for(indx=0 ; indx<ARRAY_MEMBER_COUNT(wifi_drivers);indx++) {
    		memset(cmd, 0, sizeof(cmd));
    		snprintf(cmd, sizeof(cmd)-1, "insmod /root/18/%s", wifi_drivers[indx]);
            system(cmd);
    	}
    }
    else {
    	for(indx=0 ; indx<ARRAY_MEMBER_COUNT(wifi_drivers);indx++) {
    		memset(cmd, 0, sizeof(cmd));
    		snprintf(cmd, sizeof(cmd)-1, "rmmod /root/18/%s", wifi_drivers[indx]);
            system(cmd);
    	}
    }
}
	
// because of system call "usleep" is returning before desired time elapses due to SIGNALs use this function // 	
#define SURELY_USLEEP_STEP_USLEEP    (10000)
inline void surely_usleep(uint32_t u_sleep)
{
	volatile uint32_t indx;
	uint32_t diff;
	if(SURELY_USLEEP_STEP_USLEEP > u_sleep) {
		usleep(u_sleep);
		return;
	}
	for(indx = (u_sleep/SURELY_USLEEP_STEP_USLEEP) ; indx ; indx--)
	        usleep(SURELY_USLEEP_STEP_USLEEP);
	if(u_sleep != (diff = ((u_sleep/SURELY_USLEEP_STEP_USLEEP)*SURELY_USLEEP_STEP_USLEEP)))
		usleep(u_sleep - diff);
}

inline void surely_msleep(uint32_t m_sleep) {
    surely_usleep(m_sleep*1000UL);
}

uint8_t set_non_blocking(int fd, uint8_t nblocking)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        ERRL(logi, "fcntl(F_GETFL) FAILED:(%s)\n", strerror(errno));
        return FALSE;
    }
    flags = nblocking ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK));
    if (fcntl(fd, F_SETFL, flags) != 0) {
    	ERRL(logi, "fcntl(F_SETFL) FAILED:(%s)\n", strerror(errno));
        return  FALSE;
    }

    return TRUE;
}

uint8_t is_ipv4_valid(const char *s)
{
    volatile int indx;
    int len = strlen(s);

    if (len < 7 || len > 15)
        return FALSE;

    char tail[16];
    tail[0] = 0;

    unsigned int d[4];
    int c = sscanf(s, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);

    if (c != 4 || tail[0])
        return FALSE;

    for (indx = 0; indx < 4; indx++)
        if (d[indx] > 255)
            return FALSE;

    return TRUE;
}

int32_t create_and_exec_script(mj_log_type log_instance, char *script_fpath, char *script_cmd, uint8_t execute, uint8_t remove_on_exit)
{
	pid_t newp = -1;
	if((NULL == script_cmd) || (NULL == script_fpath)) {
		ERRL2(log_instance, "NULL parameter\n");
		return -1;
	}

	FILE *fd = fopen(script_fpath, "w+");
	if(NULL == fd) {
		ERRL2(log_instance, "script_file(%s) open FAILED:(%s)\n", script_fpath, strerror(errno));
		return -1;
	} else {
		fprintf(fd, "#!/bin/sh\n\n");
		fprintf(fd, "%s", script_cmd);
		fprintf(fd, "\n");
		if(remove_on_exit)
			fprintf(fd, "rm %s\n", script_fpath);
		fprintf(fd, "\n");
		fflush(fd);
		fclose(fd);
		{
			char cmd[128];
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd)-1, "chmod +x %s\n", script_fpath);
			if(0 > system(cmd)) {
				ERRL2(log_instance, "system() FAIED:(%s)\n", strerror(errno));
			}
		}
		{
			char cmd[128];
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd)-1, "chmod 755 %s\n", script_fpath);
			if(0 > system(cmd)) {
				ERRL2(log_instance, "system() FAIED:(%s)\n", strerror(errno));
			}
		}
		if(execute){
	        // start new streaming process //
	        newp = fork();
	        if (newp == 0) { /* child process */
	            volatile int indx;
	            // ignore all signals, ow after child killed rest of script doesn't work //
	            for (indx = SIGHUP; indx < SIGWINCH; indx++)
	                signal(indx, SIG_IGN);
#if(1)
	            execl("/bin/sh", "/bin/sh", script_fpath, NULL);
	            exit(EXIT_FAILURE);
#else
				if(0 > system(cmd)) {
					ERRL(logi, "system() FAILED:(%s)\n", strerror(errno));
				}
				if(remove_on_exit)
					remove(script_fpath);
				exit(SIGTERM);
#endif
	        }
	        else if(0 > newp) {
	            ERRL2(log_instance, "fork() FAILED: %s\n", strerror(errno));
	            return -1;
	        }
		}
	}
	return newp;
}


/* test that dir exists (0 success, -1 does not exist, -2 not dir) */
int is_dir_exist (char *d)
{
    DIR *dirptr;

    if (access ( d, F_OK ) != -1 ) {
        // file exists
        if ((dirptr = opendir (d)) != NULL) {
            closedir (dirptr);
        } else {
            return -2; /* d exists, but not dir */
        }
    } else {
        return -1;     /* d does not exist */
    }

    return 0;
}

/**************************************************************************
 name	: __close_video_file
 purpose	: close video file
 input	: none
 output	: none
 *************************************************************************/
void __close_video_file(fsock_t *fsock_p)
{
    if (fsock_p->fp) {
    	fflush(fsock_p->fp);
    	fclose(fsock_p->fp);
    }
    fsock_p->fp = NULL;
}


/**************************************************************************
 name	: __write_ts_to_file
 purpose	: process pkt that came from client
 input	: rch -> pointer of pkt
 sockfd -> socket number of client
 output	: none
 *************************************************************************/
void write_ts_to_file(uint8_t *data, uint32_t len, fsock_t *fsock_p, char *working_path)
{
    if (fsock_p->fp == NULL)
        return;

    int real_len = fwrite(data, 1, len, fsock_p->fp);
    if(0 >= len)
    	return;

    fsock_p->ts_length += real_len;

    if (fsock_p->ts_length > MAX_TS_LENGTH) {
        uint8_t file_name[MAX_SOCKET_FILE_SIZE];
        fclose(fsock_p->fp);
        fsock_p->ts_idx++;
        fsock_p->ts_length = 0;
        memset(file_name, 0, MAX_SOCKET_FILE_SIZE);
        snprintf(file_name, MAX_SOCKET_FILE_SIZE, "%s_%.2d.ts",
            fsock_p->file_name,
            fsock_p->ts_idx);

        fsock_p->fp = fopen(file_name, "wb");

        // create symbolic link to active file for ffmpeg-decklink play //
        {
        	char cmd[1024];
        	memset(cmd, 0, sizeof(cmd));
        	snprintf(cmd, sizeof(cmd)-1, "ln -s %s %s/videos/active.ts -f", file_name, working_path);
			if(0 > system(cmd)) {
				ERRL(logi, "system() FAIED:(%s)\n", strerror(errno));
			}
        	//system("sync");
        	// after ffmpeg-decklink script finishes previous file it will start with new file again //
        }
    }
}

/**************************************************************************
 name	: __open_video_file
 purpose	: Open new video file
 input	: none
 output	: none
 *************************************************************************/
void __open_video_file(fsock_t *fsock_p, char *working_path, char *ID)
{
    struct tm *bt;
    time_t curtime;
    uint8_t file_name[MAX_SOCKET_FILE_SIZE];
    uint8_t file_dir[MAX_SOCKET_FILE_SIZE*2];

    if (fsock_p->fp)
        return;

    memset(file_name, 0, MAX_SOCKET_FILE_SIZE);
    memset(fsock_p->file_name, 0, MAX_SOCKET_FILE_SIZE);

    curtime = time(NULL);
    bt = localtime(&curtime);

    // check if "dev_id" directory is exist under working_path, create if not exist //
	#if (defined STORE_TS_IN_DIFF_DIR_FOR_DEVID)
    {
        memset(file_dir, 0, sizeof(file_dir));
        snprintf(file_dir, sizeof(file_dir), "%s/videos/%s", working_path, ID);
        if (file_exist(file_dir) == FILE_NOT_EXIST) {
            if (mkdir(file_dir, 0777) == -1)
                ERRL(logi, "%s can not be created\n", file_dir);
        }
    }

    //:TODO: check device-id based max files size
    //:TODO: check maximum allowed video size

    snprintf(fsock_p->file_name, sizeof(fsock_p->file_name),
        "%s/device_%s_%.2d%.2d_%.2d%.2d%.2d", file_dir, ID,
        (bt->tm_mon + 1), bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec);
	#else
		#error "FTP server needs seperate directories for each device-id"
		snprintf(fsock_p->file_name, sizeof(fsock_p->file_name),
			"%s/videos/device_%s_%.2d%.2d_%.2d%.2d%.2d",
			_general_config.working_path, fsock_p->session.id,
			(bt->tm_mon + 1), bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec);
    #endif

    snprintf(file_name, sizeof(file_name), "%s_%.2d.ts", fsock_p->file_name, fsock_p->ts_idx);
    fsock_p->fp = fopen(file_name, "wb");

    // give full access rights for FTP client to be able to delete unnecessary files //
    {
    	char temp[256];
    	memset(temp, 0, sizeof(temp));
    	snprintf(temp, sizeof(temp)-1, "chmod -R 777 %s\n", file_dir);
    }

    // create symbolic link for ffmpeg-decklink play //
    {
    	char cmd[1024];
    	memset(cmd, 0, sizeof(cmd));
    	snprintf(cmd, sizeof(cmd)-1, "ln -s %s %s/videos/active.ts -f", file_name, working_path);
		if(0 > system(cmd)) {
			ERRL(logi, "system() FAIED:(%s)\n", strerror(errno));
		}
    	//system("sync");
    }
}
