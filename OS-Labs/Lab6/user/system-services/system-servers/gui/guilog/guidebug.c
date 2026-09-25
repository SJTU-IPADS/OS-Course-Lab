#define _GNU_SOURCE
#include <stdio.h>
#include "guilog.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
struct bitmap_hack {
        int w; // width
        int h; // height
        int depth; // depth
        int pitch; // pitch
        cap_t pmo_cap;
        union {
                u8 *raw_image; // Pointer to raw byte format array
                RGB565 *ptr_rgb565; // Pointer to RGB565 format array
                RGB *ptr_rgb; // Pointer to RGB format array
                RGBA *ptr_rgba; // Pointer to RGBA format array
        };
};
int png_dir_exist = 0;
unsigned char debug_buffer[1920 * 1080 * 4];
extern int savebmp(FILE *fp, int width, int height, void *raw_data);
void *cpy_to_disk(void *args);

void save_window(PBITMAP bitmap, PRGN region)
{
        if (png_dir_exist == 0) {
                if (mkdir("/png", S_IRUSR | S_IRGRP | S_IROTH) < 0) {
                        printf("mkdir error\n");
                        return;
                }
                png_dir_exist = 1;
        }
        static int i = 0;
        if (i > 20) {
                if (i == 21) {
                        i++;
                        pthread_t pid;
                        // pthread_create(&pid, NULL, cpy_to_disk, NULL);
                }
                return;
        }
        if (bitmap == NULL) {
                printf("bitmap is NULL\n");
                return;
        }
        struct bitmap_hack *bitmap_hack = bitmap;
        char *raw_image = bitmap_hack->raw_image;
        if (raw_image == NULL) {
                printf("raw image is NULL\n");
                return;
        }
        int width = bitmap_hack->w;
        int height = bitmap_hack->h;
        memcpy(debug_buffer, raw_image, width * height * 4);
        printf("raw data = %p, width = %d, height = %d\n",
               raw_image,
               width,
               height);
        char filename[100];
        sprintf(filename, "/png/frame-%d.bmp", i);
        FILE *f = fopen(filename, "w");
        if (f == NULL) {
                printf("open failed\n");
                return;
        }
        printf("debug buffer is %p\n", debug_buffer);
        savebmp(f, width, height, debug_buffer);
        printf("%s saved\n", filename);
        fclose(f);
        RECT *rect = get_region_rect(region, GRR_FIRST);
        if (rect != NULL) {
                for (; rect != NULL; rect = get_region_rect(region, GRR_NEXT)) {
                        for (int i = rect->t; i < rect->b; i++) {
                                memset(debug_buffer + i * width * 4
                                               + (rect->l) * 4,
                                       0,
                                       (rect->r - rect->l) * 4);
                        }
                }
        }
        sprintf(filename, "/png/frame-%d-upd.bmp", i);
        f = fopen(filename, "wb");
        if (f == NULL) {
                printf("open failed\n");
                return;
        }
        savebmp(f, width, height, debug_buffer);
        printf("%s saved\n", filename);
        fclose(f);
        i++;
}

unsigned char cpy_buffer[1920 * 1080 * 4];
int copy_file(char *dst, char *src)
{
        int fd;
        int size, exact_op;
        struct stat fileStat;
        fd = open(src, O_RDONLY | O_DIRECT | O_SYNC);
        if (fd <= 0) {
                printf("[%s][open]: open failed\n", src);
                return -1;
        }
        if (fstat(fd, &fileStat) < 0) {
                printf("[%s][fstat]: fstat failed\n", src);
        }
        size = fileStat.st_size;
        if ((exact_op = read(fd, cpy_buffer, size)) != size) {
                printf("[%s][read]: should %ld, exact %ld\n",
                       src,
                       size,
                       exact_op);
                return -1;
        }
        close(fd);

        fd = open(dst, O_CREAT | O_SYNC | O_DIRECT | O_WRONLY);
        if (fd <= 0) {
                printf("[%s][open]: open failed\n", dst);
                return -1;
        }
        printf("size = %d, and cpy buffer is %p\n", size, cpy_buffer);
        if ((exact_op = write(fd, cpy_buffer, size)) != size) {
                printf("[%s][write]: should %ld, exact %ld\n",
                       dst,
                       size,
                       exact_op);
                return -1;
        }
        close(fd);

        printf("file %s copied!\n", dst);
}

void *cpy_to_disk(void *args)
{
        int ret;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(3, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        usys_yield();
        for (;;)
                ;
        int i = 0;
        char src[50];
        char dst[50];
        printf("start to copy to disk\n");
        for (i = 20; i > 0; i--) {
                sprintf(src, "/png/frame-%d.bmp", i);
                sprintf(dst, "/user/png/frame-%d.bmp", i);
                copy_file(dst, src);

                sprintf(src, "/png/frame-%d-upd.bmp", i);
                sprintf(dst, "/user/png/frame-%d-upd.bmp", i);
                copy_file(dst, src);
        }
}