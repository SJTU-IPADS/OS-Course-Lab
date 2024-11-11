#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "copy_folder.h"
#include "guilog/guilog.h"

#define TIMEOUT 10

static void copy_file(const char *src, const char *dst)
{
        int fd_src = open(src, O_RDONLY);
        int fd_dst = open(dst, O_WRONLY | O_CREAT, 0666);
        char buffer[4096];
        ssize_t bytes;
        while ((bytes = read(fd_src, buffer, sizeof(buffer))) > 0) {
                write(fd_dst, buffer, bytes);
        }
        close(fd_src);
        close(fd_dst);
}

void copy_directory(const char *src, const char *dst)
{
        DIR *dir;
        time_t start = time(NULL);
        static int already_printed = 0;
        if (!already_printed) {
                GUI_LOG("Copying cursor files from %s to %s\n", src, dst);
        }
retry:
        dir = opendir(src);
        if (dir == NULL) {
                if (time(NULL) - start > TIMEOUT) {
                        GUI_WARN("Timeout while reading cursor files.\n");
                        goto err_out;
                }
                sleep(1);
                goto retry;
        }
        if (!already_printed) {
                GUI_LOG("Start copying cursor files...\n");
                already_printed = 1;
        }
        mkdir(dst, 0755);
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0
                    || strcmp(entry->d_name, "..") == 0) {
                        continue;
                }
                char src_path[1024];
                snprintf(src_path,
                         sizeof(src_path),
                         "%s/%s",
                         src,
                         entry->d_name);
                char dst_path[1024];
                snprintf(dst_path,
                         sizeof(dst_path),
                         "%s/%s",
                         dst,
                         entry->d_name);
                struct stat st;
                stat(src_path, &st);
                if (S_ISDIR(st.st_mode)) {
                        mkdir(dst_path, 0755);
                        copy_directory(src_path, dst_path);
                } else if (S_ISREG(st.st_mode)) {
                        copy_file(src_path, dst_path);
                }
        }
        closedir(dir);
        return;
err_out:
        GUI_WARN("Cannot open directory %s while reading cursor files.\n",
               src);
        return;
}
