/* regions.c: print where the code, the heap and the stack of this process
 * are, and what each region may be used for.
 * /proc/self/maps has one line per region of the process's memory:
 *     address range, permissions, offset, device, inode, file
 * The permissions are r (read), w (write), x (execute). This program keeps
 * the regions that may be executed, the heap and the stack, and prints the
 * address range, the permissions and the file name of each.
 * Build: gcc -Og -o regions regions.c
 */
#include <stdio.h>
#include <string.h>

int main(void) {
    char line[512], range[64], perm[8], name[256];
    FILE *f = fopen("/proc/self/maps", "r");
    if (f == NULL) return 1;
    while (fgets(line, sizeof line, f) != NULL) {
        name[0] = '\0';
        if (sscanf(line, "%63s %7s %*s %*s %*s %255s", range, perm, name) < 2)
            continue;
        if (strncmp(perm, "r-x", 3) == 0 || strcmp(name, "[heap]") == 0
                || strcmp(name, "[stack]") == 0) {
            const char *base = strrchr(name, '/');
            printf("%s %s %s\n", range, perm, base ? base + 1 : name);
        }
    }
    fclose(f);
    return 0;
}
