/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018-2021 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <cyaml/cyaml.h>

#include "ttest.h"
#include "test.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

/** Macro to squash unused variable compiler warnings. */
#define UNUSED(_x) ((void)(_x))

/** Helper macro to count bytes of YAML input data. */
#define YAML_LEN(_y) (sizeof(_y) - 1)

/**
 * Unit test context data.
 */
typedef struct test_data {
	char **buffer;
	cyaml_data_t **data;
	cyaml_data_t **copy;
	unsigned *seq_count;
	const struct cyaml_config *config;
	const struct cyaml_schema_value *schema;
} test_data_t;

/**
 * Common clean up function to free data loaded by tests.
 *
 * \param[in]  data  The unit test context data.
 */
static void cyaml_cleanup(void *data)
{
	struct test_data *td = data;
	unsigned seq_count = 0;

	if (td->seq_count != NULL) {
		seq_count = *(td->seq_count);
	}

	if (td->config->mem_fn != NULL && td->buffer != NULL) {
		td->config->mem_fn(td->config->mem_ctx, *(td->buffer), 0);
	}

	if (td->data != NULL) {
		cyaml_free(td->config, td->schema, *(td->data), seq_count);
	}

	if (td->copy != NULL) {
		cyaml_free(td->config, td->schema, *(td->copy), seq_count);
	}
}

