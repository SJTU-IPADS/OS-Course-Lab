#include <yaml.h>
#include <pthread.h>
#include <sys/stat.h>
#include "fsm.h"
#include "mount_disk_fs.h"

#define STR_DEVICE      "device"
#define STR_MOUNT_POINT "mount_point"

#define DISKFS_CONFIG_FILE "/disk_fs.yaml"
#define MOUNT_POINT_SIZE   128
#define DEVICE_SIZE        128
#define max(a, b)          ((a) > (b) ? (a) : (b))

void mount_disk_fs(cap_t server_cap)
{
        yaml_parser_t parser;
        yaml_event_t event;
        int key_size = max(strlen(STR_DEVICE), strlen(STR_MOUNT_POINT));
        char key[key_size + 1];
        char device[DEVICE_SIZE + 1] = {0};
        char mount_point[MOUNT_POINT_SIZE + 1] = {0};
        FILE *fh;
        fsm_server_cap = server_cap;

        fh = fopen(DISKFS_CONFIG_FILE, "r");
        if (!fh) {
                error("%s: Failed to open file %s\n",
                      __func__,
                      DISKFS_CONFIG_FILE);
                return;
        }

        /* Initialize parser */
        if (!yaml_parser_initialize(&parser))
                error("%s: Failed to initialize parser!\n", __func__);

        /* Set input file */
        yaml_parser_set_input_file(&parser, fh);

        /* Start parsing */
        while (1) {
                if (!yaml_parser_parse(&parser, &event)) {
                        error("%s:Parser error %d\n", __func__, parser.error);
                        goto out;
                }

                /* Check if this is an alias, scalar, sequence, mapping, or
                 * other node */
                if (event.type == YAML_SCALAR_EVENT) {
                        strncpy(key, (char *)event.data.scalar.value, key_size);

                        if (!yaml_parser_parse(&parser, &event)) {
                                error("%s:Parser error %d\n",
                                      __func__,
                                      parser.error);
                                goto out;
                        }

                        if (event.type == YAML_SCALAR_EVENT) {
                                if (strcmp(key, STR_DEVICE) == 0) {
                                        strncpy(device,
                                                (char *)event.data.scalar.value,
                                                DEVICE_SIZE);
                                } else if (strcmp(key, STR_MOUNT_POINT) == 0) {
                                        strncpy(mount_point,
                                                (char *)event.data.scalar.value,
                                                MOUNT_POINT_SIZE);
                                }
                        }

                        if (strlen(device) && strlen(mount_point)) {
                                printf("Mounting %s at %s\n",
                                       device,
                                       mount_point);
                                mkdir(mount_point, 0777);
                                fsm_mount_fs(device, mount_point);
                                device[0] = '\0';
                                mount_point[0] = '\0';
                        }
                }

                /* The application is responsible for destroying the event
                 * object */
                if (event.type == YAML_STREAM_END_EVENT) {
                        yaml_event_delete(&event);
                        break;
                }
                yaml_event_delete(&event);
        }

out:
        /* Cleanup */
        yaml_parser_delete(&parser);
        fclose(fh);
        return;
}
