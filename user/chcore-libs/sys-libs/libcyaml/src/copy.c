/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018-2020 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief Deep clone of client data-structure, using schema.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"
#include "data.h"
#include "util.h"

/**
 * A CYAML copy state machine stack entry.
 */
typedef struct cyaml_state {
	/** Current copy state machine state. */
	enum cyaml_state_e state;
	/** Schema for the expected value in this state. */
	const cyaml_schema_value_t *schema;
	/** Anonymous union for schema type specific state. */
	union {
		/** Additional state for \ref CYAML_STATE_IN_STREAM state. */
		struct {
			/** Number of documents read in stream. */
			uint32_t doc_count;
		} stream;
		/**
		 * Additional state for \ref CYAML_STATE_IN_MAP_KEY and
		 *  \ref CYAML_STATE_IN_MAP_VALUE states.
		 */
		struct {
			uint8_t *data;
			uint8_t *c;
			const cyaml_schema_field_t *field;
		} mapping;
		/** Additional state for \ref CYAML_STATE_IN_SEQUENCE state. */
		struct {
			uint8_t *data;
			uint8_t *count_data;
			uint64_t entry;
			uint64_t count;
			uint64_t count_size;
		} sequence;
	};
	const uint8_t *data; /**< Start of client value data for this state. */
	uint8_t *copy; /**< Where to write client value data for this state. */
} cyaml_state_t;

/**
 * Internal YAML copying context.
 */
typedef struct cyaml_ctx {
	const cyaml_config_t *config; /**< Settings provided by client. */
	cyaml_state_t *state;   /**< Current entry in state stack, or NULL. */
	cyaml_state_t *stack;   /**< State stack */
	uint32_t stack_idx;     /**< Next (empty) state stack slot */
	uint32_t stack_max;     /**< Current stack allocation limit. */
	unsigned seq_count;     /**< Top-level sequence count. */
} cyaml_ctx_t;

/**
 * Helper to check if schema is for a \ref CYAML_SEQUENCE type.
 *
 * \param[in]  schema  The schema entry for a type.
 * \return true iff schema is for a \ref CYAML_SEQUENCE type,
 *         false otherwise.
 */
static inline bool cyaml__is_sequence(const cyaml_schema_value_t *schema)
{
	return ((schema->type == CYAML_SEQUENCE) ||
	        (schema->type == CYAML_SEQUENCE_FIXED));
}