/**
 * Test loading with NULL data parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_null_data(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] = "";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) NULL, NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_DATA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with NULL data parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_null_data(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, NULL, 0);
	if (err != CYAML_ERR_BAD_PARAM_NULL_DATA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with NULL data parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_null_data(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t *) &data_tgt, 0,
			(cyaml_data_t **) NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_DATA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_null_data2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		unsigned test_value_uint;
	} data_tgt = {
		.test_value_uint = 9999,
	};
	struct target_struct *data_cpy = &data_tgt;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = NULL,
		.copy = NULL,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(cyaml_data_t  *) &data_tgt, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_ERR_DATA_TARGET_NON_NULL) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with NULL config parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_null_config(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] = "";
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), NULL, NULL,
			(cyaml_data_t **) NULL, NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_CONFIG) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with NULL config parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_null_config(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, NULL, &top_schema, &data, 0);
	if (err != CYAML_ERR_BAD_PARAM_NULL_CONFIG) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with NULL config parameter.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_null_config(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(NULL, NULL, (const cyaml_data_t *) NULL, 0,
			(cyaml_data_t **) NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_CONFIG) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with NULL memory allocation function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_null_mem_fn(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] = "";
	cyaml_config_t cfg = *config;
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.mem_fn = NULL;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, NULL,
			(cyaml_data_t **) NULL, NULL);
	if (err != CYAML_ERR_BAD_CONFIG_NULL_MEMFN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with NULL memory allocation function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_null_mem_fn(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.mem_fn = NULL;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, &data, 0);
	if (err != CYAML_ERR_BAD_CONFIG_NULL_MEMFN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with NULL memory allocation function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_null_mem_fn(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.mem_fn = NULL;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(&cfg, NULL, (const cyaml_data_t *) NULL, 0,
			(cyaml_data_t **) NULL);
	if (err != CYAML_ERR_BAD_CONFIG_NULL_MEMFN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with NULL schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_null_schema(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] = "";
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, NULL,
			(cyaml_data_t **) NULL, NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with NULL schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_null_schema(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, NULL, &data, 0);
	if (err != CYAML_ERR_BAD_PARAM_NULL_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with NULL schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_null_schema(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	void *data_tgt = NULL;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = NULL,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, NULL, (const cyaml_data_t *) NULL, 0,
		(cyaml_data_t **) NULL);
	if (err != CYAML_ERR_BAD_PARAM_NULL_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with bad top level type (non-pointer).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_top_level_non_pointer(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"7\n";
	int *value = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, NULL);
	if (err != CYAML_ERR_TOP_LEVEL_NON_PTR) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with bad top level type (non-pointer).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_top_level_non_pointer(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_TOP_LEVEL_NON_PTR) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with bad top level type (non-pointer).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_top_level_non_pointer(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int *value = NULL;
	int *value_cpy = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema, (const cyaml_data_t *) &value, 0,
			(cyaml_data_t **) &value_cpy);
	if (err != CYAML_ERR_BAD_PARAM_NULL_DATA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with bad top level sequence and no seq_count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_top_level_sequence_no_count(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n";
	struct target_struct {
		int *value;
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, CYAML_UNLIMITED),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_BAD_PARAM_SEQ_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with bad top level sequence and no seq_count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_top_level_not_sequence_count(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"7\n";
	int *value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.seq_count = &count,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, &count);
	if (err != CYAML_ERR_BAD_PARAM_SEQ_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with bad top level sequence and no seq_count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_top_level_sequence_no_count(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int *value;
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, CYAML_UNLIMITED),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t *) &data_tgt, 0,
			(cyaml_data_t **) NULL);
	if (err != CYAML_ERR_BAD_PARAM_SEQ_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with a non-sequence and non-zero sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_top_level_not_sequence_count(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		unsigned *test_uint;
	} data = {
		.test_uint = NULL,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int)
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 42);
	if (err != CYAML_ERR_BAD_PARAM_SEQ_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with bad type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_type(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = 99999,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(data_tgt->value),
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_BAD_TYPE_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema default value with bad type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_type_default(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"{}\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = 99999,
				.flags = CYAML_FLAG_OPTIONAL,
				.data_size = sizeof(data_tgt->value),
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_BAD_TYPE_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with bad type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_type(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int value;
	} data = {
		.value = 99,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = 99999,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(data.value),
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_BAD_TYPE_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with bad type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_bad_type(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int value;
	} data_tgt = {
		.value = 9,
	};
	struct target_struct *data_cpy = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = 99999,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(data_tgt.value),
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_cpy,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t *) &data_tgt, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_ERR_BAD_TYPE_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_cpy != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with string min greater than max.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_string_min_max(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"value: foo\n";
	struct target_struct {
		const char *value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("value", CYAML_FLAG_POINTER,
				struct target_struct, value,
				10, 9),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_BAD_MIN_MAX_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Only scalar mapping keys are allowed by libcyaml.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_non_scalar_mapping_key(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"{1}: value";
	struct target_struct {
		unsigned value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("key", CYAML_FLAG_DEFAULT,
				struct target_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INTERNAL_ERROR) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: 1\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_INT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: 1\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_INT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 9,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (0) for flags.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_strval_t strings[] = {
		{ "foo", 0 },
		{ "bar", 1 },
		{ "baz", 2 },
		{ "bat", 3 },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"  - bat\n"
		"  - bar\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_FLAGS,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
				.enumeration = {
					.strings = strings,
					.count = CYAML_ARRAY_LEN(strings),
				},
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (0) for sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 2\n"
		"  - 4\n";
	struct target_struct {
		int *value;
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data_tgt->value)),
				.sequence = {
					.min = 0,
					.max = CYAML_UNLIMITED,
					.entry = &entry_schema,
				},
			},
			.data_offset = offsetof(struct target_struct, value),
			.count_size = 0,
			.count_offset = offsetof(
					struct target_struct,
					value_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9) for sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 2\n"
		"  - 4\n";
	struct target_struct {
		int *value;
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data_tgt->value)),
				.sequence = {
					.min = 0,
					.max = CYAML_UNLIMITED,
					.entry = &entry_schema,
				},
			},
			.data_offset = offsetof(struct target_struct, value),
			.count_size = 9,
			.count_offset = offsetof(
					struct target_struct,
					value_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9) for sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_6(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 2\n"
		"  - 4\n";
	struct target_struct {
		int value[4];
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(*(data_tgt->value)),
				.sequence = {
					.min = 0,
					.max = 4,
					.entry = &entry_schema,
				},
			},
			.data_offset = offsetof(struct target_struct, value),
			.count_size = 9,
			.count_offset = offsetof(
					struct target_struct,
					value_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_7(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: 1\n";
	struct target_struct {
		unsigned value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_UINT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9) for bitfield.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_8(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: 0x7\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n";
	struct target_struct {
		uint64_t value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "test_bitfield",
			.value = {
				.type = CYAML_BITFIELD,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 9,
				.bitfield = {
					.bitdefs = bitvals,
					.count = CYAML_ARRAY_LEN(bitvals),
				},
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9) for flags.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_9(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_strval_t strings[] = {
		{ "foo", 0 },
		{ "bar", 1 },
		{ "baz", 2 },
		{ "bat", 3 },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"  - bat\n"
		"  - bar\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_FLAGS,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 9,
				.enumeration = {
					.strings = strings,
					.count = CYAML_ARRAY_LEN(strings),
				},
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size (9) for sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_10(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int missing[] = { 1, 2, 3, 4 };
	static const unsigned char yaml[] =
		"{}\n";
	struct target_struct {
		int value[4];
		unsigned value_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_OPTIONAL,
				.data_size = sizeof(*(data_tgt->value)),
				.sequence = {
					.min = 0,
					.max = 4,
					.entry = &entry_schema,
					.missing = missing,
					.missing_count =
						CYAML_ARRAY_LEN(missing),
				},
			},
			.data_offset = offsetof(struct target_struct, value),
			.count_size = 9,
			.count_offset = offsetof(
					struct target_struct,
					value_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		int value;
	} data = {
		.value = 9,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_INT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		unsigned value;
	} data = {
		.value = 9,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_UINT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		bool value;
	} data = {
		.value = 1,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_BOOL,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_e {
		FIRST, SECOND, THIRD, FOURTH, COUNT
	};
	static const cyaml_strval_t strings[COUNT] = {
		[FIRST]  = { "first",  0 },
		[SECOND] = { "second", 1 },
		[THIRD]  = { "third",  2 },
		[FOURTH] = { "fourth", 3 },
	};
	static const struct target_struct {
		enum test_e value;
	} data = {
		.value = 1,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_ENUM,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
				.enumeration = {
					.strings = strings,
					.count = COUNT,
				},
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (5 for floating point value).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		float value;
	} data = {
		.value = 3.14f,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_FLOAT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 5,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_6(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	static const struct target_struct {
		enum test_f value;
	} data = {
		.value = THIRD,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_FLAGS,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
				.enumeration = {
					.strings = strings,
					.count = 4,
				},
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with a bad sequence count size.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_7(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} data = {
		.seq = NULL,
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data.seq)),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = 10,
				},
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_offset = offsetof(struct target_struct, seq_count),
			.count_size = 9,
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with data size (0).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_data_size_8(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const struct target_struct {
		uint64_t test_bitfield;
	} data = {
		.test_bitfield = 0xFFFFFFFFFFFFFFFFu,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "test_bitfield",
			.value = {
				.type = CYAML_BITFIELD,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
				.bitfield = {
					.bitdefs = bitvals,
					.count = CYAML_ARRAY_LEN(bitvals),
				},
			},
			.data_offset = offsetof(struct target_struct,
					test_bitfield),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with bad sequence count size.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_bad_data_size_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned arr[] = { 1, 2, 3, 4, 5 };
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} data = {
		.seq = arr,
		.seq_count = CYAML_ARRAY_LEN(arr),
	};
	struct target_struct *copy = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data.seq)),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = 10,

				},
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_offset = offsetof(struct target_struct, seq_count),
			.count_size = 9,
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.copy = (cyaml_data_t **) &copy,
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t  *) &data, 0,
			(cyaml_data_t **) &copy);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence fixed with unequal min and max.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_sequence_min_max(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - \n";
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE_FIXED,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data_tgt->seq)),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,

				},
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_offset = offsetof(struct target_struct, seq_count),
			.count_size = sizeof(data_tgt->seq_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_SEQUENCE_FIXED_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with sequence fixed with unequal min and max.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_sequence_min_max(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned value = 5;
	struct target_struct {
		unsigned *seq;
	} data = {
		.seq = &value,
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE_FIXED,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data.seq)),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,

				},
			},
			.data_offset = offsetof(struct target_struct, seq),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 0);
	if (err != CYAML_ERR_SEQUENCE_FIXED_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with sequence fixed with unequal min and max.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_sequence_min_max(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} data_tgt = {
		.seq = NULL,
		.seq_count = 0,
	};
	struct target_struct *data_cpy = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE_FIXED,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(*(data_tgt.seq)),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,

				},
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_offset = offsetof(struct target_struct, seq_count),
			.count_size = sizeof(data_tgt.seq_count),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t *) &data_tgt, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_ERR_SEQUENCE_FIXED_COUNT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_cpy != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with data size for float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_data_size_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: 1\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_FLOAT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 7,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence in sequence.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_sequence_in_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"- -\n";
	unsigned **seq = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value inner_entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, **seq),
	};
	static const struct cyaml_schema_value outer_entry_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned,
				&inner_entry_schema, 0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned *,
				&outer_entry_schema, 0, CYAML_UNLIMITED)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &seq,
		.seq_count = &count,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &seq, &count);
	if (err != CYAML_ERR_SEQUENCE_IN_SEQUENCE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (seq != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with sequence in sequence.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_sequence_in_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned seq[4] = { 1, 2, 3 };
	static const unsigned *data[] = { seq, seq };
	static const struct cyaml_schema_value inner_entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, **data),
	};
	static const struct cyaml_schema_value outer_entry_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned,
				&inner_entry_schema, 0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned *,
				&outer_entry_schema, 0, CYAML_UNLIMITED)
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema, &data, 2);
	if (err != CYAML_ERR_SEQUENCE_IN_SEQUENCE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying with schema with sequence in sequence.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_sequence_in_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned data[2][2] = { { 1, 2 }, { 3, 4 } };
	unsigned **seq = NULL;
	static const struct cyaml_schema_value inner_entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, **seq),
	};
	static const struct cyaml_schema_value outer_entry_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned,
				&inner_entry_schema, 0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, unsigned *,
				&outer_entry_schema, 0, CYAML_UNLIMITED)
	};
	test_data_t td = {
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(const cyaml_data_t *) data, 2,
			(cyaml_data_t **) &seq);
	if (err != CYAML_ERR_SEQUENCE_IN_SEQUENCE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (seq != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test copying a mapping with optional mapping missing.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_schema_required_mapping_missing(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct_map {
		char *str;
	};
	struct target_struct {
		char *str;
		struct target_struct_map *map;
	} data = {
		.str = (char *) "Hello",
		.map = NULL,
	};
	struct target_struct *data_cpy = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("map-str",
				CYAML_FLAG_POINTER,
				struct target_struct, str, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("str",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, str, 0, CYAML_UNLIMITED),
		CYAML_FIELD_MAPPING_PTR("map", CYAML_FLAG_POINTER,
				struct target_struct, map,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = NULL,
		.copy = (cyaml_data_t **) &data_cpy,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_copy(config, &top_schema,
			(cyaml_data_t  *) &data, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_ERR_MAPPING_FIELD_MISSING) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint, but value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: scalar\n";
	struct target_struct {
		unsigned a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with string top level type, with bad value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"{ Hello }\n";
	char *value = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, int, 0, CYAML_UNLIMITED)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when flags expected, but numerical value has trailing junk.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_flags_junk(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - 1thousand\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags, but numerical value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_flags_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - -7\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags, but numerical value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_flags_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - 0x100000000\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags, but numerical value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_flags_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - 0x10000000000000000\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a NULL when NULLs aren't allowed by the schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_invalid_value_null_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int d[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
	static const int *data[] = { d + 0, d + 1, d + 2, NULL,
	                             d + 4, d + 5, d + 6, d + 7, };
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, 3),
	};
	char *buffer = NULL;
	size_t len = 0;
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_STYLE_BLOCK;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, data,
			CYAML_ARRAY_LEN(data));
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but numerical value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: invalid\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value is non-scalar.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: {}\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_UNEXPECTED_EVENT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value is not in schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    b: {}\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value too big.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: 0xf\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value is of wrong type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    {}: {}\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_UNEXPECTED_EVENT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value is of wrong type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_bitfield_6(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    []\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects bitfield, but value outside data.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_bad_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  62, .bits =  4 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: 1\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_BAD_BITVAL_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving when schema has bitfield defined outside value data.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_bad_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  30, .bits =  4 },
	};
	static const struct target_struct {
		uint32_t test_bitfield;
	} data = {
		.test_bitfield = 0xFFFFFFFFu,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_ERR_BAD_BITVAL_IN_SCHEMA) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_range1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 3.5e+38\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_range2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: -3.5e+38\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_range3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 1.55331e-40f\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_range4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: -1.55331e-40f\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_junk(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0.452*00E003\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects float but value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_float_invalid(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: Gasp\n";
	struct target_struct {
		float a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects double but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_double_range1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 1.8e+4999\n";
	struct target_struct {
		double a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects double but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_double_range2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: -1.8e+4999\n";
	struct target_struct {
		double a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a",
				CYAML_FLAG_DEFAULT | CYAML_FLAG_STRICT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects double but value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_double_junk(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0.452*00E003\n";
	struct target_struct {
		double a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects double but value is invalid.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_double_invalid(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: Gasp\n";
	struct target_struct {
		double a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value has trailing junk.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_junk(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 3*9+4\n";
	struct target_struct {
		int a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_range_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: -129\n";
	struct target_struct {
		signed char a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_range_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 128\n";
	struct target_struct {
		signed char a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_range_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x10000\n";
	struct target_struct {
		int16_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_range_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x100000000\n";
	struct target_struct {
		int32_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int_range_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x10000000000000000\n";
	struct target_struct {
		int32_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value has trailing junk.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_junk(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 3*8+4\n";
	struct target_struct {
		unsigned int a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_range_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: -1\n";
	struct target_struct {
		unsigned char a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_range_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 256\n";
	struct target_struct {
		unsigned char a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_range_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x10000\n";
	struct target_struct {
		uint16_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_range_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x100000000\n";
	struct target_struct {
		uint32_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects uint but value is out of range.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_uint_range_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 0x10000000000000000\n";
	struct target_struct {
		uint32_t a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int8_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: -129\n";
	struct target_struct {
		int8_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int8_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: 128\n";
	struct target_struct {
		int8_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int16_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: -32769\n";
	struct target_struct {
		int16_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int16_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: 32768\n";
	struct target_struct {
		int16_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int32_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: -2147483649\n";
	struct target_struct {
		int32_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int32_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: 2147483648\n";
	struct target_struct {
		int32_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int64_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: -9223372036854775809\n";
	struct target_struct {
		int64_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test integer limit.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_int64_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"val: 9223372036854775808\n";
	struct target_struct {
		int64_t val;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("val", CYAML_FLAG_DEFAULT,
				struct target_struct, val),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a positive signed integer with a range constraint.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_range_int_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 90\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = -16,
					.max =  64,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a negative signed integer with a range constraint.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_range_int_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: -9000\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = -16,
					.max =  64,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an unsigned integer with a range constraint.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_range_uint_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 10\n";
	struct target_struct {
		unsigned test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(UINT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min =  50,
					.max = 100,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an unsigned integer with a range constraint.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_invalid_value_range_uint_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 200\n";
	struct target_struct {
		unsigned test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(UINT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min =  50,
					.max = 100,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Integer validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] value   The value to be validated.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__int_is_even(void *ctx,
		const cyaml_schema_value_t *schema,
		int64_t value)
{
	UNUSED(ctx);
	UNUSED(schema);

	return !(value & 1);
}

/**
 * Integer validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] value   The value to be validated.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__uint_is_power_of_2(void *ctx,
		const cyaml_schema_value_t *schema,
		uint64_t value)
{
	UNUSED(ctx);
	UNUSED(schema);

	return (value & (value - 1)) == 0;
}

/**
 * Floating point validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] value   The value to be validated.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__float_near_pi(void *ctx,
		const cyaml_schema_value_t *schema,
		double value)
{
	UNUSED(ctx);
	UNUSED(schema);

	return fabs(value - M_PI) < 0.01;
}

/**
 * String validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] value   The value to be validated.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__string_has_no_spaces(void *ctx,
		const cyaml_schema_value_t *schema,
		const char *value)
{
	UNUSED(ctx);
	UNUSED(schema);

	while (*value != '\0') {
		if (*value == ' ') {
			return false;
		}
		value++;
	}

	return true;
}

enum test_animal_e {
	TEST_ANIMAL_BAT,
	TEST_ANIMAL_DOG,
	TEST_ANIMAL_CAT,
};

struct test_animal_s {
	enum test_animal_e animal;
	char *noise;
};

/**
 * Mapping validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] value   The value to be validated.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__mapping_cat_validator(void *ctx,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *value)
{
	const struct test_animal_s *animal = value;

	UNUSED(ctx);
	UNUSED(schema);

	if (animal->animal == TEST_ANIMAL_CAT) {
		if (strcmp(animal->noise, "meow") == 0 ||
		    strcmp(animal->noise, "purr") == 0) {
			return true;
		}
	}

	return false;
}

/**
 * Sequence validation callback.
 *
 * \param[in] ctx     Client's private validation context.
 * \param[in] schema  The schema for the value.
 * \param[in] seq       The value to be validated.
 * \param[in] seq_count Number of entries in seq.
 * \return `true` if values is valid, `false` otherwise.
 */
