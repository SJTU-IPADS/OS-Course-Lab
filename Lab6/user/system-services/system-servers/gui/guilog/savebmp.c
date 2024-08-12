#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#pragma pack(push, 1)

typedef struct {
        unsigned short type;
        unsigned int size;
        unsigned short reserved1;
        unsigned short reserved2;
        unsigned int offset;
} BMPFileHeader;

typedef struct {
        unsigned int size;
        int width;
        int height;
        unsigned short planes;
        unsigned short bitsPerPixel;
        unsigned int compression;
        unsigned int imageSize;
        int xPixelsPerMeter;
        int yPixelsPerMeter;
        unsigned int colorsUsed;
        unsigned int colorsImportant;
} BMPInfoHeader;

typedef struct {
        unsigned char blue;
        unsigned char green;
        unsigned char red;
        unsigned char alpha;
} RGBA;

int savebmp(FILE *fp, int width, int height, void *raw_data)
{
        // Initialize imageData with RGBA image information

        BMPFileHeader fileHeader = {
                .type = 0x4D42,
                .size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)
                        + width * height * 4,
                .reserved1 = 0,
                .reserved2 = 0,
                .offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)};

        BMPInfoHeader infoHeader = {.size = sizeof(BMPInfoHeader),
                                    .width = width,
                                    .height = height,
                                    .planes = 1,
                                    .bitsPerPixel = 32,
                                    .compression = 0,
                                    .imageSize = width * height * 4,
                                    .xPixelsPerMeter = 0,
                                    .yPixelsPerMeter = 0,
                                    .colorsUsed = 0,
                                    .colorsImportant = 0};

        if (!fp) {
                printf("Failed to open file\n");
                return 1;
        }
        fwrite(&fileHeader, sizeof(BMPFileHeader), 1, fp);
        fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, fp);
        fwrite(raw_data, 1, width * height * 4, fp);
        return 0;
}