/**
 * Ensure that the CYAML copy context has space for a new stack entry.
 *
 * \param[in]  ctx     The CYAML copying context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stack_ensure(
		cyaml_ctx_t *ctx)
{
	cyaml_state_t *temp;
	uint32_t max = ctx->stack_max + 16;

	CYAML_UNUSED(ctx);

	if (ctx->stack_idx < ctx->stack_max) {
		return CYAML_OK;
	}

	temp = cyaml__realloc(ctx->config, ctx->stack, 0,
			sizeof(*ctx->stack) * max, false);
	if (temp == NULL) {
		return CYAML_ERR_OOM;
	}

	ctx->stack = temp;
	ctx->stack_max = max;
	ctx->state = ctx->stack + ctx->stack_idx - 1;

	return CYAML_OK;
}

/**
 * Push a new entry onto the CYAML copy context's stack.
 *
 * \param[in]  ctx     The CYAML copying context.
 * \param[in]  state   The CYAML copy state we're pushing a stack entry for.
 * \param[in]  schema  The CYAML schema for the value expected in state.
 * \param[in]  data    Pointer to where value's data should be read from.
 * \param[in]  copy    Pointer to where value's data should be written to.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stack_push(
		cyaml_ctx_t *ctx,
		enum cyaml_state_e state,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *data,
		cyaml_data_t *copy)
{
	cyaml_err_t err;
	cyaml_state_t s = {
		.copy = copy,
		.data = data,
		.state = state,
		.schema = schema,
	};

	err = cyaml__stack_ensure(ctx);
	if (err != CYAML_OK) {
		return err;
	}

	switch (state) {
	case CYAML_STATE_IN_MAP_KEY:
		assert(schema->type == CYAML_MAPPING);
		s.mapping.field = schema->mapping.fields;
		break;
	case CYAML_STATE_IN_SEQUENCE:
		assert(cyaml__is_sequence(schema));
		if (schema->type == CYAML_SEQUENCE_FIXED) {
			if (schema->sequence.min != schema->sequence.max) {
				return CYAML_ERR_SEQUENCE_FIXED_COUNT;
			}
		} else {
			if (ctx->state->state == CYAML_STATE_IN_SEQUENCE) {
				return CYAML_ERR_SEQUENCE_IN_SEQUENCE;

			} else if (ctx->state->state ==
					CYAML_STATE_IN_MAP_KEY) {
				const cyaml_schema_field_t *field =
						ctx->state->mapping.field - 1;
				s.sequence.count_data = ctx->state->copy +
						field->count_offset;
				s.sequence.count_size = field->count_size;
			} else {
				assert(ctx->state->state == CYAML_STATE_START);
				s.sequence.count_data = (void *)&ctx->seq_count;
				s.sequence.count_size = sizeof(ctx->seq_count);
			}
		}
		break;
	default:
		break;
	}

	cyaml__log(ctx->config, CYAML_LOG_DEBUG,
			"Copy: PUSH[%u]: %s\n", ctx->stack_idx,
			cyaml__state_to_str(state));

	ctx->stack[ctx->stack_idx] = s;
	ctx->state = ctx->stack + ctx->stack_idx;
	ctx->stack_idx++;

	return CYAML_OK;
}

/**
 * Pop the current entry on the CYAML copy context's stack.
 *
 * This frees any resources owned by the stack entry.
 *
 * \param[in]  ctx     The CYAML copying context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stack_pop(
		cyaml_ctx_t *ctx)
{
	uint32_t idx = ctx->stack_idx;

	assert(idx != 0);

	idx--;

	cyaml__log(ctx->config, CYAML_LOG_DEBUG, "Copy: POP[%u]: %s\n", idx,
			cyaml__state_to_str(ctx->state->state));

	ctx->state = (idx == 0) ? NULL : &ctx->stack[idx - 1];
	ctx->stack_idx = idx;

	return CYAML_OK;
}

/**
 * Helper to handle \ref CYAML_FLAG_POINTER.
 *
 * The current CYAML copying context's state is updated with new allocation
 * address, where necessary.
 *
 * \param[in]      ctx            The CYAML copying context.
 * \param[in]      schema         The schema for value to get data pointer for.
 * \param[in,out]  value_data_io  Current address of value's input data to be
 *                                copied.  Updated to new address if value is
 *                                allocation requiring an allocation.
 * \param[in,out]  value_copy_io  Current address of value's output data to be
 *                                written.  Updated to new address if value is
 *                                allocation requiring an allocation.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__data_handle_pointer(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const uint8_t **value_data_io,
		uint8_t **value_copy_io)
{
	cyaml_state_t *state = ctx->state;

	*value_data_io = cyaml_data_save_handle_pointer(ctx->config, schema,
			*value_data_io, "Copy");

	if (schema->flags & CYAML_FLAG_POINTER) {
		/* Need to create/extend an allocation. */
		size_t delta = schema->data_size;
		uint8_t *value_copy = NULL;

		if (*value_data_io == NULL) {
			return CYAML_ERR_BAD_PARAM_NULL_DATA;
		}

		switch (schema->type) {
		case CYAML_STRING:
			/* For a string the allocation size is the string
			 * size from the event, plus trailing NULL. */
			delta = strlen((const char *) *value_data_io) + 1;
			break;
		case CYAML_SEQUENCE:
			delta *= state->sequence.count;
			break;
		case CYAML_SEQUENCE_FIXED:
			delta *= schema->sequence.max;
			break;
		default:
			break;
		}

		cyaml__log(ctx->config, CYAML_LOG_DEBUG,
				"Copy: Allocating: (%zu bytes)\n", delta);

		value_copy = cyaml__alloc(ctx->config, delta, true);
		if (value_copy == NULL) {
			return CYAML_ERR_OOM;
		}

		cyaml__log(ctx->config, CYAML_LOG_DEBUG,
				"Copy: Writing allocation address %p into %p\n",
				value_copy, *value_copy_io);

		/* Write the allocation pointer into the data structure. */
		cyaml_data_write_pointer(value_copy, *value_copy_io);

		*value_copy_io = value_copy;
	}

	return CYAML_OK;
}

/**
 * Dump a backtrace to the log.
 *
 * \param[in]  ctx     The CYAML copying context.
 */
static void cyaml__backtrace(
		const cyaml_ctx_t *ctx)
{
	if (ctx->stack_idx > 1) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR, "Copy: Backtrace:\n");
	} else {
		return;
	}

	for (uint32_t idx = ctx->stack_idx - 1; idx != 0; idx--) {
		cyaml_state_t *state = ctx->stack + idx;
		switch (state->state) {
		case CYAML_STATE_IN_MAP_KEY: /* Fall through. */
		case CYAML_STATE_IN_MAP_VALUE:
			assert(state->mapping.field != NULL);
			cyaml__log(ctx->config, CYAML_LOG_ERROR,
					"  in mapping field: %s\n",
					state->mapping.field->key);
			break;
		case CYAML_STATE_IN_SEQUENCE:
			cyaml__log(ctx->config, CYAML_LOG_ERROR,
					"  in sequence entry: %"PRIu64"\n",
					state->sequence.count);
			break;
		default:
			break;
		}
	}
}

