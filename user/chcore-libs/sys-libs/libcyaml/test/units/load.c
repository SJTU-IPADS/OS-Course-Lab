/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2021 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <cyaml/cyaml.h>

#include "../../src/data.h"
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
	cyaml_data_t **data;
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

	if (td->data != NULL) {
		cyaml_free(td->config, td->schema, *(td->data), seq_count);
	}
}

/**
 * Test loading a positive signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	static const unsigned char yaml[] =
		"test_int: 90\n";
	struct target_struct {
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a positive unsigned 8-bit integer with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_u8(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t test = 0x55;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(UINT, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a positive signed integer with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	const int default_value = -1;
	const unsigned trample = 0xdeadbeef;
	static const unsigned char yaml[] =
		"test_int: 90\n"
		"trample: 0xdeadbeef\n";
	struct target_struct {
		int test_value_int;
		int default_int;
		unsigned trample;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD(INT, "default", CYAML_FLAG_OPTIONAL,
				struct target_struct, default_int,
				{ .missing = default_value }),
		CYAML_FIELD_UINT("trample", CYAML_FLAG_DEFAULT,
				struct target_struct, trample),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (data_tgt->default_int != default_value) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->trample != trample) {
		return ttest_fail(&tc, "Trampled structure");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a boolean with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const bool test = true;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		bool test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(BOOL, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enum with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const enum enum_test {
		ENUM_TEST_BAT,
		ENUM_TEST_DOG,
		ENUM_TEST_CAT,
	} test = ENUM_TEST_CAT;
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = ENUM_TEST_CAT },
		{ .str = "bat", .val = ENUM_TEST_BAT },
		{ .str = "dog", .val = ENUM_TEST_DOG },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum enum_test test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(ENUM, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .strings = enum_test_schema,
				  .count = CYAML_ARRAY_LEN(enum_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading flags with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_flags(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const enum flags_test {
		FLAGS_TEST_BAT = 1 << 0,
		FLAGS_TEST_DOG = 1 << 1,
		FLAGS_TEST_CAT = 1 << 2,
	} test = FLAGS_TEST_CAT | FLAGS_TEST_BAT;
	static const cyaml_strval_t flags_test_schema[] = {
		{ .str = "cat", .val = FLAGS_TEST_CAT },
		{ .str = "bat", .val = FLAGS_TEST_BAT },
		{ .str = "dog", .val = FLAGS_TEST_DOG },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum flags_test test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(FLAGS, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .strings = flags_test_schema,
				  .count = CYAML_ARRAY_LEN(flags_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a float with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const float test = 3.14159f;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		float test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(FLOAT, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %f, got: %f)",
				test, data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a double precision float with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const double test = 3.14159;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		double test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(FLOAT, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %f, got: %f)",
				test, data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const char * const test = "My cat is best cat!";
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		char test[20];
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(STRING, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test, .min = 0, .max = 19 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (strcmp(data_tgt->test, test) != 0) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %s, got: %s)",
				test, data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint64_t test = 0xFFFFFFFFFFFFFFFFu;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint64_t test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(BITFIELD, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .bitdefs = bitvals,
				  .count = CYAML_ARRAY_LEN(bitvals)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_mapping_small(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const struct test_struct {
		uint8_t value;
	} test = {
		.value = 0x55,
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		struct test_struct test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_schema[] = {
		CYAML_FIELD_UINT("value", CYAML_FLAG_DEFAULT,
				struct test_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(MAPPING, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = &test,
				  .fields = test_schema,
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test.value != test.value) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %u, got: %u)",
				test.value, data_tgt->test.value);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_mapping_large(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static long values[] = { 4, 3, 2, 1 };
	const uint8_t before = 1;
	static const struct test_struct {
		char *a;
		char b[10];
		int c;
		long d[4];
		long *e;
		unsigned e_count;
		char *f;
		char *g;
		char h[10];
		int i;
		long j[4];
		long *k;
		unsigned k_count;
		bool l;
	} test = {
		.a = (char *) "Hello",
		.b = "World!",
		.c = 0,
		.d = { 0, 0, 0, 0 },
		.e = values,
		.e_count = CYAML_ARRAY_LEN(values),
		.f = (char *) "Required!",
		.g = NULL,
		.h = "\0",
		.i = 9876,
		.j = { 1, 2, 3, 4 },
		.k = NULL,
		.l = false,
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		struct test_struct test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, sizeof(long)),
	};
	static const struct cyaml_schema_field test_schema[] = {
		CYAML_FIELD_STRING_PTR("a",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("b", CYAML_FLAG_OPTIONAL,
				struct test_struct, b, 0),
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct test_struct, c),
		CYAML_FIELD_SEQUENCE_FIXED("d", CYAML_FLAG_OPTIONAL,
				struct test_struct, d, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("e",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, e, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("f",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, f, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("g",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, g, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("h", CYAML_FLAG_OPTIONAL,
				struct test_struct, h, 0),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct test_struct, i),
		CYAML_FIELD_SEQUENCE_FIXED("j", CYAML_FLAG_OPTIONAL,
				struct test_struct, j, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("k",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, k, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_BOOL("l",
				CYAML_FLAG_OPTIONAL,
				struct test_struct, l),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(MAPPING, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = &test,
				  .fields = test_schema,
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (strcmp(data_tgt->test.a, test.a) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry a: "
				"Expected: %s, got: %s",
				test.a, data_tgt->test.a);
	}
	if (strcmp(data_tgt->test.b, test.b) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->test.c != test.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test.d[i] != test.d[i]) {
			return ttest_fail(&tc, "Incorrect value for entry d");
		}
	}
	if (data_tgt->test.e_count != test.e_count) {
		return ttest_fail(&tc, "Bad sequence element count for e");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test.e[i] != test.e[i]) {
			return ttest_fail(&tc, "Incorrect value for entry e "
					"Index: %u: Expected: %ld, got: %ld",
					i, test.e[i], data_tgt->test.e[i]);
		}
	}
	if (strcmp(data_tgt->test.f, test.f) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry f: "
				"Expected: %s, got: %s",
				test.f, data_tgt->test.f);
	}
	if (data_tgt->test.g != test.g) {
		return ttest_fail(&tc, "Incorrect value for entry g: "
				"Expected: %s, got: %s",
				test.g, data_tgt->test.g);
	}
	if (strcmp(data_tgt->test.h, test.h) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry h");
	}
	if (data_tgt->test.i != test.i) {
		return ttest_fail(&tc, "Incorrect value for entry i");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test.j[i] != test.j[i]) {
			return ttest_fail(&tc, "Incorrect value for entry j");
		}
	}
	if (data_tgt->test.k != test.k) {
		return ttest_fail(&tc, "Incorrect value for entry k");
	}

	if (data_tgt->test.l != test.l) {
		return ttest_fail(&tc, "Incorrect value for entry l");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_sequence_small(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint8_t test[] = { 99 };
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t test[CYAML_ARRAY_LEN(test)];
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint8_t),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(SEQUENCE_FIXED, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .entry = &entry_schema,
				  .min = CYAML_ARRAY_LEN(test),
				  .max = CYAML_ARRAY_LEN(test),
				  .missing = test, }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	for (size_t i = 0; i < CYAML_ARRAY_LEN(test); i++) {
		if (data_tgt->test[i] != test[i]) {
			return ttest_fail(&tc, "Incorrect default value");
		}
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with a default value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_sequence_large(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint8_t test[] = {
		99, 98, 97, 96, 95, 94, 93, 92, 91, 90,
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, };
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t test[CYAML_ARRAY_LEN(test)];
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint8_t),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD(SEQUENCE_FIXED, "test", CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .entry = &entry_schema,
				  .min = CYAML_ARRAY_LEN(test),
				  .max = CYAML_ARRAY_LEN(test),
				  .missing = test, }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	for (size_t i = 0; i < CYAML_ARRAY_LEN(test); i++) {
		if (data_tgt->test[i] != test[i]) {
			return ttest_fail(&tc, "Incorrect default value");
		}
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for an unsigned integer pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_u8_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t test = 0x55;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(UINT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a signed integer pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_int_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	const int default_value = -1;
	static const unsigned char yaml[] =
		"test_int: 90\n";
	struct target_struct {
		int *test_value_int;
		int *default_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT_PTR("test_int", CYAML_FLAG_POINTER,
				struct target_struct, test_value_int),
		CYAML_FIELD_PTR(INT, "default",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, default_int,
				{ .missing = default_value }),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int == NULL) {
		return ttest_fail(&tc, "Expected allocation for value");
	}

	if (*data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (data_tgt->default_int == NULL) {
		return ttest_fail(&tc, "Expected allocated default value");
	}

	if (*data_tgt->default_int != default_value) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a boolean pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bool_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const bool test = true;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		bool *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(BOOL, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a enum pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_enum_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const enum enum_test {
		ENUM_TEST_BAT,
		ENUM_TEST_DOG,
		ENUM_TEST_CAT,
	} test = ENUM_TEST_CAT;
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = ENUM_TEST_CAT },
		{ .str = "bat", .val = ENUM_TEST_BAT },
		{ .str = "dog", .val = ENUM_TEST_DOG },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum enum_test *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(ENUM, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .strings = enum_test_schema,
				  .count = CYAML_ARRAY_LEN(enum_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a flags pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_flags_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const enum flags_test {
		FLAGS_TEST_BAT = 1 << 0,
		FLAGS_TEST_DOG = 1 << 1,
		FLAGS_TEST_CAT = 1 << 2,
	} test = FLAGS_TEST_CAT | FLAGS_TEST_BAT;
	static const cyaml_strval_t flags_test_schema[] = {
		{ .str = "cat", .val = FLAGS_TEST_CAT },
		{ .str = "bat", .val = FLAGS_TEST_BAT },
		{ .str = "dog", .val = FLAGS_TEST_DOG },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum flags_test *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLAGS, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .strings = flags_test_schema,
				  .count = CYAML_ARRAY_LEN(flags_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a float pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_float_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const float test = 3.14159f;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		float *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLOAT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %f, got: %f)",
				test, *data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a double precision float pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_double_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const double test = 3.14159;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		double *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLOAT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %f, got: %f)",
				test, *data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a string pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const char * const test = "Hello World!";
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		char *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(STRING, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .min = 0, .max = CYAML_UNLIMITED }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (strcmp(data_tgt->test, test) != 0) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %s, got: %s)",
				test, data_tgt->test);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a bitfield pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bitfield_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint64_t test = 0xFFFFFFFFFFFFFFFFu;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint64_t *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(BITFIELD, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = test,
				  .bitdefs = bitvals,
				  .count = CYAML_ARRAY_LEN(bitvals)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (*data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect default value");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a mapping pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_mapping_small_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const struct test_struct {
		uint8_t value;
	} test = {
		.value = 0x55,
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		struct test_struct *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_schema[] = {
		CYAML_FIELD_UINT("value", CYAML_FLAG_DEFAULT,
				struct test_struct, value),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(MAPPING, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = &test,
				  .fields = test_schema,
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (data_tgt->test->value != test.value) {
		return ttest_fail(&tc, "Incorrect default value "
				"(expected: %u, got: %u)",
				test.value, data_tgt->test->value);
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a mapping pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_mapping_large_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static long values[] = { 4, 3, 2, 1 };
	const uint8_t before = 1;
	static const struct test_struct {
		char *a;
		char b[10];
		int c;
		long d[4];
		long *e;
		unsigned e_count;
		char *f;
		char *g;
		char h[10];
		int i;
		long j[4];
		long *k;
		unsigned k_count;
		bool l;
	} test = {
		.a = (char *) "Hello",
		.b = "World!",
		.c = 0,
		.d = { 0, 0, 0, 0 },
		.e = values,
		.e_count = CYAML_ARRAY_LEN(values),
		.f = (char *) "Required!",
		.g = NULL,
		.h = "\0",
		.i = 9876,
		.j = { 1, 2, 3, 4 },
		.k = NULL,
		.l = false,
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		struct test_struct *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, sizeof(long)),
	};
	static const struct cyaml_schema_field test_schema[] = {
		CYAML_FIELD_STRING_PTR("a",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("b", CYAML_FLAG_OPTIONAL,
				struct test_struct, b, 0),
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct test_struct, c),
		CYAML_FIELD_SEQUENCE_FIXED("d", CYAML_FLAG_OPTIONAL,
				struct test_struct, d, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("e",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, e, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("f",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, f, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("g",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, g, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("h", CYAML_FLAG_OPTIONAL,
				struct test_struct, h, 0),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct test_struct, i),
		CYAML_FIELD_SEQUENCE_FIXED("j", CYAML_FLAG_OPTIONAL,
				struct test_struct, j, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("k",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct test_struct, k, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_BOOL("l",
				CYAML_FLAG_OPTIONAL,
				struct test_struct, l),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(MAPPING, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = &test,
				  .fields = test_schema,
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (strcmp(data_tgt->test->a, test.a) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry a: "
				"Expected: %s, got: %s",
				test.a, data_tgt->test->a);
	}
	if (strcmp(data_tgt->test->b, test.b) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->test->c != test.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test->d[i] != test.d[i]) {
			return ttest_fail(&tc, "Incorrect value for entry d");
		}
	}
	if (data_tgt->test->e_count != test.e_count) {
		return ttest_fail(&tc, "Bad sequence element count for e");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test->e[i] != test.e[i]) {
			return ttest_fail(&tc, "Incorrect value for entry e "
					"Index: %u: Expected: %ld, got: %ld",
					i, test.e[i], data_tgt->test->e[i]);
		}
	}
	if (strcmp(data_tgt->test->f, test.f) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry f: "
				"Expected: %s, got: %s",
				test.f, data_tgt->test->f);
	}
	if (data_tgt->test->g != test.g) {
		return ttest_fail(&tc, "Incorrect value for entry g: "
				"Expected: %s, got: %s",
				test.g, data_tgt->test->g);
	}
	if (strcmp(data_tgt->test->h, test.h) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry h");
	}
	if (data_tgt->test->i != test.i) {
		return ttest_fail(&tc, "Incorrect value for entry i");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->test->j[i] != test.j[i]) {
			return ttest_fail(&tc, "Incorrect value for entry j");
		}
	}
	if (data_tgt->test->k != test.k) {
		return ttest_fail(&tc, "Incorrect value for entry k");
	}

	if (data_tgt->test->l != test.l) {
		return ttest_fail(&tc, "Incorrect value for entry l");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a sequence pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_sequence_small_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint8_t test[] = { 99 };
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t *test;
		unsigned test_count;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint8_t),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(SEQUENCE, "test",
				CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
				struct target_struct, test,
				{ .entry = &entry_schema,
				  .min = CYAML_ARRAY_LEN(test),
				  .max = CYAML_ARRAY_LEN(test),
				  .missing = test,
				  .missing_count = CYAML_ARRAY_LEN(test) }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (data_tgt->test_count != CYAML_ARRAY_LEN(test)) {
		return ttest_fail(&tc, "Incorrect entry count");
	}

	for (size_t i = 0; i < CYAML_ARRAY_LEN(test); i++) {
		if (data_tgt->test[i] != test[i]) {
			return ttest_fail(&tc, "Incorrect default value");
		}
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a sequence pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_sequence_large_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const uint8_t test[] = {
		99, 98, 97, 96, 95, 94, 93, 92, 91, 90,
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, };
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t *test;
		unsigned test_count;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint8_t),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(SEQUENCE, "test",
				CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
				struct target_struct, test,
				{ .entry = &entry_schema,
				  .min = CYAML_ARRAY_LEN(test),
				  .max = CYAML_ARRAY_LEN(test),
				  .missing = test,
				  .missing_count = CYAML_ARRAY_LEN(test) }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get default allocation");
	}

	if (data_tgt->test_count != CYAML_ARRAY_LEN(test)) {
		return ttest_fail(&tc, "Incorrect entry count");
	}

	for (size_t i = 0; i < CYAML_ARRAY_LEN(test); i++) {
		if (data_tgt->test[i] != test[i]) {
			return ttest_fail(&tc, "Incorrect default value");
		}
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a positive unsigned 8-bit integer via pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_u8_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint8_t *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(UINT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a signed integer pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_int_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		int *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(INT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a boolean pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bool_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		bool *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(BOOL, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a enum pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_enum_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
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
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum enum_test *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(ENUM, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0,
				  .strings = enum_test_schema,
				  .count = CYAML_ARRAY_LEN(enum_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a flags pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_flags_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
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
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		enum flags_test *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLAGS, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0,
				  .strings = flags_test_schema,
				  .count = CYAML_ARRAY_LEN(flags_test_schema)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a float pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_float_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		float *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLOAT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a double precision float pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_double_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		double *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(FLOAT, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0 }),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a default value for a bitfield pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_field_default_bitfield_ptr_zero(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const uint8_t before = 1;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	const uint8_t after = 0xff;
	static const unsigned char yaml[] =
		"before: 1\n"
		"after: 0xff\n";
	struct target_struct {
		uint8_t before;
		uint64_t *test;
		uint8_t after;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("before", CYAML_FLAG_DEFAULT,
				struct target_struct, before),
		CYAML_FIELD_PTR(BITFIELD, "test",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, test,
				{ .missing = 0,
				  .bitdefs = bitvals,
				  .count = CYAML_ARRAY_LEN(bitvals)
				}),
		CYAML_FIELD_UINT("after", CYAML_FLAG_DEFAULT,
				struct target_struct, after),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->before != before) {
		return ttest_fail(&tc, "Incorrect value before default");
	}

	if (data_tgt->test != NULL) {
		return ttest_fail(&tc, "Shouldn't have an allocation");
	}

	if (data_tgt->after != after) {
		return ttest_fail(&tc, "Incorrect value after default");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a negative signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = -77;
	static const unsigned char yaml[] =
		"test_int: -77\n";
	struct target_struct {
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a positive signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_pos_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	static const unsigned char yaml[] =
		"test_int: 90\n";
	struct target_struct {
		int *test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT_PTR("test_int", CYAML_FLAG_POINTER,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a  negative signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_neg_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = -77;
	static const unsigned char yaml[] =
		"test_int: -77\n";
	struct target_struct {
		int *test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT_PTR("test_int", CYAML_FLAG_POINTER,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned value = 9999;
	static const unsigned char yaml[] =
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_uint != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_uint_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned value = 9999;
	static const unsigned char yaml[] =
		"test_uint: 9999\n";
	struct target_struct {
		unsigned *test_value_uint;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT_PTR("test_uint", CYAML_FLAG_POINTER,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_uint != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	float value = 3.14159f;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		float test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %f, got: %f",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_float_underflow(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	float value = 1.55331e-40f;
	static const unsigned char yaml[] =
		"test_fp: 1.55331e-40\n";
	struct target_struct {
		float test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %e, got: %e",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a pointer to float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_float_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	float value = 3.14159f;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		float *test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT_PTR("test_fp", CYAML_FLAG_POINTER,
				struct target_struct, test_value_fp),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	double value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		double test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_double_underflow(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	double value = 1.79769e+308;
	static const unsigned char yaml[] =
		"test_fp: 1.79769e+309\n";
	struct target_struct {
		double test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
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

	value *= 10;
	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a pointer to double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_double_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	double value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		double *test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT_PTR("test_fp", CYAML_FLAG_POINTER,
				struct target_struct, test_value_fp),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a boolean value (true).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_true(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = true;
	static const unsigned char yaml[] =
		"test_bool: true\n";
	struct target_struct {
		unsigned test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bool),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a boolean value (false).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_false(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = false;
	static const unsigned char yaml[] =
		"test_bool: false\n";
	struct target_struct {
		unsigned test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bool),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a boolean value (true).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_true_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = true;
	static const unsigned char yaml[] =
		"test_bool: true\n";
	struct target_struct {
		unsigned *test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL_PTR("test_bool", CYAML_FLAG_POINTER,
				struct target_struct, test_value_bool),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a boolean value (false).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_false_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = false;
	static const unsigned char yaml[] =
		"test_bool: false\n";
	struct target_struct {
		unsigned *test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL_PTR("test_bool", CYAML_FLAG_POINTER,
				struct target_struct, test_value_bool),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_enum,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to an enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum *test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM_PTR("test_enum", CYAML_FLAG_POINTER,
				struct target_struct, test_value_enum,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sparse enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST  = 3,
		TEST_ENUM_SECOND = 77,
		TEST_ENUM_THIRD  = 183,
		TEST_ENUM_FOURTH = 9900,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[] = {
		{ "first",  TEST_ENUM_FIRST },
		{ "second", TEST_ENUM_SECOND },
		{ "third",  TEST_ENUM_THIRD },
		{ "fourth", TEST_ENUM_FOURTH },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_enum,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enumeration with numerical fallback.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum_fallback(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST  = 3,
		TEST_ENUM_SECOND = 77,
		TEST_ENUM_THIRD  = 183,
		TEST_ENUM_FOURTH = 9900,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[] = {
		{ "first",  TEST_ENUM_FIRST },
		{ "second", TEST_ENUM_SECOND },
		{ "third",  TEST_ENUM_THIRD },
		{ "fourth", TEST_ENUM_FOURTH },
	};
	static const unsigned char yaml[] =
		"test_enum: 77\n";
	struct target_struct {
		enum test_enum test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_enum,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string to a character array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = "Hello World!";
	static const unsigned char yaml[] =
		"test_string: Hello World!\n";
	struct target_struct {
		char test_value_string[50];
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING("test_string", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_string, 0),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, value) != 0) {
		fprintf(stderr, "expected: %s\n", value);
		for (unsigned i = 0; i < strlen(value) + 1; i++) {
			fprintf(stderr, "%2.2x ", value[i]);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "     got: %s\n", data_tgt->test_value_string);
		for (unsigned i = 0; i < sizeof(data_tgt->test_value_string); i++) {
			fprintf(stderr, "%2.2x ", value[i]);
		}
		fprintf(stderr, "\n");
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string to an allocated char pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = "null";
	static const unsigned char yaml[] =
		"test_string: null\n";
	struct target_struct {
		char * test_value_string;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an empty string to an allocated char pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string_ptr_empty(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = "";
	static const unsigned char yaml[] =
		"test_string:\n";
	struct target_struct {
		char * test_value_string;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a null string to an allocated nullable char pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string_ptr_null_str(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = NULL;
	static const unsigned char yaml[] =
		"test_string: null\n";
	struct target_struct {
		char * test_value_string;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string",
				CYAML_FLAG_POINTER_NULL_STR,
				struct target_struct, test_value_string,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_string != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an empty string to an allocated nullable char pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string_ptr_null_empty(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = NULL;
	static const unsigned char yaml[] =
		"test_string:\n";
	struct target_struct {
		char * test_value_string;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER_NULL,
				struct target_struct, test_value_string,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_string != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an ignored value with descendants.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_ignore_deep(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"ignore:\n"
		"    foo: bar\n"
		"    bar:\n"
		"      - 1\n"
		"      - 2\n";
	struct target_struct {
		bool foo;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->foo != false) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an ignored value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_ignore_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"ignore: foo\n";
	struct target_struct {
		bool foo;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->foo != false) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an optional ignored value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_ignore_optional_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"ignore: foo\n";
	struct target_struct {
		bool foo;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
		CYAML_FIELD_IGNORE("optional", CYAML_FLAG_OPTIONAL),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->foo != false) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags(
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
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n"
		"    - 1024\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_ptr(
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
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n"
		"    - 1024\n";
	struct target_struct {
		enum test_flags *test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS_PTR("test_flags", CYAML_FLAG_POINTER,
				struct target_struct, test_value_flags,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an empty flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_empty(
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
	} value = TEST_FLAGS_NONE;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags: []\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sparse flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 <<  0),
		TEST_FLAGS_SECOND = (1 <<  4),
		TEST_FLAGS_THIRD  = (1 <<  7),
		TEST_FLAGS_FOURTH = (1 << 11),
		TEST_FLAGS_FIFTH  = (1 << 14),
		TEST_FLAGS_SIXTH  = (1 << 20),
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	uint64_t value = 0xFFFFFFFFFFFFFFFFu;
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bitfield != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_bitfield);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a bitfield.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bitfield_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	uint64_t value = 0xFFFFFFFFFFFFFFFFu;
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
		uint64_t *test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD_PTR("test_bitfield", CYAML_FLAG_POINTER,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bitfield != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_bitfield);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping, to an internal structure.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} value;
	static const unsigned char yaml[] =
		"mapping:\n"
		"    a: 123\n"
		"    b: 9999\n";
	struct target_struct {
		struct value_s test_value_mapping;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING("mapping", CYAML_FLAG_DEFAULT,
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

	memset(&value, 0, sizeof(value));
	value.a = 123;
	value.b = 9999;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (memcmp(&data_tgt->test_value_mapping, &value, sizeof(value)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping, to an allocated structure.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} value;
	static const unsigned char yaml[] =
		"mapping:\n"
		"    a: 123\n"
		"    b: 9999\n";
	struct target_struct {
		struct value_s *test_value_mapping;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING_PTR("mapping", CYAML_FLAG_POINTER,
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

	memset(&value, 0, sizeof(value));
	value.a = 123;
	value.b = 9999;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (memcmp(data_tgt->test_value_mapping, &value, sizeof(value)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers into an int[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of enums into an enum test_enum[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref[] = { TEST_ENUM_FIRST, TEST_ENUM_SECOND, TEST_ENUM_THIRD };
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - first\n"
		"    - second\n"
		"    - third\n";
	struct target_struct {
		enum test_enum seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT,
				*(data_tgt->seq), strings, TEST_ENUM__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of unsigned integers into an unsigned[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of boolean values into an bool[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool ref[] = { true, false, true, false, true, false, true, false };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - true\n"
		"    - false\n"
		"    - yes\n"
		"    - no\n"
		"    - enable\n"
		"    - disable\n"
		"    - 1\n"
		"    - 0\n";
	struct target_struct {
		bool seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of flag sequences into an array of flag words.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_flags(
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
	} ref[3] = {
		TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
		TEST_FLAGS_FIRST,
		TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH
	};
	#define TEST_FLAGS__COUNT 6
	static const cyaml_strval_t strings[TEST_FLAGS__COUNT] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - - second\n"
		"      - fifth\n"
		"      - 1024\n"
		"    - - first\n"
		"    - - fourth\n"
		"      - sixth\n";
	struct target_struct {
		enum test_flags seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data_tgt->seq),
				strings, TEST_FLAGS__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings into an array of char[7].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char seq[5][7];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data_tgt->seq), 0, 6),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings into an array of allocated strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char *seq[5];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data_tgt->seq),
				0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings into an array of structures.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3];
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
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

	memset(ref, 0, sizeof(ref));
	ref[0].a = 123;
	ref[0].b = 9999;
	ref[1].a = 4000;
	ref[1].b = 62000;
	ref[2].a = 1;
	ref[2].b = 765;

	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	if (memcmp(data_tgt->seq, ref, sizeof(ref)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings into an array of pointers to structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s *seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i]->a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i]->b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into int[4][3].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int seq[4][3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data_tgt->seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, **(data_tgt->seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into int*[4].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int *seq[4];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data_tgt->seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, **(data_tgt->seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into one-dimensional int[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int seq[12];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(int[3]),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,
				}
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_size = sizeof(data_tgt->seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/* Note: count is count of entries of the outer sequence entries,
	 * so, 4, not 12. */
	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i],
						ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers to allocated int* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of enums to allocated enum test_enum* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref[] = { TEST_ENUM_FIRST, TEST_ENUM_SECOND, TEST_ENUM_THIRD };
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - first\n"
		"    - second\n"
		"    - third\n";
	struct target_struct {
		enum test_enum *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT,
				*(data_tgt->seq), strings, TEST_ENUM__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of unsigned integers to allocated unsigned* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of boolean values to allocated bool* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool ref[] = { true, false, true, false, true, false, true, false };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - true\n"
		"    - false\n"
		"    - yes\n"
		"    - no\n"
		"    - enable\n"
		"    - disable\n"
		"    - 1\n"
		"    - 0\n";
	struct target_struct {
		bool *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of flag sequences to allocated flag words array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_flags(
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
	} ref[3] = {
		TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
		TEST_FLAGS_FIRST,
		TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH
	};
	#define TEST_FLAGS__COUNT 6
	static const cyaml_strval_t strings[TEST_FLAGS__COUNT] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - - second\n"
		"      - fifth\n"
		"      - 1024\n"
		"    - - first\n"
		"    - - fourth\n"
		"      - sixth\n";
	struct target_struct {
		enum test_flags *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data_tgt->seq),
				strings, TEST_FLAGS__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings to allocated array of char[7].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char (*seq)[7];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data_tgt->seq), 0, 6),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings to allocated array of
 * allocated strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data_tgt->seq),
				0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings to allocated array mapping structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i].a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i].b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings to allocated array of pointers to
 * mapping structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i]->a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i]->b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers to allocated array
 * of int[3].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int (*seq)[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers to allocated array
 * of allocated arrays of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers a one-dimensional allocated
 * array of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(int[3]),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,
				}
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_size = sizeof(data_tgt->seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/* Note: count is count of entries of the outer sequence entries,
	 * so, 4, not 12. */
	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i],
						ref[j][i]);
			}
		}
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
static bool test_load_mapping_entry_int8_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = 127;
	static const unsigned char yaml[] =
		"val: 127\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi8", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int8_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = -128;
	static const unsigned char yaml[] =
		"val: -128\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi8", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int16_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = -32768;
	static const unsigned char yaml[] =
		"val: -32768\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi16", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int16_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = 32767;
	static const unsigned char yaml[] =
		"val: 32767\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi16", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int32_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = -2147483648;
	static const unsigned char yaml[] =
		"val: -2147483648\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi32", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int32_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = 2147483647;
	static const unsigned char yaml[] =
		"val: 2147483647\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi32", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int64_limit_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = -9223372036854775807ll;
	static const unsigned char yaml[] =
		"val: -9223372036854775807\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi64", expecting %"PRIi64")",
				data_tgt->val, value);
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
static bool test_load_mapping_entry_int64_limit_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int64_t value = 9223372036854775807;
	static const unsigned char yaml[] =
		"val: 9223372036854775807\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->val != value) {
		return ttest_fail(&tc, "Incorrect value "
				"(got %"PRIi64", expecting %"PRIi64")",
				data_tgt->val, value);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading sequence of pointers to integer values with NULLs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_null_str_values_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int expected[] = {
		777, 6, 5, 4, 3, 2, 1, 0,
	};
	static const bool expected_nulls[] = {
		false, false, false, true, false, false, true, false,
	};
	static const unsigned char yaml[] =
		"- 777\n"
		"- 6\n"
		"- 5\n"
		"- ~\n"
		"- 3\n"
		"- 2\n"
		"- null\n"
		"- 0\n";
	int **value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER_NULL_STR, **value)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, *value,
				&entry_schema, 0, CYAML_UNLIMITED)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != CYAML_ARRAY_LEN(expected)) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(expected); i ++) {
		if (expected_nulls[i]) {
			if (value[i] != NULL) {
				return ttest_fail(&tc, "Expected NULL.");
			}
			continue;
		} else {
			if (value[i] == NULL) {
				return ttest_fail(&tc, "Unexpected NULL.");
			}
			if ((*(value)[i]) != expected[i]) {
				return ttest_fail(&tc, "Bad value.");
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading sequence of pointers to integer values with NULLs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_null_values_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int expected[] = {
		777, 6, 5, 4, 3, 2, 1, 0,
	};
	static const bool expected_nulls[] = {
		false, false, false, true, false, false, true, false,
	};
	static const unsigned char yaml[] =
		"- 777\n"
		"- 6\n"
		"- 5\n"
		"- \n"
		"- 3\n"
		"- 2\n"
		"- \n"
		"- 0\n";
	int **value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER_NULL, **value)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, *value,
				&entry_schema, 0, CYAML_UNLIMITED)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != CYAML_ARRAY_LEN(expected)) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(expected); i ++) {
		if (expected_nulls[i]) {
			if (value[i] != NULL) {
				return ttest_fail(&tc, "Expected NULL.");
			}
			continue;
		} else {
			if (value[i] == NULL) {
				return ttest_fail(&tc, "Unexpected NULL.");
			}
			if ((*(value)[i]) != expected[i]) {
				return ttest_fail(&tc, "Bad value.");
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading sequence of pointers to integer values with NULLs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_null_str_values_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned int expected[] = {
		7, 6, 5555, 4, 3, 2, 1, 0,
	};
	static const bool expected_nulls[] = {
		false, true, false, true, false, false, true, false,
	};
	static const unsigned char yaml[] =
		"- 7\n"
		"- \n"
		"- 5555\n"
		"- Null\n"
		"- 3\n"
		"- 2\n"
		"- NULL\n"
		"- 0\n";
	unsigned int **value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_POINTER_NULL_STR,
				**value)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, *value,
				&entry_schema, 0, CYAML_UNLIMITED)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != CYAML_ARRAY_LEN(expected)) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(expected); i ++) {
		if (expected_nulls[i]) {
			if (value[i] != NULL) {
				return ttest_fail(&tc, "Expected NULL.");
			}
			continue;
		} else {
			if (value[i] == NULL) {
				return ttest_fail(&tc, "Unexpected NULL.");
			}
			if ((*(value)[i]) != expected[i]) {
				return ttest_fail(&tc, "Bad value.");
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading sequence of pointers to mapping values with NULLs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_null_str_values_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct entry_value {
		int a;
		int b;
	};
	static const struct entry_value expected[] = {
		{ .a = 7 },
		{ .a = 6 },
		{ .a = 5555 },
		{ .a = 4 },
		{ .a = 3 },
		{ .b = 2 },
		{ .a = 1 },
		{ .a = 0 },
	};
	static const bool expected_nulls[] = {
		false, true, false, true, false, false, true, false,
	};
	static const unsigned char yaml[] =
		"- a: 7\n"
		"- \n"
		"- a: 5555\n"
		"- Null\n"
		"- a: 3\n"
		"- b: 2\n"
		"- NULL\n"
		"- a: 0\n";
	struct entry_value **value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_OPTIONAL,
				struct entry_value, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_OPTIONAL,
				struct entry_value, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER_NULL_STR,
				**value, mapping_schema)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, *value,
				&entry_schema, 0, CYAML_UNLIMITED)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != CYAML_ARRAY_LEN(expected)) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(expected); i ++) {
		if (expected_nulls[i]) {
			if (value[i] != NULL) {
				return ttest_fail(&tc, "Expected NULL.");
			}
			continue;
		} else {
			if (value[i] == NULL) {
				return ttest_fail(&tc, "Unexpected NULL.");
			}
			if (memcmp(&(*(value)[i]), &expected[i],
					sizeof(**value)) != 0) {
				return ttest_fail(&tc, "Bad value.");
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 1 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[1];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 2 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[2];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 3 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[3];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 4 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[4];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 5 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[5];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 6 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_6(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[6];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 7 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_7(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[7];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 8 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_8(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[8];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with scalar top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"7\n";
	int *value = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if (*value != 7) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with string top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"Hello\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if (strcmp(value, "Hello") != 0) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence_fixed top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"- 7\n"
		"- 6\n"
		"- 5\n";
	int *value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, CYAML_UNLIMITED)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != 3) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if ((value[0] != 7) && (value[1] != 6) && (value[2] != 5)) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence_fixed top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_sequence_fixed(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"- 7\n"
		"- 6\n"
		"- 5\n";
	int *value = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(CYAML_FLAG_POINTER, int,
				&entry_schema, 3)
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if ((value[0] != 7) && (value[1] != 6) && (value[2] != 5)) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a stream with more than one document.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_multiple_documents_ignored(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
	} data = {
		.a = 9,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"---\n"
		"b: foo\n";
	struct target_struct *data_tgt = NULL;
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping multiple fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_with_multiple_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: 900\n"
		"d: 9000\n"
		"e: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("d", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("e", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with optional fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_with_optional_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	long values[] = { 4, 3, 2, 1 };
	const struct target_struct {
		char *a;
		char b[10];
		int c;
		long d[4];
		long *e;
		unsigned e_count;
		char *f;
		char *g;
		char h[10];
		int i;
		long j[4];
		long *k;
		unsigned k_count;
		bool l;
	} data = {
		.a = (char *) "Hello",
		.b = "World!",
		.c = 0,
		.d = { 0, 0, 0, 0 },
		.e = values,
		.e_count = CYAML_ARRAY_LEN(values),
		.f = (char *) "Required!",
		.g = NULL,
		.h = "\0",
		.i = 9876,
		.j = { 1, 2, 3, 4 },
		.k = NULL,
		.l = false,
	};
	static const unsigned char yaml[] =
		"a: Hello\n"
		"b: World!\n"
		"e: [ 4, 3, 2, 1 ]\n"
		"f: Required!\n"
		"i: 9876\n"
		"j: [ 1, 2, 3, 4 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, sizeof(long)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("b", CYAML_FLAG_OPTIONAL,
				struct target_struct, b, 0),
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct target_struct, c),
		CYAML_FIELD_SEQUENCE_FIXED("d", CYAML_FLAG_OPTIONAL,
				struct target_struct, d, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("e",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, e, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("f",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, f, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("g",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, g, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("h", CYAML_FLAG_OPTIONAL,
				struct target_struct, h, 0),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct target_struct, i),
		CYAML_FIELD_SEQUENCE_FIXED("j", CYAML_FLAG_OPTIONAL,
				struct target_struct, j, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("k",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, k, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_BOOL("l",
				CYAML_FLAG_OPTIONAL,
				struct target_struct, l),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->a, data.a) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry a: "
				"Expected: %s, got: %s",
				data.a, data_tgt->a);
	}
	if (strcmp(data_tgt->b, data.b) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->d[i] != data.d[i]) {
			return ttest_fail(&tc, "Incorrect value for entry d");
		}
	}
	if (data_tgt->e_count != data.e_count) {
		return ttest_fail(&tc, "Bad sequence element count for e");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->e[i] != data.e[i]) {
			return ttest_fail(&tc, "Incorrect value for entry e "
					"Index: %u: Expected: %ld, got: %ld",
					i, data.e[i], data_tgt->e[i]);
		}
	}
	if (strcmp(data_tgt->f, data.f) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry f: "
				"Expected: %s, got: %s",
				data.f, data_tgt->f);
	}
	if (data_tgt->g != data.g) {
		return ttest_fail(&tc, "Incorrect value for entry g: "
				"Expected: %s, got: %s",
				data.g, data_tgt->g);
	}
	if (strcmp(data_tgt->h, data.h) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry h");
	}
	if (data_tgt->i != data.i) {
		return ttest_fail(&tc, "Incorrect value for entry i");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->j[i] != data.j[i]) {
			return ttest_fail(&tc, "Incorrect value for entry j");
		}
	}
	if (data_tgt->k != data.k) {
		return ttest_fail(&tc, "Incorrect value for entry k");
	}

	if (data_tgt->l != data.l) {
		return ttest_fail(&tc, "Incorrect value for entry l");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with only optional fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_only_optional_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int c;
		int i;
	};
	static const unsigned char yaml[] =
		"\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct target_struct, c),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct target_struct, i),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Shouldn't have allocated anything");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with only optional fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_without_any_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int i;
	};
	static const unsigned char yaml[] =
		"{}\n";
	struct target_struct *data_tgt = NULL;
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
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt == NULL) {
		return ttest_fail(&tc, "Should have allocated empty structure");
	}

	if (data_tgt->i != 0) {
		return ttest_fail(&tc, "Value should be initialised to 0");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with unknown keys ignored by config.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_ignored_unknown_keys(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: 900\n"
		"d: 9000\n"
		"e: 90000\n"
		"f: 900000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("d", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("e", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
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

	cfg.flags |= CYAML_CFG_IGNORE_UNKNOWN_KEYS;
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with unknown keys ignored by config.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_warn_ignored_keys(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: 900\n"
		"d: 9000\n"
		"e: 90000\n"
		"f: 900000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("d", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("e", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
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

	cfg.flags |= CYAML_CFG_IGNORE_UNKNOWN_KEYS |
	             CYAML_CFG_IGNORED_KEY_WARNING;
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with max size 4, and only 2 entries in YAML.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_without_max_entries(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		const char *seq[4];
		unsigned seq_count;
	} data = {
		.seq = { "1", "2", NULL, NULL },
		.seq_count = 2,
	};
	static const unsigned char yaml[] =
		"seq: [ 1, 2 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char *,
				0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("seq", CYAML_FLAG_OPTIONAL,
				struct target_struct, seq, &sequence_entry,
				0, 4),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->seq_count != 2) {
		return ttest_fail(&tc, "Incorrect sequence entry count");
	}
	for (unsigned i = 0; i < 4; i++) {
		if ((data_tgt->seq[i] == NULL) != (data.seq[i] == NULL)) {
			return ttest_fail(&tc, "Incorrect value for entry %u",
					i);
		}
		if (data_tgt->seq[i] != NULL) {
			if (strcmp(data_tgt->seq[i], data.seq[i]) != 0) {
				return ttest_fail(&tc,
						"Incorrect value for entry %u",
						i);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading without a logging function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_no_log(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		const char *seq[4];
		unsigned seq_count;
	} data = {
		.seq = { "1", "2", NULL, NULL },
		.seq_count = 2,
	};
	static const unsigned char yaml[] =
		"seq: [ 1, 2 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char *,
				0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("seq", CYAML_FLAG_OPTIONAL,
				struct target_struct, seq, &sequence_entry,
				0, 4),
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

	cfg.log_fn = NULL;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->seq_count != 2) {
		return ttest_fail(&tc, "Incorrect sequence entry count");
	}
	for (unsigned i = 0; i < 4; i++) {
		if ((data_tgt->seq[i] == NULL) != (data.seq[i] == NULL)) {
			return ttest_fail(&tc, "Incorrect value for entry %u",
					i);
		}
		if (data_tgt->seq[i] != NULL) {
			if (strcmp(data_tgt->seq[i], data.seq[i]) != 0) {
				return ttest_fail(&tc,
						"Incorrect value for entry %u",
						i);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with duplicate ignored mapping fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_duplicate_ignored(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	static const unsigned char yaml[] =
		"ignore: 90\n"
		"ignore: 90\n"
		"test_int: 90\n";
	struct target_struct {
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with arbitrary C struct member name for entry count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_sequence_entry_count_member(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned *entries;
		uint32_t n_entries;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->entries)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE_COUNT("sequence", CYAML_FLAG_POINTER,
				struct target_struct, entries, n_entries,
				&entry_schema, 0, CYAML_UNLIMITED),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->n_entries) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->entries[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->entries[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enum with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_enum_insensitive(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		{ "first",  TEST_ENUM_FIRST  },
		{ "second", TEST_ENUM_SECOND },
		{ "third",  TEST_ENUM_THIRD  },
	};
	static const unsigned char yaml[] =
		"SECOND\n";
	enum test_enum *data_tgt = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_POINTER,
				*data_tgt, strings, TEST_ENUM__COUNT),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt != ref) {
		return ttest_fail(&tc, "Incorrect value for enum");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flags value with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_flags_insensitive(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_FLAG_FIRST  = (1 << 1),
		TEST_FLAG_SECOND = (1 << 3),
		TEST_FLAG_THIRD  = (1 << 5),
	} ref = TEST_FLAG_FIRST | TEST_FLAG_THIRD;
	static const cyaml_strval_t strings[] = {
		{ "first",  TEST_FLAG_FIRST  },
		{ "second", TEST_FLAG_SECOND },
		{ "third",  TEST_FLAG_THIRD  },
	};
	static const unsigned char yaml[] =
		"- First\n"
		"- Third\n";
	enum test_enum *data_tgt = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_POINTER,
				*data_tgt, strings, CYAML_ARRAY_LEN(strings)),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt != ref) {
		return ttest_fail(&tc, "Incorrect value for enum");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Lollipop: 9\n"
		"Squiggle: 90\n"
		"Unicorns: 900\n"
		"Cheerful: 9000\n"
		"LibCYAML: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("lollipop", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("squiggle", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("unicorns", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("cheerful", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("libcyaml", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Plinth: 9\n"
		"..Cusp?: 90\n"
		"..Cusp!: 900\n"
		"Bleat: 9000\n"
		"Foo~-|Bar: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plinth", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("..cusp?", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("..cusp!", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("bleat", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("foO~-|baR", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Pling: 9\n"
		"Plin: 90\n"
		"Pli: 900\n"
		"Pl: 9000\n"
		"P: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plin", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("pli", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pl", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("p", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case sensitive string matching for value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_value_sensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"pling: 9\n"
		"PLing: 90\n"
		"PLINg: 900\n"
		"pliNG: 9000\n"
		"PLING: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("PLing", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("PLINg", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pliNG", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("PLING", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(
				CYAML_FLAG_POINTER | CYAML_FLAG_CASE_SENSITIVE,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching for value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_value_insensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Pling: 9\n"
		"Plin: 90\n"
		"Pli: 900\n"
		"Pl: 9000\n"
		"P: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plin", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("pli", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pl", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("p", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(
				CYAML_FLAG_POINTER |
				CYAML_FLAG_CASE_INSENSITIVE,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;
	ttest_ctx_t tc;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	if (!ttest_start(report, __func__, cyaml_cleanup, &td, &tc)) {
		return true;
	}

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with an unused anchor.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_unused_anchor(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *string_value = "Hello World!";
	const int int_value = 9;
	static const unsigned char yaml[] =
		"test_string: &foo Hello World!\n"
		"test_int: 9\n";
	struct target_struct {
		char * test_value_string;
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, string_value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (data_tgt->test_value_int != int_value) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_anchor_scalar_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *string_value = "Hello World!";
	const int int_value = 9;
	static const unsigned char yaml[] =
		"test_int_anchor: &foo 9\n"
		"test_string: Hello World!\n"
		"test_int: *foo\n";
	struct target_struct {
		char * test_value_string;
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("test_int_anchor", CYAML_FLAG_OPTIONAL),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, string_value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (data_tgt->test_value_int != int_value) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_anchor_scalar_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *string_value = "Hello World!";
	const int int_value = 9;
	static const unsigned char yaml[] =
		"test_string_anchor: &foo Hello World!\n"
		"test_string: *foo\n"
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, string_value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (data_tgt->test_value_int != int_value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading multiple anchored and alisased scalars.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_anchor_multiple_scalars(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *string_value1 = "Hello Me!";
	const char *string_value2 = "Hello World!";
	const int int_value = 99;
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 Hello World!\n"
		"  - &a2 Hello Me!\n"
		"  - &a3 99\n"
		"test_string1: *a2\n"
		"test_int: *a3\n"
		"test_string2: *a1\n";
	struct target_struct {
		char * test_value_string1;
		char * test_value_string2;
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_STRING_PTR("test_string1", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string1,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("test_string2", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string2,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string1, string_value1) != 0) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %s, got: %s",
				string_value1, data_tgt->test_value_string1);
	}

	if (strcmp(data_tgt->test_value_string2, string_value2) != 0) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %s, got: %s",
				string_value2, data_tgt->test_value_string2);
	}

	if (data_tgt->test_value_int != int_value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %i, got: %i",
				int_value, data_tgt->test_value_int);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an aliased mapping.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_anchor_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *test_a = "Hello Me!";
	const int test_b = 777;
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a2 Hello Me!\n"
		"  - &a1 {\n"
		"      a: *a2,\n"
		"      b: 777,\n"
		"    }\n"
		"test: *a1\n";
	struct my_test {
		int b;
		char *a;
	};
	struct target_struct {
		struct my_test test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field inner_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct my_test, a,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct my_test, b),
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test.a, test_a) != 0) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %s, got: %s",
				test_a, data_tgt->test.a);
	}

	if (data_tgt->test.b != test_b) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %i, got: %i",
				test_b, data_tgt->test.b);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an aliased sequence.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_anchor_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const int test_a[] = { 1, 22, 333, 4444 };
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 [\n"
		"      1,\n"
		"      22,\n"
		"      333,\n"
		"      4444,\n"
		"    ]\n"
		"test: *a1\n";
	struct target_struct {
		int *a;
		unsigned a_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_SEQUENCE("test", CYAML_FLAG_POINTER,
				struct target_struct, a,
				&sequence_entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a_count != CYAML_ARRAY_LEN(test_a)) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %u, got: %u",
				data_tgt->a_count, CYAML_ARRAY_LEN(test_a));
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(test_a); i++) {
		if (data_tgt->a[i] != test_a[i]) {
			return ttest_fail(&tc, "Incorrect value: "
					"expected: %i, got: %i",
					data_tgt->a[i], test_a[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with anchors within anchors, etc.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_anchor_deep_mapping_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *test_a = "Hello Me!";
	const int test_b[] = { 1, 22, 333, 4444 };
	static const unsigned char yaml[] =
		"anchors:\n"
		"  - &a1 Hello Me!\n"
		"  - &a2 {\n"
		"      a: *a1,\n"
		"      b: &a3 [ 1, 22, 333, 4444 ],\n"
		"      c: *a1,\n"
		"      d: *a3,\n"
		"    }\n"
		"test_a: *a2\n"
		"test_b: *a3\n";
	struct my_test {
		char *a;
		int *b;
		unsigned b_count;
		char *c;
		int *d;
		unsigned d_count;
	};
	struct target_struct {
		struct my_test test_a;
		int *test_b;
		unsigned test_b_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field inner_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct my_test, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("b", CYAML_FLAG_POINTER, struct my_test, b,
				&sequence_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("c", CYAML_FLAG_POINTER,
				struct my_test, c, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("d", CYAML_FLAG_POINTER, struct my_test, d,
				&sequence_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("anchors", CYAML_FLAG_OPTIONAL),
		CYAML_FIELD_MAPPING("test_a", CYAML_FLAG_DEFAULT,
				struct target_struct, test_a,
				inner_mapping_schema),
		CYAML_FIELD_SEQUENCE("test_b", CYAML_FLAG_POINTER,
				struct target_struct, test_b,
				&sequence_entry_schema,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_a.a, test_a) != 0) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %s, got: %s",
				test_a, data_tgt->test_a.a);
	}

	if (strcmp(data_tgt->test_a.c, test_a) != 0) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %s, got: %s",
				test_a, data_tgt->test_a.c);
	}

	if (data_tgt->test_b_count != CYAML_ARRAY_LEN(test_b)) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %u, got: %u",
				data_tgt->test_b_count,
				CYAML_ARRAY_LEN(test_b));
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(test_b); i++) {
		if (data_tgt->test_b[i] != test_b[i]) {
			return ttest_fail(&tc, "Incorrect value: "
					"expected: %i, got: %i",
					data_tgt->test_b[i], test_b[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading when an anchor is updated.
 *
 * The newest definition should be used.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_anchor_updated_anchor(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *string_value1 = "Hello Me!";
	const char *string_value2 = "Hello World!";
	static const unsigned char yaml[] =
		"a: &a1 Hello Me!\n"
		"b: *a1\n"
		"c: &a1 Hello World!\n"
		"d: *a1\n";
	struct target_struct {
		char *a;
		char *b;
		char *c;
		char *d;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a", CYAML_FLAG_POINTER,
				struct target_struct, a,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("b", CYAML_FLAG_POINTER,
				struct target_struct, b,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("c", CYAML_FLAG_POINTER,
				struct target_struct, c,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("d", CYAML_FLAG_POINTER,
				struct target_struct, d,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->a, string_value1) != 0) {
		return ttest_fail(&tc, "Incorrect value (a): "
				"expected: %s, got: %s",
				string_value1, data_tgt->a);
	}

	if (strcmp(data_tgt->b, string_value1) != 0) {
		return ttest_fail(&tc, "Incorrect value (b): "
				"expected: %s, got: %s",
				string_value1, data_tgt->b);
	}

	if (strcmp(data_tgt->c, string_value2) != 0) {
		return ttest_fail(&tc, "Incorrect value (c): "
				"expected: %s, got: %s",
				string_value2, data_tgt->c);
	}

	if (strcmp(data_tgt->d, string_value2) != 0) {
		return ttest_fail(&tc, "Incorrect value (d): "
				"expected: %s, got: %s",
				string_value2, data_tgt->d);
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
static bool test_load_mapping_field_range_int_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int test = 90;
	static const unsigned char yaml[] =
		"test: 90\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = INT64_MIN,
					.max = INT64_MAX,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_int_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int test = -9000;
	static const unsigned char yaml[] =
		"test: -9000\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = INT64_MIN,
					.max = INT64_MAX,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_int_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int test = -9000;
	static const unsigned char yaml[] =
		"test: -9000\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = INT64_MIN,
					.max = 0,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_int_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int test = 1000000;
	static const unsigned char yaml[] =
		"test: 1000000\n";
	struct target_struct {
		int test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(INT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = 0,
					.max = 9999999,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_uint_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned test = 90;
	static const unsigned char yaml[] =
		"test: 90\n";
	struct target_struct {
		unsigned test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(UINT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = 0,
					.max = UINT64_MAX,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_uint_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned test = 90;
	static const unsigned char yaml[] =
		"test: 90\n";
	struct target_struct {
		unsigned test;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD(UINT, "test", CYAML_FLAG_DEFAULT,
				struct target_struct, test, {
					.min = 50,
					.max = UINT64_MAX,
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_range_uint_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned test = 90;
	static const unsigned char yaml[] =
		"test: 90\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_validate_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int test = 90;
	static const unsigned char yaml[] =
		"test: 90\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_validate_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned test = 64;
	static const unsigned char yaml[] =
		"test: 64\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_validate_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const enum enum_test {
		ENUM_TEST_BAT,
		ENUM_TEST_DOG,
		ENUM_TEST_CAT,
	} test = ENUM_TEST_CAT;
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = ENUM_TEST_CAT },
		{ .str = "bat", .val = ENUM_TEST_BAT },
		{ .str = "dog", .val = ENUM_TEST_DOG },
	};
	static const unsigned char yaml[] =
		"test: cat\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_validate_flags(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const enum flags_test {
		FLAGS_TEST_BAT = 1 << 0,
		FLAGS_TEST_DOG = 1 << 1,
		FLAGS_TEST_CAT = 1 << 2,
	} test = FLAGS_TEST_CAT;
	static const cyaml_strval_t flags_test_schema[] = {
		{ .str = "cat", .val = FLAGS_TEST_CAT },
		{ .str = "bat", .val = FLAGS_TEST_BAT },
		{ .str = "dog", .val = FLAGS_TEST_DOG },
	};
	static const unsigned char yaml[] =
		"test: [ cat ]\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value");
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
static bool test_load_mapping_field_validate_bitfield(
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
		"test: { b: 1 }\n";
	struct target_struct {
		uint64_t test;
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
	if (err != CYAML_OK) {
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
static bool test_load_mapping_field_validate_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const float test = 3.14159f;
	static const unsigned char yaml[] =
		"test: 3.14159\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value "
				"(expected: %f, got: %f)",
				test, data_tgt->test);
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
static bool test_load_mapping_field_validate_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const double test = 3.14159;
	static const unsigned char yaml[] =
		"test: 3.14159\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test != test) {
		return ttest_fail(&tc, "Incorrect value "
				"(expected: %f, got: %f)",
				test, data_tgt->test);
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
static bool test_load_mapping_field_validate_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const char * const test = "Cat";
	static const unsigned char yaml[] =
		"test: Cat\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test == NULL) {
		return ttest_fail(&tc, "Failed to get allocation");
	}

	if (strcmp(data_tgt->test, test) != 0) {
		return ttest_fail(&tc, "Incorrect value "
				"(expected: %s, got: %s)",
				test, data_tgt->test);
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
static bool test_load_mapping_field_validate_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct test_animal_s test = {
		.animal = TEST_ANIMAL_CAT,
		.noise = (char *)"purr",
	};
	static const cyaml_strval_t enum_test_schema[] = {
		{ .str = "cat", .val = TEST_ANIMAL_CAT },
		{ .str = "bat", .val = TEST_ANIMAL_BAT },
		{ .str = "dog", .val = TEST_ANIMAL_DOG },
	};
	static const unsigned char yaml[] =
		"animal: cat\n"
		"noise: purr\n";
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->animal != test.animal) {
		return ttest_fail(&tc, "Incorrect value");
	}

	if (strcmp(data_tgt->noise, test.noise) != 0) {
		return ttest_fail(&tc, "Incorrect value "
				"(expected: %s, got: %s)",
				test.noise, data_tgt->noise);
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
static bool test_load_mapping_field_validate_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int8_t ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
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
static bool test_load_mapping_field_validate_sequence_fixed(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int8_t ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
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
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Run the YAML loading unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool load_tests(
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

	ttest_heading(rc, "Load single entry mapping tests: simple types");

	pass &= test_load_mapping_entry_enum(rc, &config);
	pass &= test_load_mapping_entry_uint(rc, &config);
	pass &= test_load_mapping_entry_float(rc, &config);
	pass &= test_load_mapping_entry_double(rc, &config);
	pass &= test_load_mapping_entry_string(rc, &config);
	pass &= test_load_mapping_entry_int_pos(rc, &config);
	pass &= test_load_mapping_entry_int_neg(rc, &config);
	pass &= test_load_mapping_entry_enum_ptr(rc, &config);
	pass &= test_load_mapping_entry_uint_ptr(rc, &config);
	pass &= test_load_mapping_entry_float_ptr(rc, &config);
	pass &= test_load_mapping_entry_bool_true(rc, &config);
	pass &= test_load_mapping_entry_bool_false(rc, &config);
	pass &= test_load_mapping_entry_double_ptr(rc, &config);
	pass &= test_load_mapping_entry_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_int_pos_ptr(rc, &config);
	pass &= test_load_mapping_entry_int_neg_ptr(rc, &config);
	pass &= test_load_mapping_entry_enum_sparse(rc, &config);
	pass &= test_load_mapping_entry_ignore_deep(rc, &config);
	pass &= test_load_mapping_entry_ignore_scalar(rc, &config);
	pass &= test_load_mapping_entry_enum_fallback(rc, &config);
	pass &= test_load_mapping_entry_bool_true_ptr(rc, &config);
	pass &= test_load_mapping_entry_bool_false_ptr(rc, &config);
	pass &= test_load_mapping_entry_float_underflow(rc, &config);
	pass &= test_load_mapping_entry_double_underflow(rc, &config);
	pass &= test_load_mapping_entry_string_ptr_empty(rc, &config);
	pass &= test_load_mapping_entry_string_ptr_null_str(rc, &config);
	pass &= test_load_mapping_entry_string_ptr_null_empty(rc, &config);
	pass &= test_load_mapping_entry_ignore_optional_scalar(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: complex types");

	pass &= test_load_mapping_entry_flags(rc, &config);
	pass &= test_load_mapping_entry_mapping(rc, &config);
	pass &= test_load_mapping_entry_bitfield(rc, &config);
	pass &= test_load_mapping_entry_flags_ptr(rc, &config);
	pass &= test_load_mapping_entry_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_flags_empty(rc, &config);
	pass &= test_load_mapping_entry_bitfield_ptr(rc, &config);
	pass &= test_load_mapping_entry_flags_sparse(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: sequences");

	pass &= test_load_mapping_entry_sequence_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_enum(rc, &config);
	pass &= test_load_mapping_entry_sequence_uint(rc, &config);
	pass &= test_load_mapping_entry_sequence_bool(rc, &config);
	pass &= test_load_mapping_entry_sequence_flags(rc, &config);
	pass &= test_load_mapping_entry_sequence_string(rc, &config);
	pass &= test_load_mapping_entry_sequence_mapping(rc, &config);
	pass &= test_load_mapping_entry_sequence_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: ptr sequences");

	pass &= test_load_mapping_entry_sequence_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_enum(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_uint(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_bool(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_flags(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_string(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_mapping(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Test integer limits");

	pass &= test_load_mapping_entry_int8_limit_neg(rc, &config);
	pass &= test_load_mapping_entry_int8_limit_pos(rc, &config);
	pass &= test_load_mapping_entry_int16_limit_neg(rc, &config);
	pass &= test_load_mapping_entry_int16_limit_pos(rc, &config);
	pass &= test_load_mapping_entry_int32_limit_neg(rc, &config);
	pass &= test_load_mapping_entry_int32_limit_pos(rc, &config);
	pass &= test_load_mapping_entry_int64_limit_neg(rc, &config);
	pass &= test_load_mapping_entry_int64_limit_pos(rc, &config);

	ttest_heading(rc, "Load tests: ptr sequence with null values");

	pass &= test_load_sequence_null_values_int(rc, &config);
	pass &= test_load_sequence_null_str_values_int(rc, &config);
	pass &= test_load_sequence_null_str_values_uint(rc, &config);
	pass &= test_load_sequence_null_str_values_mapping(rc, &config);

	ttest_heading(rc, "Load tests: sequence count sizes");

	pass &= test_load_mapping_entry_sequence_count_1(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_2(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_3(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_4(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_5(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_6(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_7(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_8(rc, &config);

	ttest_heading(rc, "Load tests: various");

	pass &= test_load_no_log(rc, &config);
	pass &= test_load_duplicate_ignored(rc, &config);
	pass &= test_load_schema_top_level_scalar(rc, &config);
	pass &= test_load_schema_top_level_string(rc, &config);
	pass &= test_load_schema_top_level_sequence(rc, &config);
	pass &= test_load_mapping_warn_ignored_keys(rc, &config);
	pass &= test_load_multiple_documents_ignored(rc, &config);
	pass &= test_load_mapping_without_any_fields(rc, &config);
	pass &= test_load_mapping_with_multiple_fields(rc, &config);
	pass &= test_load_mapping_with_optional_fields(rc, &config);
	pass &= test_load_mapping_only_optional_fields(rc, &config);
	pass &= test_load_mapping_ignored_unknown_keys(rc, &config);
	pass &= test_load_sequence_without_max_entries(rc, &config);
	pass &= test_load_schema_top_level_sequence_fixed(rc, &config);
	pass &= test_load_schema_sequence_entry_count_member(rc, &config);

	ttest_heading(rc, "Load tests: case sensitivity");

	pass &= test_load_enum_insensitive(rc, &config);
	pass &= test_load_flags_insensitive(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_1(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_2(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_3(rc, &config);
	pass &= test_load_mapping_fields_value_sensitive_1(rc, &config);
	pass &= test_load_mapping_fields_value_insensitive_1(rc, &config);

	ttest_heading(rc, "Load tests: anchors and aliases (scalars)");

	pass &= test_load_unused_anchor(rc, &config);
	pass &= test_load_anchor_scalar_int(rc, &config);
	pass &= test_load_anchor_scalar_string(rc, &config);
	pass &= test_load_anchor_multiple_scalars(rc, &config);

	ttest_heading(rc, "Load tests: anchors and aliases (non scalars)");

	pass &= test_load_anchor_mapping(rc, &config);
	pass &= test_load_anchor_sequence(rc, &config);
	pass &= test_load_anchor_deep_mapping_sequence(rc, &config);

	ttest_heading(rc, "Load tests: anchors and aliases (edge cases)");

	pass &= test_load_anchor_updated_anchor(rc, &config);

	ttest_heading(rc, "Load tests: default values");

	pass &= test_load_mapping_field_default_u8(rc, &config);
	pass &= test_load_mapping_field_default_int(rc, &config);
	pass &= test_load_mapping_field_default_bool(rc, &config);
	pass &= test_load_mapping_field_default_enum(rc, &config);
	pass &= test_load_mapping_field_default_flags(rc, &config);
	pass &= test_load_mapping_field_default_float(rc, &config);
	pass &= test_load_mapping_field_default_double(rc, &config);
	pass &= test_load_mapping_field_default_string(rc, &config);
	pass &= test_load_mapping_field_default_bitfield(rc, &config);
	pass &= test_load_mapping_field_default_mapping_large(rc, &config);
	pass &= test_load_mapping_field_default_mapping_small(rc, &config);
	pass &= test_load_mapping_field_default_sequence_large(rc, &config);
	pass &= test_load_mapping_field_default_sequence_small(rc, &config);

	ttest_heading(rc, "Load tests: default values (pointers)");

	pass &= test_load_mapping_field_default_u8_ptr(rc, &config);
	pass &= test_load_mapping_field_default_int_ptr(rc, &config);
	pass &= test_load_mapping_field_default_bool_ptr(rc, &config);
	pass &= test_load_mapping_field_default_enum_ptr(rc, &config);
	pass &= test_load_mapping_field_default_flags_ptr(rc, &config);
	pass &= test_load_mapping_field_default_float_ptr(rc, &config);
	pass &= test_load_mapping_field_default_double_ptr(rc, &config);
	pass &= test_load_mapping_field_default_string_ptr(rc, &config);
	pass &= test_load_mapping_field_default_bitfield_ptr(rc, &config);
	pass &= test_load_mapping_field_default_mapping_large_ptr(rc, &config);
	pass &= test_load_mapping_field_default_mapping_small_ptr(rc, &config);
	pass &= test_load_mapping_field_default_sequence_large_ptr(rc, &config);
	pass &= test_load_mapping_field_default_sequence_small_ptr(rc, &config);

	ttest_heading(rc, "Load tests: default values zero (pointers)");

	pass &= test_load_mapping_field_default_u8_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_int_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_bool_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_enum_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_flags_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_float_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_double_ptr_zero(rc, &config);
	pass &= test_load_mapping_field_default_bitfield_ptr_zero(rc, &config);

	ttest_heading(rc, "Load tests: Integer range constraints");

	pass &= test_load_mapping_field_range_int_1(rc, &config);
	pass &= test_load_mapping_field_range_int_2(rc, &config);
	pass &= test_load_mapping_field_range_int_3(rc, &config);
	pass &= test_load_mapping_field_range_int_4(rc, &config);
	pass &= test_load_mapping_field_range_uint_1(rc, &config);
	pass &= test_load_mapping_field_range_uint_2(rc, &config);
	pass &= test_load_mapping_field_range_uint_3(rc, &config);

	ttest_heading(rc, "Load tests: Validation callbacks");

	pass &= test_load_mapping_field_validate_int(rc, &config);
	pass &= test_load_mapping_field_validate_uint(rc, &config);
	pass &= test_load_mapping_field_validate_enum(rc, &config);
	pass &= test_load_mapping_field_validate_flags(rc, &config);
	pass &= test_load_mapping_field_validate_float(rc, &config);
	pass &= test_load_mapping_field_validate_double(rc, &config);
	pass &= test_load_mapping_field_validate_string(rc, &config);
	pass &= test_load_mapping_field_validate_mapping(rc, &config);
	pass &= test_load_mapping_field_validate_bitfield(rc, &config);
	pass &= test_load_mapping_field_validate_sequence(rc, &config);
	pass &= test_load_mapping_field_validate_sequence_fixed(rc, &config);

	return pass;
}
