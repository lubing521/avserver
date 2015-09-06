
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/dir.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "sd_config.h"

#define DEBUG   1

#define DIR_LEN 128

#define SD_MOUNTED  0x01

#define SD_PATH     "/mnt/mmc0"

#define __E(fmt, args...)  fprintf(stderr, "Error: "fmt, ## args);

static int gSdStatus = 0x00;

const char *get_sdpath()
{
	return SD_PATH;
}

int GetSdStatus()
{
	return gSdStatus;
}

int CheckSDCard()
{
	int ret = 0;
	char cmd[128];

	sprintf(cmd, "umount %s", SD_PATH);
	system(cmd);
	sprintf(cmd, "mount -t vfat /dev/mmcblk0p1 %s", SD_PATH);
	system(cmd);

	gSdStatus |= SD_MOUNTED;

	return ret;
}

long long GetCardfreeSpace(char *path)
{
	long long int free;//Kbyte
	struct statfs disk_statfs;

	if(statfs(path, &disk_statfs)>=0){
		printf("f_bsize = %d, f_bfree = %d\n", disk_statfs.f_bsize, disk_statfs.f_bfree);
		free = (((long long int)disk_statfs.f_bsize * (long long int)disk_statfs.f_bfree)/(long long int)1024);
	}

	return free;
}

int SDUnmount()
{
	int ret = 0;

	ret = system("umount /mnt/mmc");
	if(ret == 0){
		gSdStatus &= ~(SD_MOUNTED);
	}
	sleep(1);
	return ret;
}

int SdFormat()
{
	int ret = 0;
	char cmd[128];

	sprintf(cmd, "umount %s", SD_PATH);
	system(cmd);

	if(system("mkdosfs /dev/mmcblk0p1")&&system("mkdosfs /dev/mmcblk0")){
		__E("SD format fail.\n");
		ret = -1;
	}
	if(system("mount -t vfat /dev/mmcblk0p1 /mnt/mmc")&&
		system("mount -t vfat /dev/mmcblk0 /mnt/mmc")){
			__E("SD mount fail.\n");
			ret = -1;
	}

	return ret;
}



/*
 *mkdir for avi files,
 *return:
 *  success: 0;
*/
char* mkdir_t(int deepth)
{
    int i = 0;
    int depth = 2;
    int ret = 0;
    time_t timep;
    struct tm *p;
	char *dir_str = (char *)malloc(DIR_LEN * sizeof(char));

	if (dir_str == NULL)
		return NULL;

    time(&timep);
    p = localtime(&timep);
    for (i = 0; i < depth; i++) {
        if (i == 0)
            sprintf(dir_str, SD_PATH"/%04d%02d%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);
        else
            sprintf(dir_str, SD_PATH"/%04d%02d%02d/%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour);

        if (access(dir_str, F_OK) != 0) {
            /* create dir */
            ret = mkdir(dir_str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (ret)
                printf("mkdir_t: mkdir %s fail, errno = %d\n", dir_str, errno);
        }
    }

    return dir_str;
}

/*
 *compare the direct time.
 *if dir_1 >= dir_2 return 1;
 *if dir_1 < dir_2 return 0;
 *if fail return -1;
*/
int Is_gt(char *dir_1, char *dir_2)
{
    int d_dir_1 = 0;
    int d_dir_2 = 0;

    if (dir_2 == NULL)
        return -1;

    if (dir_1 == NULL)
        return 0;

    d_dir_1 = atoi(dir_1);
    d_dir_2 = atoi(dir_2);
    if ((d_dir_1 / 10000) < (d_dir_2 / 10000))
        return 1;

    if ((d_dir_1 / 100) < (d_dir_2 / 100))
        return 1;

    if (d_dir_1 < d_dir_2)
        return 1;

    return 0;
}

int IS_DIR(const char* path)
{
         struct stat st;
         lstat(path, &st);
         return S_ISDIR(st.st_mode);
}

int IS_FILE(const char* path)
{
         struct stat st;
         lstat(path, &st);
         return S_ISREG(st.st_mode);
}

int is_special_dir(const char *path)
{
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}
/*
 *return:
 *  -1: dir is null, fail
 *   0: SD_PATH fail
 *   1: found a day directory, but it is empty
 *   2: found a oldest hour directory
 *   3: found a oldest hour directory, but it is the last one
*/
int find_oldest_dir_from_sdroot(char *dir)
{
    int i = 0;
    int fnd = 0;
    DIR *pdir = NULL;
    struct dirent *pdirent = NULL;
    int first_dir = 1;

    if (dir == NULL)
        return -1;

    pdir = opendir(SD_PATH);
    if (pdir) {
        while (pdirent = readdir(pdir)) {
#if DEBUG
            printf("find dir: %s\n", pdirent->d_name);
#endif
            /* not a directory */
            if (!IS_DIR(pdirent->d_name))
                continue;

            /* if the first char is not '2' or the secend char is not '0',
                the direct is not my target, so continue*/
            if (pdirent->d_name[0] != '2' || pdirent->d_name[0] != '0')
                continue;

            if (first_dir) {
                first_dir = 0;
                strcpy(dir, pdirent->d_name);
                continue;
            }

            if (!Is_gt(dir, pdirent->d_name))
                strcpy(dir, pdirent->d_name);

            fnd = 1;
        }
        for (i = 0; (i < 24) && (fnd == 1); i++) {
            char tmp_dir[DIR_LEN];
            /* eg: SD_PATH/20150711/00 */
            sprintf(tmp_dir, "%s/%s/%02d", SD_PATH, dir, i);
            if (access(tmp_dir, F_OK) == 0) {
                strcpy(dir, tmp_dir);
                fnd = 2 + (i == 23);
                break;
            }
        }
#if DEBUG
        printf("find oldest dir: %s\n", dir);
#endif
    }

    return fnd;
}

int del_dir(char *dir_path)
{
    DIR *pdir = NULL;
    struct dirent *pdirent = NULL;

    if (dir_path == NULL)
        return -1;

    if (IS_FILE(dir_path)) {
        remove(dir_path);
        return 0;
    }

    pdir = opendir(dir_path);
    while (pdirent = readdir(pdir)) {
        char tmp_dir[DIR_LEN];
        if (is_special_dir(pdirent->d_name))
            continue;

        sprintf(tmp_dir, "%s/%s", dir_path, pdirent->d_name);
        del_dir(tmp_dir);
        rmdir(tmp_dir);
    }
    return 0;
}



#if 0
int main(int argc, char* argv[])
{
	int ret = 0;
	char dir_file[128];
	
	printf("argc = %d, argv[1] = %c\n", argc, *argv[1]);
	if (*argv[1] == 'm') {
		mkdir_t(2);	
		mkdir_t(2);
	} else {
		ret = find_oldest_dir_from_sdroot(dir_file);
		if (ret) {
			printf("find oldest dir : %s\n", dir_file);
			del_dir(dir_file);
		} else {
			printf("not find dir\n");	
		}
	}
	
	return 0;
}

#endif