static bool test__sequence_is_fibonacci(void *ctx,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *seq,
		size_t seq_count)
{
	const int8_t *data = seq;

	UNUSED(ctx);
	UNUSED(schema);

	if (seq_count > 2) {
		for (size_t i = 2; i < seq_count; i++) {
			if (data[i] != data[i - 2] + data[i - 1]) {
				return false;
			}
		}
	}

	return true;
}

/**
 * Test loading a signed integer with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 91\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.validation_cb = test__int_is_even,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an unsigned integer with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 65\n";
	struct target_struct {
		unsigned test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(UINT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.validation_cb =
						test__uint_is_power_of_2,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enum with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum enum_test {
		ENUM_TEST_BAT,
		ENUM_TEST_DOG,
		ENUM_TEST_CAT,
	};
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = ENUM_TEST_CAT },
		{ .str = "bat", .val = ENUM_TEST_BAT },
		{ .str = "dog", .val = ENUM_TEST_DOG },
	};
	static const unsigned char yaml[] =
		"test: dog\n";
	struct target_struct {
		enum enum_test test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(ENUM, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .strings = enum_test_schema,
				  .count = CYAML_ARRAY_LEN(enum_test_schema),
				  .validation_cb = test__int_is_even,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flags with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_flags(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum flags_test {
		FLAGS_TEST_BAT = 1 << 0,
		FLAGS_TEST_DOG = 1 << 1,
		FLAGS_TEST_CAT = 1 << 2,
	};
	static const cyaml_strval_t flags_test_schema[] = {
		{ .str = "cat", .val = FLAGS_TEST_CAT },
		{ .str = "bat", .val = FLAGS_TEST_BAT },
		{ .str = "dog", .val = FLAGS_TEST_DOG },
	};
	static const unsigned char yaml[] =
		"test: [ bat ]\n";
	struct target_struct {
		enum flags_test test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(FLAGS, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .strings = flags_test_schema,
				  .count = CYAML_ARRAY_LEN(flags_test_schema),
				  .validation_cb = test__int_is_even,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test: { b: 1, c: 8 }\n";
	struct target_struct {
		uint8_t before;
		uint64_t test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(BITFIELD, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .bitdefs = bitvals,
				  .count = CYAML_ARRAY_LEN(bitvals),
				  .validation_cb = test__uint_is_power_of_2,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a float with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 3.1\n";
	struct target_struct {
		float test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(FLOAT, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .validation_cb = test__float_near_pi }),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a double with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: 3.1\n";
	struct target_struct {
		double test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(FLOAT, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .validation_cb = test__float_near_pi }),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test: Cats are the best\n";
	struct target_struct {
		char *test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_PTR(STRING, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .min = 0, .max = CYAML_UNLIMITED,
				  .validation_cb = test__string_has_no_spaces,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = TEST_ANIMAL_CAT },
		{ .str = "bat", .val = TEST_ANIMAL_BAT },
		{ .str = "dog", .val = TEST_ANIMAL_DOG },
	};
	static const unsigned char yaml[] =
		"animal: cat\n"
		"noise: bark\n";
	struct test_animal_s *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(ENUM, "animal", CYAML_FLAG_OPTIONAL,
				struct test_animal_s, animal,
				{ .strings = enum_test_schema,
				  .count = CYAML_ARRAY_LEN(enum_test_schema),
				}),
		CYAML_FIELD_PTR(STRING, "noise",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_animal_s, noise,
				{ .min = 0, .max = CYAML_UNLIMITED }),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE(MAPPING, CYAML_FLAG_POINTER,
				struct test_animal_s, {
					.fields = mapping_schema,
					.validation_cb =
						test__mapping_cat_validator,
				}),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int8_t ref[] = { 1, 1, 2, 4, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 4\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int8_t seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_PTR(SEQUENCE, "sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_ARRAY_LEN(ref),
					.validation_cb =
						test__sequence_is_fibonacci,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with a validation callback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_validation_cb_sequence_fixed(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int8_t ref[] = { 1, 1, 2, 4, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 4\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int8_t seq[CYAML_ARRAY_LEN(ref)];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_PTR(SEQUENCE_FIXED, "sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, {
					.entry = &entry_schema,
					.min = CYAML_ARRAY_LEN(ref),
					.max = CYAML_ARRAY_LEN(ref),
					.validation_cb =
						test__sequence_is_fibonacci,
				}),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects string, but it's too short.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_string_min_length(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: foo\n";
	struct target_struct {
		char *a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a, 4, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_STRING_LENGTH_MIN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects string, but it's too long.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_string_max_length(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: fifth\n";
	struct target_struct {
		char *a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a, 4, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_STRING_LENGTH_MAX) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with a duplicate field.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_duplicate_mapping_field(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test_uint: 9998\n"
		"test_uint: 9999\n";
	struct target_struct {
		unsigned test_value_uint;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_UNEXPECTED_EVENT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects mapping field which is not present.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_missing_mapping_field(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"a: 2\n";
	struct target_struct {
		int a;
		int b;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_MAPPING_FIELD_MISSING) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema disallows mapping field.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_unknown_mapping_field(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"wrong_key: 2\n";
	struct target_struct {
		int a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_KEY) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects sequence, it has too few entries.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_sequence_min_entries(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 1\n"
		"  - 2\n";
	struct target_struct {
		int *a;
		unsigned a_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->a)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("key", CYAML_FLAG_POINTER,
				struct target_struct, a, &entry_schema,
				3, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_SEQUENCE_ENTRIES_MIN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects sequence, it has too many entries.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_sequence_max_entries(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 1\n"
		"  - 2\n"
		"  - 3\n";
	struct target_struct {
		int *a;
		unsigned a_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->a)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("key", CYAML_FLAG_POINTER,
				struct target_struct, a, &entry_schema,
				2, 2),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_SEQUENCE_ENTRIES_MAX) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags and finds a mapping inside.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_flags_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - map:\n"
		"        a:\n"
		"        b:\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_STRICT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_UNEXPECTED_EVENT) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects enum, but string is not allowed.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_enum_bad_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	};
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"key: fourth\n";
	struct target_struct {
		enum test_enum a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, TEST_ENUM__COUNT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags but YAML has bad flag string.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_flags_bad_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - seventh\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving when schema expects strict enum, but value not allowed.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_strict_enum_bad_value(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	};
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	struct target_struct {
		enum test_enum a;
	} data = {
		.a = 876,
	};
	char *buffer = NULL;
	size_t len = 0;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("key", CYAML_FLAG_STRICT,
				struct target_struct, a,
				strings, TEST_ENUM__COUNT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects strict enum but YAML has bad string.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_strict_enum_bad_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	};
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"key: fourth\n";
	struct target_struct {
		enum test_enum a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("key", CYAML_FLAG_STRICT,
				struct target_struct, a,
				strings, TEST_ENUM__COUNT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test saving when schema expects strict flags, but value not allowed.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_schema_strict_flags_bad_value(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	struct target_struct {
		enum test_flags a;
	} data = {
		.a = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | (1 << 9),
	};
	char *buffer = NULL;
	size_t len = 0;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_STRICT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (buffer != NULL || len != 0) {
		return ttest_fail(&tc, "Buffer/len not untouched.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects strict flags but YAML has bad string.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_strict_flags_bad_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key:\n"
		"    - first\n"
		"    - seventh\n";
	struct target_struct {
		enum test_flags a;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_STRICT,
				struct target_struct, a,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int, but YAML has sequence.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_int_read_seq(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n"
		"  - 90";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("key", CYAML_FLAG_DEFAULT,
				struct target_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int, but YAML ends.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_int_read_end_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key:\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("key", CYAML_FLAG_DEFAULT,
				struct target_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects int, but YAML ends.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_int_read_end_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: |"
		"...";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("key", CYAML_FLAG_DEFAULT,
				struct target_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_LIBYAML_PARSER) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects flags, but YAML has scalar.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_flags_read_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"key: first\n";
	struct target_struct {
		int value;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("key", CYAML_FLAG_DEFAULT,
				struct target_struct, value,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects mapping, but YAML has scalar.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_mapping_read_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: scalar\n";
	struct value_s {
		int a;
	};
	struct target_struct {
		struct value_s test_value_mapping;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING("key", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_mapping,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when schema expects sequence, but YAML has scalar.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_schema_expect_sequence_read_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"key: foo\n";
	struct target_struct {
		int *a;
		unsigned a_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->a)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("key", CYAML_FLAG_POINTER,
				struct target_struct, a, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Data non-NULL on error.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_free_null(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, NULL, NULL, &tc)) {
		return true;
	}

	UNUSED(config);

	cyaml_mem(NULL, NULL, 0);

	return ttest_pass(&tc);
}

/** Context for allocation failure tests. */
struct test_cyaml_mem_ctx {
	unsigned required;
	unsigned fail;
	unsigned current;
};

