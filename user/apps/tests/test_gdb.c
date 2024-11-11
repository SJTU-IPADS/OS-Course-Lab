/* This is a test program for gdb to trace. */
int main(int argc, char *argv[])
{
        int i = 0;
        while (1) {
                i = i + 2;
                if (i > 1000000) {
                        i = 0;
                }
        }
        return 0;
}