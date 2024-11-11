/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2021 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "ttest.h"
#include "test.h"

/**
 * Print program usage
 *
 * \param[in]  prog_name  Name of program.
 */
static void usage(const char *prog_name)
{
	fprintf(stderr, "Usage: %s [-q|-v|-d]\n", prog_name);
}

/**
 * Main entry point from OS.
 *
 * \param[in]  argc  Argument count.
 * \param[in]  argv  Vector of string arguments.
 * \return Program return code.
 */
int main(int argc, char *argv[])
{
	bool pass = true;
	bool quiet = false;
	ttest_report_ctx_t rc;
	const char *test_list = NULL;
	cyaml_log_fn_t log_fn = cyaml_log;
	cyaml_log_t log_level = CYAML_LOG_ERROR;
	enum {
		ARG_PROG_NAME,
		ARG_VERBOSE,
		ARG_TEST_LIST,
		ARG__COUNT,
	};

	if (argc > ARG__COUNT) {
		usage(argv[ARG_PROG_NAME]);
		return EXIT_FAILURE;

	}

	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-q") == 0) ||
		    (strcmp(argv[i], "--quiet") == 0)) {
			quiet = true;
			log_fn = NULL;
		} else if ((strcmp(argv[i], "-v") == 0) ||
		           (strcmp(argv[i], "--verbose") == 0)) {
			log_level = CYAML_LOG_INFO;
		} else if ((strcmp(argv[i], "-d") == 0) ||
		           (strcmp(argv[i], "--debug") == 0)) {
			log_level = CYAML_LOG_DEBUG;
		} else {
			test_list = argv[i];
		}
	}

	rc = ttest_init(test_list, quiet);

	pass &= utf8_tests(&rc, log_level, log_fn);
	pass &= util_tests(&rc, log_level, log_fn);
	pass &= free_tests(&rc, log_level, log_fn);
	pass &= load_tests(&rc, log_level, log_fn);
	pass &= errs_tests(&rc, log_level, log_fn);
	pass &= file_tests(&rc, log_level, log_fn);
	pass &= save_tests(&rc, log_level, log_fn);
	pass &= copy_tests(&rc, log_level, log_fn);

	ttest_report(&rc);

	return (pass) ? EXIT_SUCCESS : EXIT_FAILURE;
}