/*
 * Allocation counter.
 *
 * Used to count all allocations made when loading an input.
 *
 * \param[in]  ctx   Allocation context.
 * \param[in]  ptr   Pointer to allocation to resize or NULL.
 * \param[in]  size  Size to set allocation to.
 * \return Pointer to new allocation, or NULL on failure.
 */
static void * test_cyaml_mem_count_allocs(
		void *ctx,
		void *ptr,
		size_t size)
{
	struct test_cyaml_mem_ctx *mem_ctx = ctx;

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	mem_ctx->required++;

	return realloc(ptr, size);
}

/**
 * Allocation failure tester.
 *
 * Used to make specific allocation fail.
 *
 * \param[in]  ctx   Allocation context.
 * \param[in]  ptr   Pointer to allocation to resize or NULL.
 * \param[in]  size  Size to set allocation to.
 * \return Pointer to new allocation, or NULL on failure.
 */
static void * test_cyaml_mem_fail(
		void *ctx,
		void *ptr,
		size_t size)
{
	struct test_cyaml_mem_ctx *mem_ctx = ctx;

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	if (mem_ctx->current == mem_ctx->fail) {
		return NULL;
	}

	mem_ctx->current++;

	return realloc(ptr, size);
}

/**
 * Test loading, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_alloc_oom_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const int test[] = { 99, 98, 97 };
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n";
	struct animal_s {
		char *kind;
		char *sound;
		int *position;
		int *default_int;
		int *default_sequence;
		unsigned default_sequence_count;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE_FIXED("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 3),
		CYAML_FIELD_PTR(INT, "default_int",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct animal_s, default_int,
				{ .missing = 9 }),
		CYAML_FIELD_PTR(SEQUENCE, "default_sequence",
				CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
				struct animal_s, default_sequence,
				{ .entry = &position_entry_schema,
				  .min = CYAML_ARRAY_LEN(test),
				  .max = CYAML_ARRAY_LEN(test),
				  .missing = test,
				  .missing_count = CYAML_ARRAY_LEN(test) }),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	/* Now free what was loaded. */
	cyaml_free(config, &top_schema, data_tgt, 0);
	data_tgt = NULL;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
				(cyaml_data_t **) &data_tgt, NULL);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_free(config, &top_schema, data_tgt, 0);
		data_tgt = NULL;
	}

	return ttest_pass(&tc);
}