/**
 * Read a value of numerical type.
 *
 * \param[in]  ctx     The CYAML copying context.
 * \param[in]  schema  The schema for the value to be copied.
 * \param[in]  data    The place to read the value from in the client data.
 * \param[in]  copy    The place to write the value to in the client data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_number(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const uint8_t *data,
		uint8_t *copy)
{
	CYAML_UNUSED(ctx);

	memcpy(copy, data, schema->data_size);

	return CYAML_OK;
}

/**
 * Read a value of type \ref CYAML_STRING.
 *
 * \param[in]  ctx     The CYAML copying context.
 * \param[in]  schema  The schema for the value to be copied.
 * \param[in]  data    The place to read the value from in the client data.
 * \param[in]  copy    The place to write the value to in the client data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_string(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const uint8_t *data,
		uint8_t *copy)
{
	CYAML_UNUSED(ctx);
	CYAML_UNUSED(schema);

	memcpy(copy, data, strlen((const char *)data) + 1);

	return CYAML_OK;
}

/**
 * Write a sequence entry count to the client data structure.
 *
 * \param[in]  ctx        The CYAML copying context.
 * \param[in]  schema     The schema for the value to be copied.
 * \param[in]  seq_count  The sequence entry count to write.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__write_sequence_count(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		uint64_t seq_count)
{
	cyaml_err_t err;

	if (schema->type != CYAML_SEQUENCE) {
		return CYAML_OK;
	}

	err = cyaml_data_write(seq_count,
			ctx->state->sequence.count_size,
			ctx->state->sequence.count_data);

	/* Can't go wrong, because the count_size was changed when reading. */
	assert(err == CYAML_OK);

	return err;
}

/**
 * Handle a YAML event corresponding to a YAML data value.
 *
 * \param[in]  ctx        The CYAML copying context.
 * \param[in]  schema     CYAML schema for the expected value.
 * \param[in]  data       The place to read the value from in the client data.
 * \param[in]  seq_count  Entry count for sequence values.  Unused for
 *                        non-sequence values.
 * \param[in]  copy       Pointer to where value's data should be written.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_value(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const uint8_t *data,
		uint64_t seq_count,
		uint8_t *copy)
{
	cyaml_err_t err;

	cyaml__log(ctx->config, CYAML_LOG_DEBUG,
			"Copy: Cloning value of type '%s'%s\n",
			cyaml__type_to_str(schema->type),
			schema->flags & CYAML_FLAG_POINTER ? " (pointer)" : "");

//	if (schema->type == CYAML_MAPPING &&
//	    schema->flags & CYAML_FLAG_POINTER) {
//		const uint8_t *mapping_data = data;
//		mapping_data = cyaml_data_save_handle_pointer(ctx->config,
//				schema, mapping_data, "Copy");
//		if (mapping_data == NULL) {
//			return CYAML_ERR_BAD_PARAM_NULL_DATA;
//		}
//	}

	if (!cyaml__is_sequence(schema)) {
		/* Since sequences extend their allocation for each entry,
		 * they're handled in the sequence-specific code.
		 */
		err = cyaml__data_handle_pointer(ctx, schema, &data, &copy);
		if (err != CYAML_OK) {
			return err;
		}
	}

	switch (schema->type) {
	case CYAML_INT:     /* Fall through. */
	case CYAML_UINT:    /* Fall through. */
	case CYAML_BOOL:    /* Fall through. */
	case CYAML_ENUM:    /* Fall through. */
	case CYAML_FLOAT:   /* Fall through. */
	case CYAML_FLAGS:   /* Fall through. */
	case CYAML_BITFIELD:
		err = cyaml__clone_number(ctx, schema, data, copy);
		break;
	case CYAML_STRING:
		err = cyaml__clone_string(ctx, schema, data, copy);
		break;
	case CYAML_MAPPING:
		err = cyaml__stack_push(ctx, CYAML_STATE_IN_MAP_KEY,
				schema, data, copy);
		break;
	case CYAML_SEQUENCE_FIXED: /* Fall through. */
	case CYAML_SEQUENCE:
		err = cyaml__stack_push(ctx, CYAML_STATE_IN_SEQUENCE,
				schema, data, copy);
		if (err != CYAML_OK) {
			return err;
		}
		ctx->state->sequence.count = seq_count;
		err = cyaml__data_handle_pointer(ctx, schema, &data, &copy);
		if (err != CYAML_OK) {
			return err;
		}
		err = cyaml__write_sequence_count(ctx, schema, seq_count);
		ctx->state->data = data;
		ctx->state->copy = copy;
		break;
	default:
		err = CYAML_ERR_BAD_TYPE_IN_SCHEMA;
		break;
	}

	return err;
}

