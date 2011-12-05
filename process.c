#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "process.h"

static int is_number (char * string)
{
	while (*string) {
		if (!isdigit (*string))
			return 0;
		string++;
	}
	return 1;
}

static int getprogname(char *pid, char *cmd, int len)
{
	char filename[256];
	snprintf(filename, sizeof(filename), "/proc/%s/cmdline", pid);

	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
        return -1;
	}

	int l = read(fd, cmd, len);

	close (fd);
    cmd[l] = '\0';

	return 0;
}

static u32 str2u32 (char *ptr)
{
	u32 ret = 0;

	while ((*ptr >= '0') && (*ptr <= '9')) {
		ret *= 10;
		ret += *ptr - '0';
		ptr++;
	}

	return ret;
}

static void _parse_pid(hashm_t *pinfos, char *stPid)
{
    pid_t pid = atoi(stPid);
    char  cmd[256] = {0};
	if (getprogname(stPid, cmd, 256) != 0) {
        return;
    }

    char path[256] = {0};
	snprintf(path, sizeof(path)-1, "/proc/%s/fd", stPid);

	DIR * dir = opendir(path);
	if (!dir) {
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(dir))) {
		if (entry->d_type != DT_LNK) {
			continue;
        }

        char fullname[256] = {0};
		snprintf (fullname, sizeof(fullname)-1, "%s/%s", path, entry->d_name);

		char linkname [255];
		int len = readlink(fullname, linkname, sizeof(linkname)-1);
		if (len == -1) {
			continue;
		}
		linkname[len] = '\0';

	    if (strncmp(linkname, "socket:[", 8) == 0) {
	    	u32 inode = str2u32(linkname+8);

            pinfo_t *pinfo = NULL;
            jn_hashm_add(pinfos, (void*)&inode, sizeof(inode), (void**) &pinfo);
            if (pinfo == NULL) {
                continue;
            }
           
            memset(pinfo, 0x00, sizeof(pinfo_t)); 
            pinfo->pid = pid;
            strcpy(pinfo->cmd, cmd);
	    }
	}

	closedir(dir);
}

int pinfo_init(hashm_t *pinfos)
{
    return jn_hashm_init(pinfos, 20, sizeof(pinfo_t), 1024);
}

int pinfo_refille(hashm_t *pinfos)
{
	DIR *proc = opendir ("/proc");

	if (proc == 0) {
        return -1;
	}

    jn_hashm_del_all(pinfos);

	struct dirent *entry;
	while ((entry = readdir(proc)) != NULL) {
		if (entry->d_type != DT_DIR) {
            continue;
        }

		if (!is_number(entry->d_name)) {
            continue;
        }

		_parse_pid(pinfos, entry->d_name);
	}

	closedir(proc);

    return 0;
}

int process_init(hashm_t *pros)
{
    return jn_hashm_init(pros, 20, sizeof(process_t), 1024);
}

void process_show(hashm_t *pros, cb_t cb)
{
    if (cb) {
        cb(NULL, 0, 0);
    }

    hashm_iter_t iter;

    process_t *process;
    time_t now = time(NULL);

    jn_hashm_first(pros, &iter, (void**)&process);
    while (process) {
        if (cb) {
            cb(process, now, 1);
        }
        process->old_send_byte = process->send_byte;
        process->old_recv_byte = process->recv_byte;
        process->calc_time = time(NULL);
        jn_hashm_next(pros, &iter, (void**)&process);
    }

    if (cb) {
        cb(NULL, 0, 2);
    }

}

int process_timeout(hashm_t *mhash)
{
    time_t now = time(NULL);
    process_t *process = NULL;

    hashm_iter_t iter;
    jn_hashm_first(mhash, &iter, (void**)&process);
    while (process != NULL) {
        if (now - process->refresh_time >= PROCESS_TIMEOUT) {
            jn_hashm_del(mhash, (void*)&process->pinfo.pid, sizeof(pid_t));
        }

        jn_hashm_next(mhash, &iter, (void**)&process);
    }

    return 0;
}