/**
 * Test loading, with all memory allocation failure at every possible point.
 *
 * Uses aliases and anchors, to exercies the event/anchor recording error
 * paths too.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_alloc_oom_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 {"
		"      kind: cat,\n"
		"      sound: meow,\n"
		"      position: &a2 [ 1, &my_value 2, 1],\n"
		"      flags: &a3 [\n"
		"        first,\n"
		"        &a4 second,\n"
		"        third,\n"
		"        fourth,\n"
		"      ]\n"
		"    }\n"
		"  - kind: snake\n"
		"    sound: &a5 hiss\n"
		"    position: &a6 [ 3, 1, 0]\n"
		"    flags: &a7 [\n"
		"      first,\n"
		"      second,\n"
		"      third,\n"
		"      fourth,\n"
		"    ]\n"
		"animals:\n"
		"  - *a1\n"
		"  - kind: snake\n"
		"    sound: *a5\n"
		"    position: *a6\n"
		"    flags: *a7\n"
		"    value: *my_value\n";
	struct animal_s {
		char *kind;
		char *sound;
		int **position;
		unsigned position_count;
		enum test_f *flags;
		int value;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_FLAGS("flags",
				CYAML_FLAG_STRICT | CYAML_FLAG_POINTER,
				struct animal_s, flags, strings, 4),
		CYAML_FIELD_INT("value", CYAML_FLAG_OPTIONAL,
				struct animal_s, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	/* Now free what was loaded. */
	cyaml_free(config, &top_schema, data_tgt, 0);
	data_tgt = NULL;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
				(cyaml_data_t **) &data_tgt, NULL);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_free(config, &top_schema, data_tgt, 0);
		data_tgt = NULL;
	}

	return ttest_pass(&tc);
}

/**
 * Test saving, with memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_alloc_oom_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n";
	struct animal_s {
		char *kind;
		char *sound;
		int *position;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE_FIXED("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 3),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/* First we load the YAML, so we have something to test saving. */
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, data_tgt, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	/* Now free what was loaded. */
	cyaml_mem(&mem_ctx, buffer, 0);
	buffer = NULL;
	len = 0;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_save_data(&buffer, &len, &cfg, &top_schema,
				data_tgt, 0);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_mem(&mem_ctx, buffer, 0);
		buffer = NULL;
		len = 0;
	}

	return ttest_pass(&tc);
}

/**
 * Test loading, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_save_alloc_oom_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"    flags:\n"
		"      - first\n"
		"      - second\n"
		"      - third\n"
		"      - fourth\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n"
		"    flags:\n"
		"      - first\n"
		"      - second\n"
		"      - third\n"
		"      - fourth\n";
	struct animal_s {
		char *kind;
		char *sound;
		int **position;
		unsigned position_count;
		enum test_f *flags;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_FLAGS("flags",
				CYAML_FLAG_STRICT | CYAML_FLAG_POINTER,
				struct animal_s, flags, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/* First we load the YAML, so we have something to test saving. */
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, data_tgt, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	/* Now free what was loaded. */
	cyaml_mem(&mem_ctx, buffer, 0);
	buffer = NULL;
	len = 0;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_save_data(&buffer, &len, &cfg, &top_schema,
				data_tgt, 0);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_mem(&mem_ctx, buffer, 0);
		buffer = NULL;
		len = 0;
	}

	return ttest_pass(&tc);
}