/**
 * YAML saving handler for the \ref CYAML_STATE_START state.
 *
 * \param[in]  ctx  The CYAML copying context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_start(
		cyaml_ctx_t *ctx)
{
	cyaml_state_t *state = ctx->state;
	return cyaml__clone_value(ctx, state->schema,
			state->data, ctx->seq_count, state->copy);
}

/**
 * YAML copying handler for the \ref CYAML_STATE_IN_MAP_KEY and \ref
 * CYAML_STATE_IN_MAP_VALUE states.
 *
 * \param[in]  ctx  The CYAML copying context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_mapping(
		cyaml_ctx_t *ctx)
{
	cyaml_err_t err = CYAML_OK;
	cyaml_state_t *state = ctx->state;
	const cyaml_schema_field_t *field = state->mapping.field;

	if (field != NULL && field->key != NULL) {
		uint64_t seq_count = 0;

		if (field->value.type == CYAML_IGNORE) {
			ctx->state->mapping.field++;
			return CYAML_OK;
		}
		cyaml__log(ctx->config, CYAML_LOG_INFO,
				"Copy: [%s]\n", field->key);

		/* Advance the field before writing value, since writing the
		 * value can put a new state entry on the stack. */
		ctx->state->mapping.field++;

		if (field->value.type == CYAML_SEQUENCE) {
			seq_count = cyaml_data_read(field->count_size,
					ctx->state->data + field->count_offset,
					&err);
			if (err != CYAML_OK) {
				return err;
			}
			cyaml__log(ctx->config, CYAML_LOG_INFO,
					"Copy: Sequence entry count: %"PRIu64"\n",
					seq_count);

		} else if (field->value.type == CYAML_SEQUENCE_FIXED) {
			seq_count = field->value.sequence.min;
		}

		err = cyaml__clone_value(ctx,
				&field->value,
				state->data + field->data_offset,
				seq_count,
				state->copy + field->data_offset);
		if (err == CYAML_ERR_BAD_PARAM_NULL_DATA) {
			if (!(field->value.flags & CYAML_FLAG_OPTIONAL)) {
				return CYAML_ERR_MAPPING_FIELD_MISSING;
			}
			err = CYAML_OK;
		}
	} else {
		err = cyaml__stack_pop(ctx);
	}

	return err;
}

/**
 * YAML copying handler for the \ref CYAML_STATE_IN_SEQUENCE state.
 *
 * \param[in]  ctx  The CYAML copying context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__clone_sequence(
		cyaml_ctx_t *ctx)
{
	cyaml_err_t err = CYAML_OK;

	if (ctx->state->sequence.entry < ctx->state->sequence.count) {
		const cyaml_schema_value_t *schema = ctx->state->schema;
		const cyaml_schema_value_t *value = schema->sequence.entry;
		unsigned seq_count = 0;
		size_t data_size;
		size_t offset;

		if (value->type == CYAML_SEQUENCE_FIXED) {
			seq_count = value->sequence.max;
		}

		if (value->flags & CYAML_FLAG_POINTER) {
			data_size = sizeof(NULL);
		} else {
			data_size = value->data_size;
			if (value->type == CYAML_SEQUENCE_FIXED) {
				data_size *= seq_count;
			}
		}
		offset = data_size * ctx->state->sequence.entry;

		cyaml__log(ctx->config, CYAML_LOG_INFO,
				"Copy: Sequence entry %"PRIu64" of %"PRIu64"\n",
				ctx->state->sequence.entry + 1,
				ctx->state->sequence.count);

		/* Advance the entry before writing value, since writing the
		 * value can put a new state entry on the stack. */
		ctx->state->sequence.entry++;

		err = cyaml__clone_value(ctx, value,
				ctx->state->data + offset,
				seq_count,
				ctx->state->copy + offset);
	} else {
		err = cyaml__stack_pop(ctx);
	}

	return err;
}

