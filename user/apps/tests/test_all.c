/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <ctype.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <inttypes.h>

#include "test_tools.h"
#include <time.h>

#define MAX_TESTS 256
#define MAX_LINE  255
#define MAX_ARGS  16

int tokenize_config_line(char *line, char **tokens)
{
        const char s[2] = " ";
        char *token;
        int i = 0;
        int length = strlen(line);
        if (line[length - 1] == '\n') {
                line[strlen(line) - 1] = '\0';
        }
        token = strtok(line, s);
        while (token != NULL) {
                tokens[i++] = token;
                token = strtok(NULL, s);
        }
        tokens[i + 1] = NULL;
        return i;
}

struct timespec diff_timespec(const struct timespec *time1,
                              const struct timespec *time0)
{
        struct timespec diff = {.tv_sec = time1->tv_sec - time0->tv_sec,
                                .tv_nsec = time1->tv_nsec - time0->tv_nsec};
        if (diff.tv_nsec < 0) {
                diff.tv_nsec += 1000000000;
                diff.tv_sec--;
        }
        return diff;
}

/*
 * prepare args from the line content read from config file in the args
 * returns the number of args
 * remember to delete the \n at the end of each line first
 */
int get_process_args(char *line, char **args)
{
        const char s[2] = " ";
        char *token;
        int i = 0;
        int length = strlen(line);
        if (line[length - 1] == '\n') {
                line[strlen(line) - 1] = '\0';
        }
        token = strtok(line, s);
        while (token != NULL) {
                token = strtok(NULL, s);
                args[i++] = token;
        }
        return i - 1;
}

bool str_is_blank_or_comment(const char *s)
{
        /* Comment line */
        if (*s == '#')
                return true;

        while (*s != '\0') {
                if (!isspace((unsigned char)*s))
                        return false;
                s++;
        }
        return true;
}

/* Hope to mitigate the out-of-order issue of print. */
void flush_output(void)
{
        printf("\r\n");
        sleep(1);
        printf("\r\n");
}

/*
 *  create each test process and wait until them exit
 *  check the exitcode returned from waitpid()
 */
void run_tests(char *config_file_name)
{
        FILE *file;
        char *tokens[MAX_ARGS];
        char **argv;
        char line_content[MAX_LINE];
        struct new_process_caps client_process_caps[MAX_TESTS];
        int status[MAX_TESTS];
        int faillist[MAX_TESTS];
        int pid, argc;
        int token_cnt;
        int test_num = 0;
        int succ_num = 0;
        int fail_num = 0;
        int i = 1;
        int expected_ret;
        struct timespec start, end, duration;

        printf("Read config file: %s\n", config_file_name);
        file = fopen(config_file_name, "r");
        chcore_assert(file, "failed to open the config file!\n");

        while (fgets(line_content, MAX_LINE, file)) {
                if (str_is_blank_or_comment(line_content)) {
                        continue;
                }
                test_num++;
        }
        rewind(file);

        while (fgets(line_content, MAX_LINE, file)) {
                if (str_is_blank_or_comment(line_content)) {
                        continue;
                }
                status[i] = -1;
                // config line: <timeout> <expected_ret> <...>
                token_cnt = tokenize_config_line(line_content, tokens);
                expected_ret = atoi(tokens[1]);

                argc = token_cnt - 2;
                argv = &tokens[2];
                clock_gettime(CLOCK_REALTIME, &start);
                pid = create_process(argc, argv, &client_process_caps[i]);
                if (pid >= 0) {
                        chcore_waitpid(pid, &status[i], 0, 0);
                        clock_gettime(CLOCK_REALTIME, &end);
                } else {
                        printf("failed to create process %s !\n", argv[0]);
                        printf(COLOR_RED "[%d/%d]: %s failed!" COLOR_DEFAULT "\n",
                               i,
                               test_num,
                               argv[0]);
                        i++;
                        continue;
                }

                duration = diff_timespec(&end, &start);

                flush_output();
                if (status[i] != expected_ret) {
                        printf(COLOR_RED "[%d/%d]: %s failed!" COLOR_DEFAULT "\n",
                               i,
                               test_num,
                               argv[0]);
                        printf(COLOR_RED
                               "Get exitstatus:%d, take_time: %" PRId64 "s:%ldns" COLOR_DEFAULT
                               "\n",
                               status[i],
                               duration.tv_sec,
                               duration.tv_nsec);
                        faillist[fail_num++] = i;
                } else {
                        succ_num++;
                        printf(COLOR_RED
                               "[%d/%d]: %s passed! take_time: %" PRId64 "s:%ldns" COLOR_DEFAULT
                               "\n",
                               i,
                               test_num,
                               argv[0],
                               duration.tv_sec,
                               duration.tv_nsec);
                }
                i++;
        }

        if (succ_num == test_num) {
                printf(COLOR_RED "[%d/%d] tests passed" COLOR_DEFAULT "\n",
                       succ_num,
                       test_num);
                printf(COLOR_RED "All tests passed!" COLOR_DEFAULT "\n");
        } else {
                printf(COLOR_RED "[%d/%d] tests passed" COLOR_DEFAULT "\n",
                       succ_num,
                       test_num);
                for (i = 0; i < fail_num; i++) {
                        printf(COLOR_RED "test%d failed" COLOR_DEFAULT "\n", faillist[i]);
                }
        }
        fclose(file);
}

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <test_config_file>\n", argv[0]);
                exit(-1);
        }

        run_tests(argv[1]);
        return 0;
}