/**
 * Test copying, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_alloc_oom_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n";
	struct animal_s {
		char *kind;
		char *sound;
		int *position;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	struct target_struct *data_cpy = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE_FIXED("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 3),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.copy = (cyaml_data_t **) &data_cpy,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	err = cyaml_copy(&cfg, &top_schema,
			(cyaml_data_t  *)  data_tgt, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	cyaml_free(config, &top_schema, data_cpy, 0);
	data_cpy = NULL;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_copy(&cfg, &top_schema,
				(cyaml_data_t  *)  data_tgt, 0,
				(cyaml_data_t **) &data_cpy);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_free(config, &top_schema, data_cpy, 0);
		data_cpy = NULL;
	}

	return ttest_pass(&tc);
}

/**
 * Test copying, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_alloc_oom_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"    flags:\n"
		"      - first\n"
		"      - second\n"
		"      - third\n"
		"      - fourth\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n"
		"    flags:\n"
		"      - first\n"
		"      - second\n"
		"      - third\n"
		"      - fourth\n";
	struct animal_s {
		char *kind;
		char *sound;
		int **position;
		unsigned position_count;
		enum test_f *flags;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	struct target_struct *data_cpy = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_FLAGS("flags",
				CYAML_FLAG_STRICT | CYAML_FLAG_POINTER,
				struct animal_s, flags, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.copy = (cyaml_data_t **) &data_cpy,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	err = cyaml_copy(&cfg, &top_schema,
			(cyaml_data_t  *)  data_tgt, 0,
			(cyaml_data_t **) &data_cpy);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	cyaml_free(config, &top_schema, data_cpy, 0);
	data_cpy = NULL;

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_copy(&cfg, &top_schema,
				(cyaml_data_t  *)  data_tgt, 0,
				(cyaml_data_t **) &data_cpy);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_free(config, &top_schema, data_cpy, 0);
		data_cpy = NULL;
	}

	return ttest_pass(&tc);
}

/**
 * Test copying, with all memory allocation failure at every possible point.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_copy_alloc_oom_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"animals:\n"
		"  - kind: cat\n"
		"    sound: meow\n"
		"    position: [ 1, 2, 1]\n"
		"  - kind: snake\n"
		"    sound: hiss\n"
		"    position: [ 3, 1, 0]\n";
	struct animal_s {
		char *kind;
		char *sound;
		int *position;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	struct target_struct data_cpy = { 0 };
	struct target_struct *data_cpy_ptr = &data_cpy;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE_FIXED("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 3),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	static const struct cyaml_schema_value top_schema2 = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.copy = NULL,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;
	struct test_cyaml_mem_ctx mem_ctx = {
		.required = 0,
	};

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	/*
	 * First we load the YAML with the counting allocation function,
	 * to find the number of allocations required to load the document.
	 * This is deterministic.
	 */
	cfg.mem_fn = test_cyaml_mem_count_allocs;
	cfg.mem_ctx = &mem_ctx;

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	err = cyaml_copy(&cfg, &top_schema2,
			(cyaml_data_t  *)  data_tgt, 0,
			(cyaml_data_t **) &data_cpy_ptr);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (mem_ctx.required == 0) {
		return ttest_fail(&tc, "There were no allocations.");
	}

	cyaml_free(config, &top_schema2, data_cpy_ptr, 0);
	data_cpy_ptr = &data_cpy;
	memset(data_cpy_ptr, 0, sizeof(*data_cpy_ptr));

	/*
	 * Now we load the document multiple times, forcing every possible
	 * allocation to fail.
	 */
	cfg.mem_fn = test_cyaml_mem_fail;

	for (mem_ctx.fail = 0; mem_ctx.fail < mem_ctx.required;
			mem_ctx.fail++) {
		mem_ctx.current = 0;
		err = cyaml_copy(&cfg, &top_schema2,
				(cyaml_data_t  *)  data_tgt, 0,
				(cyaml_data_t **) &data_cpy_ptr);
		if (err != CYAML_ERR_OOM) {
			return ttest_fail(&tc, cyaml_strerror(err));
		}

		/* Now free what was loaded. */
		cyaml_free(config, &top_schema2, data_cpy_ptr, 0);
		data_cpy_ptr = &data_cpy;
		memset(data_cpy_ptr, 0, sizeof(*data_cpy_ptr));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flag with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_flag_value_alias(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		unsigned a;
		unsigned b;
	};
	static const cyaml_strval_t str[] = {
		{ "one",   (1 << 0) },
		{ "two",   (1 << 1) },
		{ "three", (1 << 2) },
		{ "four",  (1 << 3) },
		{ "five",  (1 << 4) },
	};
	static const unsigned char yaml[] =
		"a: \n"
		"  - &foo one\n"
		"  - two\n"
		"  - five\n"
		"b:\n"
		"  - *foo\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a, str, 5),
		CYAML_FIELD_FLAGS("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b, str, 5),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_bitfield_value_alias_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "7", .offset =  0, .bits =  1 },
		{ .name = "a", .offset =  1, .bits =  2 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: &foo 2\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n"
		"test_bitfield2:\n"
		"    *foo: 1\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n";
	struct target_struct {
		uint64_t test_value_bitfield;
		uint64_t test_value_bitfield2;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_BITFIELD("test_bitfield2", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield2,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_bitfield_value_alias_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "7", .offset =  0, .bits =  1 },
		{ .name = "a", .offset =  1, .bits =  2 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: &foo 2\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n"
		"test_bitfield2:\n"
		"    a: *foo\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n";
	struct target_struct {
		uint64_t test_value_bitfield;
		uint64_t test_value_bitfield2;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_BITFIELD("test_bitfield2", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield2,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_mapping_key_alias(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		char *a;
		char *b;
		char *c;
		char *d;
	};
	static const unsigned char yaml[] =
		"a: &example b\n"
		"*example: test\n"
		"c: meh\n"
		"d: foo\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct target_struct, b, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("c", CYAML_FLAG_POINTER,
				struct target_struct, c, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("d", CYAML_FLAG_POINTER,
				struct target_struct, d, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_mapping_value_alias_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		char *a;
		char *b;
		char *c;
		char *d;
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: &foo 900\n"
		"d: *foo\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct target_struct, b, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("c", CYAML_FLAG_POINTER,
				struct target_struct, c, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("d", CYAML_FLAG_POINTER,
				struct target_struct, d, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_mapping_value_alias_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		char *a;
		char *b;
		char *c;
		char *d;
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: &foo d\n"
		"c: *d\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct target_struct, b, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("d", CYAML_FLAG_POINTER,
				struct target_struct, d, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS | CYAML_CFG_IGNORE_UNKNOWN_KEYS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with an aliased value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_mapping_value_alias_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		char *a;
		char *b;
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: &foo d\n"
		"c: [a, b, c, *foo]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct target_struct, b, 0, CYAML_UNLIMITED),
		CYAML_FIELD_IGNORE("c", CYAML_FLAG_DEFAULT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_NO_ALIAS;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with an aliased string value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_invalid_alias(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"test_string_anchor: &foo Hello World!\n"
		"test_string: *bar\n"
		"test_int: 9\n";
	struct target_struct {
		char * test_value_string;
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("test_string_anchor", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an incomplete alias.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_incomplete_alias(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 {\n"
		"      a: 777,\n"
		"      b: *a1,\n"
		"    }\n"
		"test: *a1\n";
	struct my_test {
		int a;
		char *b;
	};
	struct target_struct {
		struct my_test test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field inner_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct my_test, a),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct my_test, b,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_MAPPING("test", CYAML_FLAG_DEFAULT,
				struct target_struct, test,
				inner_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_ALIAS) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/** Structure for log messages checking. */
struct log_check {
	unsigned string_count_expected; /**< Number of expected log messages. */
	const char *const *strings;     /**< Array of expected log messages. */
	unsigned string_count;          /**< Number of log messages captured. */
	bool error;                     /**< Whether a logging error has been found. */
};

/**
 * Initialise a log check structure for a given log string vector.
 *
 * \param[in]  lc    The log check structure to initialise.
 * \param[in]  strv  A string vector containing expected log messages.
 * \return true on success, false otherwise.
 */
static inline bool log_check_init(
		struct log_check *lc,
		const char *const *strv)
{
	if (lc == NULL) {
		return false;
	}

	lc->string_count_expected = 0;
	lc->string_count = 0;
	lc->strings = strv;
	lc->error = false;

	while (strv[lc->string_count_expected] != NULL) {
		lc->string_count_expected++;
	}

	return true;
}

static struct log_check lc;

/**
 * CYAML log function, for checking logs.
 *
 * Uses the lc static state.
 *
 * \param[in] level  Log level of message to log.
 * \param[in] ctx    Client's private logging context.
 * \param[in] fmt    Format string for message to log.
 * \param[in] args   Additional arguments used by fmt.
 */
static void cyaml_log_check(
		cyaml_log_t level,
		void *ctx,
		const char *fmt,
		va_list args)
{
	int ret;
	size_t len;
	char buffer[128];

	(void)(ctx);

	if (level < CYAML_LOG_ERROR) {
		/* We only care that critical log messages emerge. */
		return;
	}

	ret = vsnprintf(buffer, sizeof(buffer), fmt, args);
	if (ret <= 0) {
		fprintf(stderr, "TEST ERROR: Logging error\n");
		return;
	}
	len = (unsigned)ret;
	if (len >= sizeof(buffer)) {
		fprintf(stderr, "TEST ERROR: Buffer too small\n");
		assert(len < sizeof(buffer));
		return;
	}

	if (lc.string_count >= lc.string_count_expected) {
		fprintf(stderr, "ERROR: More log messages than expected\n");
		fprintf(stderr, "     got: %s", buffer);
		lc.error |= true;
		goto out;
	}

	if (strcmp(lc.strings[lc.string_count], buffer) != 0) {
		fprintf(stderr, "ERROR: Unexpected log message\n");
		fprintf(stderr, "     got: %s", buffer);
		fprintf(stderr, "expected: %s", lc.strings[lc.string_count]);
		lc.error |= true;
		goto out;
	}

out:
	lc.string_count++;
}

/**
 * Check that the logging contains the expected information.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_err_load_log(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const char * const expected_log[] = {
		"Load: Invalid INT value: 'foo'\n",
		"Load: Backtrace:\n",
		"  in sequence entry '2' (line: 5, column: 26)\n",
		"  in mapping field 'position' (line: 5, column: 17)\n",
		"  in sequence entry '1' (line: 2, column: 5)\n",
		"  in mapping field 'animals' (line: 23, column: 3)\n",
		NULL
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	cyaml_config_t cfg = *config;
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 {\n"
		"      kind: cat,\n"
		"      sound: meow,\n"
		"      position: &a2 [ 1, &my_value foo, 1],\n"
		"      flags: &a3 [\n"
		"        first,\n"
		"        &a4 second,\n"
		"        third,\n"
		"        fourth,\n"
		"      ]\n"
		"    }\n"
		"  - kind: snake\n"
		"    sound: &a5 hiss\n"
		"    position: &a6 [ 3, 1, 0]\n"
		"    flags: &a7 [\n"
		"      first,\n"
		"      second,\n"
		"      third,\n"
		"      fourth,\n"
		"    ]\n"
		"animals:\n"
		"  - *a1\n"
		"  - kind: snake\n"
		"    sound: *a5\n"
		"    position: *a6\n"
		"    flags: *a7\n"
		"    value: *my_value\n";
	struct animal_s {
		char *kind;
		char *sound;
		int **position;
		unsigned position_count;
		enum test_f *flags;
		int value;
	};
	struct target_struct {
		struct animal_s **animal;
		uint32_t animal_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value position_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int),
	};
	static const struct cyaml_schema_field animal_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal_s, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("sound", CYAML_FLAG_POINTER,
				struct animal_s, sound, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("position", CYAML_FLAG_POINTER,
				struct animal_s, position,
				&position_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_FLAGS("flags",
				CYAML_FLAG_STRICT | CYAML_FLAG_POINTER,
				struct animal_s, flags, strings, 4),
		CYAML_FIELD_INT("value", CYAML_FLAG_OPTIONAL,
				struct animal_s, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animal_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, **(data_tgt->animal),
				animal_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animal,
				&animal_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	log_check_init(&lc, expected_log);
	cfg.log_fn = cyaml_log_check;
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err == CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (lc.error == true) {
		return ttest_fail(&tc, "Unexpected log message");
	}
	if (lc.string_count_expected > lc.string_count) {
		return ttest_fail(&tc, "Missing log message(s)");
	}

	return ttest_pass(&tc);
}

/**
 * Run the CYAML error unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool errs_tests(
		ttest_report_ctx_t *rc,
		cyaml_log_t log_level,
		cyaml_log_fn_t log_fn)
{
	bool pass = true;
	cyaml_config_t config = {
		.log_fn = log_fn,
		.mem_fn = cyaml_mem,
		.log_level = log_level,
		.flags = CYAML_CFG_DEFAULT,
	};

	/* Since we expect loads of error logging for these tests,
	 * suppress log output if required log level is greater
	 * than \ref CYAML_LOG_INFO.
	 */
	if (log_level > CYAML_LOG_INFO) {
		config.log_fn = NULL;
	}

	ttest_heading(rc, "Bad parameter tests");

	pass &= test_err_load_null_data(rc, &config);
	pass &= test_err_save_null_data(rc, &config);
	pass &= test_err_copy_null_data(rc, &config);
	pass &= test_err_copy_null_data2(rc, &config);
	pass &= test_err_load_null_config(rc, &config);
	pass &= test_err_save_null_config(rc, &config);
	pass &= test_err_copy_null_config(rc, &config);
	pass &= test_err_load_null_mem_fn(rc, &config);
	pass &= test_err_save_null_mem_fn(rc, &config);
	pass &= test_err_copy_null_mem_fn(rc, &config);
	pass &= test_err_load_null_schema(rc, &config);
	pass &= test_err_save_null_schema(rc, &config);
	pass &= test_err_copy_null_schema(rc, &config);
	pass &= test_err_load_schema_top_level_non_pointer(rc, &config);
	pass &= test_err_save_schema_top_level_non_pointer(rc, &config);
	pass &= test_err_copy_schema_top_level_non_pointer(rc, &config);
	pass &= test_err_load_schema_top_level_sequence_no_count(rc, &config);
	pass &= test_err_copy_schema_top_level_sequence_no_count(rc, &config);
	pass &= test_err_load_schema_top_level_not_sequence_count(rc, &config);
	pass &= test_err_save_schema_top_level_not_sequence_count(rc, &config);

	ttest_heading(rc, "Bad schema tests");

	pass &= test_err_load_schema_bad_type(rc, &config);
	pass &= test_err_save_schema_bad_type(rc, &config);
	pass &= test_err_copy_schema_bad_type(rc, &config);
	pass &= test_err_load_schema_bad_bitfield(rc, &config);
	pass &= test_err_save_schema_bad_bitfield(rc, &config);
	pass &= test_err_load_schema_string_min_max(rc, &config);
	pass &= test_err_load_schema_bad_data_size_1(rc, &config);
	pass &= test_err_load_schema_bad_data_size_2(rc, &config);
	pass &= test_err_load_schema_bad_data_size_3(rc, &config);
	pass &= test_err_load_schema_bad_data_size_4(rc, &config);
	pass &= test_err_load_schema_bad_data_size_5(rc, &config);
	pass &= test_err_load_schema_bad_data_size_6(rc, &config);
	pass &= test_err_load_schema_bad_data_size_7(rc, &config);
	pass &= test_err_load_schema_bad_data_size_8(rc, &config);
	pass &= test_err_load_schema_bad_data_size_9(rc, &config);
	pass &= test_err_save_schema_bad_data_size_1(rc, &config);
	pass &= test_err_save_schema_bad_data_size_2(rc, &config);
	pass &= test_err_save_schema_bad_data_size_3(rc, &config);
	pass &= test_err_save_schema_bad_data_size_4(rc, &config);
	pass &= test_err_save_schema_bad_data_size_5(rc, &config);
	pass &= test_err_save_schema_bad_data_size_6(rc, &config);
	pass &= test_err_save_schema_bad_data_size_7(rc, &config);
	pass &= test_err_save_schema_bad_data_size_8(rc, &config);
	pass &= test_err_copy_schema_bad_data_size_1(rc, &config);
	pass &= test_err_load_schema_bad_data_size_10(rc, &config);
	pass &= test_err_load_schema_sequence_min_max(rc, &config);
	pass &= test_err_save_schema_sequence_min_max(rc, &config);
	pass &= test_err_copy_schema_sequence_min_max(rc, &config);
	pass &= test_err_load_schema_bad_type_default(rc, &config);
	pass &= test_err_load_schema_bad_data_size_float(rc, &config);
	pass &= test_err_load_schema_sequence_in_sequence(rc, &config);
	pass &= test_err_save_schema_sequence_in_sequence(rc, &config);
	pass &= test_err_copy_schema_sequence_in_sequence(rc, &config);
	pass &= test_err_copy_schema_required_mapping_missing(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: bad values");

	pass &= test_err_load_non_scalar_mapping_key(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint(rc, &config);
	pass &= test_err_load_schema_invalid_value_string(rc, &config);
	pass &= test_err_load_schema_invalid_value_flags_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_flags_2(rc, &config);
	pass &= test_err_load_schema_invalid_value_flags_3(rc, &config);
	pass &= test_err_save_schema_invalid_value_null_ptr(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_junk(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_junk(rc, &config);
	pass &= test_err_load_schema_invalid_value_flags_junk(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_2(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_3(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_4(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_5(rc, &config);
	pass &= test_err_load_schema_invalid_value_bitfield_6(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_junk(rc, &config);
	pass &= test_err_load_schema_invalid_value_double_junk(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_range_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_range_2(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_range_3(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_range_4(rc, &config);
	pass &= test_err_load_schema_invalid_value_int_range_5(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_range_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_range_2(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_range_3(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_range_4(rc, &config);
	pass &= test_err_load_schema_invalid_value_uint_range_5(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_range1(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_range2(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_range3(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_range4(rc, &config);
	pass &= test_err_load_schema_invalid_value_double_range1(rc, &config);
	pass &= test_err_load_schema_invalid_value_double_range2(rc, &config);
	pass &= test_err_load_schema_invalid_value_float_invalid(rc, &config);
	pass &= test_err_load_schema_invalid_value_double_invalid(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: Test integer limits");

	pass &= test_err_load_schema_invalid_value_int8_limit_neg(rc, &config);
	pass &= test_err_load_schema_invalid_value_int8_limit_pos(rc, &config);
	pass &= test_err_load_schema_invalid_value_int16_limit_neg(rc, &config);
	pass &= test_err_load_schema_invalid_value_int16_limit_pos(rc, &config);
	pass &= test_err_load_schema_invalid_value_int32_limit_neg(rc, &config);
	pass &= test_err_load_schema_invalid_value_int32_limit_pos(rc, &config);
	pass &= test_err_load_schema_invalid_value_int64_limit_neg(rc, &config);
	pass &= test_err_load_schema_invalid_value_int64_limit_pos(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: Integer range constraints");

	pass &= test_err_load_schema_invalid_value_range_int_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_range_int_2(rc, &config);
	pass &= test_err_load_schema_invalid_value_range_uint_1(rc, &config);
	pass &= test_err_load_schema_invalid_value_range_uint_2(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: Validation callbacks");

	pass &= test_err_load_schema_validation_cb_int(rc, &config);
	pass &= test_err_load_schema_validation_cb_uint(rc, &config);
	pass &= test_err_load_schema_validation_cb_enum(rc, &config);
	pass &= test_err_load_schema_validation_cb_flags(rc, &config);
	pass &= test_err_load_schema_validation_cb_float(rc, &config);
	pass &= test_err_load_schema_validation_cb_double(rc, &config);
	pass &= test_err_load_schema_validation_cb_string(rc, &config);
	pass &= test_err_load_schema_validation_cb_mapping(rc, &config);
	pass &= test_err_load_schema_validation_cb_bitfield(rc, &config);
	pass &= test_err_load_schema_validation_cb_sequence(rc, &config);
	pass &= test_err_load_schema_validation_cb_sequence_fixed(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: string lengths");

	pass &= test_err_load_schema_string_min_length(rc, &config);
	pass &= test_err_load_schema_string_max_length(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: mapping fields");

	pass &= test_err_load_schema_missing_mapping_field(rc, &config);
	pass &= test_err_load_schema_unknown_mapping_field(rc, &config);
	pass &= test_err_load_schema_duplicate_mapping_field(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: sequence counts");

	pass &= test_err_load_schema_sequence_min_entries(rc, &config);
	pass &= test_err_load_schema_sequence_max_entries(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: bad flags/enum strings");

	pass &= test_err_load_schema_flags_mapping(rc, &config);
	pass &= test_err_load_schema_enum_bad_string(rc, &config);
	pass &= test_err_load_schema_flags_bad_string(rc, &config);
	pass &= test_err_save_schema_strict_enum_bad_value(rc, &config);
	pass &= test_err_load_schema_strict_enum_bad_string(rc, &config);
	pass &= test_err_save_schema_strict_flags_bad_value(rc, &config);
	pass &= test_err_load_schema_strict_flags_bad_string(rc, &config);

	ttest_heading(rc, "YAML / schema mismatch: expected value type tests");

	pass &= test_err_load_schema_expect_int_read_seq(rc, &config);
	pass &= test_err_load_schema_expect_int_read_end_1(rc, &config);
	pass &= test_err_load_schema_expect_int_read_end_2(rc, &config);
	pass &= test_err_load_schema_expect_flags_read_scalar(rc, &config);
	pass &= test_err_load_schema_expect_mapping_read_scalar(rc, &config);
	pass &= test_err_load_schema_expect_sequence_read_scalar(rc, &config);

	ttest_heading(rc, "Memory allocation handling tests");

	pass &= test_err_load_log(rc, &config);
	pass &= test_err_free_null(rc, &config);
	pass &= test_err_load_alloc_oom_1(rc, &config);
	pass &= test_err_load_alloc_oom_2(rc, &config);
	pass &= test_err_save_alloc_oom_1(rc, &config);
	pass &= test_err_save_alloc_oom_2(rc, &config);
	pass &= test_err_copy_alloc_oom_1(rc, &config);
	pass &= test_err_copy_alloc_oom_2(rc, &config);
	pass &= test_err_copy_alloc_oom_3(rc, &config);

	ttest_heading(rc, "Alias tests");

	pass &= test_err_load_invalid_alias(rc, &config);
	pass &= test_err_load_incomplete_alias(rc, &config);
	pass &= test_err_load_flag_value_alias(rc, &config);
	pass &= test_err_load_mapping_key_alias(rc, &config);
	pass &= test_err_load_mapping_value_alias_1(rc, &config);
	pass &= test_err_load_mapping_value_alias_2(rc, &config);
	pass &= test_err_load_mapping_value_alias_3(rc, &config);
	pass &= test_err_load_bitfield_value_alias_1(rc, &config);
	pass &= test_err_load_bitfield_value_alias_2(rc, &config);

	return pass;
}