/**
 * Check that common copy params from client are valid.
 *
 * \param[in] config     The client's CYAML library config.
 * \param[in] schema     The schema describing the content of data.
 * \param[in] seq_count  Top level sequence count.
 * \param[in] data_out   Points to client's address to write data to.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static inline cyaml_err_t cyaml__validate_copy_params(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		unsigned seq_count,
		cyaml_data_t **data_out)
{
	if (config == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_CONFIG;
	}
	if (config->mem_fn == NULL) {
		return CYAML_ERR_BAD_CONFIG_NULL_MEMFN;
	}
	if (schema == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_SCHEMA;
	}
	if (cyaml__is_sequence(schema) != (seq_count != 0)) {
		return CYAML_ERR_BAD_PARAM_SEQ_COUNT;
	}
	if (data_out == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_DATA;
	}
	if ((schema->flags & CYAML_FLAG_POINTER) && (*data_out != NULL)) {
		return CYAML_ERR_DATA_TARGET_NON_NULL;
	}
	return CYAML_OK;
}

/**
 * Get the total length of an allocation.
 *
 * \param[in]  schema     Schema for the data value to get the size of.
 * \param[in]  seq_count  If top level type is sequence, this should be the
 *                        entry count, otherwise it is ignored.
 * \return the byte length of the data.
 */
static size_t cyaml__get_data_len(
		const cyaml_schema_value_t *schema,
		unsigned seq_count)
{
	size_t len = cyaml__is_sequence(schema) ?
			schema->sequence.entry->data_size :
			schema->data_size;

	if (schema->type == CYAML_SEQUENCE) {
		len *= seq_count;
	} else if (schema->type == CYAML_SEQUENCE_FIXED) {
		len *= schema->sequence.max;
	}

	return len;
}

/* Exported function, documented in include/cyaml/cyaml.h */
cyaml_err_t cyaml_copy(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *data,
		unsigned seq_count,
		cyaml_data_t **data_out)
{
	cyaml_data_t *copy = NULL;
	cyaml_ctx_t ctx = {
		.config = config,
		.seq_count = seq_count,
	};
	typedef cyaml_err_t (* const cyaml_clone_fn)(
			cyaml_ctx_t *ctx);
	static const cyaml_clone_fn fn[CYAML_STATE__COUNT] = {
		[CYAML_STATE_START]        = cyaml__clone_start,
		[CYAML_STATE_IN_MAP_KEY]   = cyaml__clone_mapping,
		[CYAML_STATE_IN_MAP_VALUE] = cyaml__clone_mapping,
		[CYAML_STATE_IN_SEQUENCE]  = cyaml__clone_sequence,
	};
	cyaml_err_t err = CYAML_OK;

	err = cyaml__validate_copy_params(config, schema, seq_count, data_out);
	if (err != CYAML_OK) {
		if (ctx.config != NULL) {
			cyaml__log(ctx.config, CYAML_LOG_ERROR,
					"Copy: Bad call parameters\n");
		}
		return err;
	}

	if (!(schema->flags & CYAML_FLAG_POINTER)) {
		size_t len = cyaml__get_data_len(schema, seq_count);
		copy = cyaml__alloc(config, len, true);
		if (copy == NULL) {
			return CYAML_ERR_OOM;
		}
	}

	err = cyaml__stack_push(&ctx, CYAML_STATE_START, schema,
			(schema->flags & CYAML_FLAG_POINTER) ? &data : data,
			(schema->flags & CYAML_FLAG_POINTER) ? &copy : copy);
	if (err != CYAML_OK) {
		goto out;
	}

	do {
		cyaml__log(ctx.config, CYAML_LOG_DEBUG,
				"Copy: Handle state %s\n",
				cyaml__state_to_str(ctx.state->state));
		err = fn[ctx.state->state](&ctx);
		if (err != CYAML_OK) {
			goto out;
		}
	} while (ctx.stack_idx > 1);

	cyaml__stack_pop(&ctx);

	assert(ctx.stack_idx == 0);

	if (!(schema->flags & CYAML_FLAG_POINTER)) {
		size_t len = cyaml__get_data_len(schema, seq_count);
		if (*data_out == NULL) {
			err = CYAML_ERR_BAD_PARAM_NULL_DATA;
			goto out;
		}
		memcpy(*data_out, copy, len);
	} else {
		*data_out = copy;
	}
out:
	if (err != CYAML_OK) {
		cyaml_free(config, schema, copy, ctx.seq_count);
		cyaml__backtrace(&ctx);
	}
	if (!(schema->flags & CYAML_FLAG_POINTER)) {
		cyaml__free(config, copy);
	}
	while (ctx.stack_idx > 0) {
		cyaml__stack_pop(&ctx);
	}
	cyaml__free(config, ctx.stack);
	return err;
}
