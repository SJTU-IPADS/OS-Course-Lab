/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2023 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML library private internal implementation details.
 *
 * These macros provide functionality to enable the client facing header
 * to work. They are not intended to be used directly by client code.
 */

#ifndef CYAML_PRIVATE_H
#define CYAML_PRIVATE_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Internal macro to add \ref CYAML_FLAG_POINTER to some flags.
 *
 * \param[in]  _flags  The flags to modify.
 * \return The modified flags.
 */
#define CYAML__POINTER_ADD(_flags) \
	((_flags) | CYAML_FLAG_POINTER)

/**
 * Internal macro to remove \ref CYAML_FLAG_POINTER from some flags.
 *
 * \param[in]  _flags  The flags to modify.
 * \return The modified flags.
 */
#define CYAML__POINTER_REMOVE(_flags) \
	((_flags) & (~CYAML_FLAG_POINTER))

/** Type to \ref cyaml_schema_value union member for INT. */
#define CYAML__UNION_MEMBER_INT            .integer
/** Type to \ref cyaml_schema_value union member for UINT. */
#define CYAML__UNION_MEMBER_UINT           .unsigned_integer
/** Type to \ref cyaml_schema_value union member for BOOL. */
#define CYAML__UNION_MEMBER_BOOL           .boolean
/** Type to \ref cyaml_schema_value union member for ENUM. */
#define CYAML__UNION_MEMBER_ENUM           .enumeration
/** Type to \ref cyaml_schema_value union member for FLAGS. */
#define CYAML__UNION_MEMBER_FLAGS          .enumeration
/** Type to \ref cyaml_schema_value union member for FLOAT. */
#define CYAML__UNION_MEMBER_FLOAT          .floating_point
/** Type to \ref cyaml_schema_value union member for STRING. */
#define CYAML__UNION_MEMBER_STRING         .string
/** Type to \ref cyaml_schema_value union member for MAPPING. */
#define CYAML__UNION_MEMBER_MAPPING        .mapping
/** Type to \ref cyaml_schema_value union member for BITFIELD. */
#define CYAML__UNION_MEMBER_BITFIELD       .bitfield
/** Type to \ref cyaml_schema_value union member for SEQUENCE. */
#define CYAML__UNION_MEMBER_SEQUENCE       .sequence
/** Type to \ref cyaml_schema_value union member for SEQUENCE_FIXED. */
#define CYAML__UNION_MEMBER_SEQUENCE_FIXED .sequence

/** Fake a valid sequence count member for INT. */
#define CYAML__SEQUENCE_COUNT_INT(_member, _count)            _member
/** Fake a valid sequence count member for UINT. */
#define CYAML__SEQUENCE_COUNT_UINT(_member, _count)           _member
/** Fake a valid sequence count member for BOOL. */
#define CYAML__SEQUENCE_COUNT_BOOL(_member, _count)           _member
/** Fake a valid sequence count member for ENUM. */
#define CYAML__SEQUENCE_COUNT_ENUM(_member, _count)           _member
/** Fake a valid sequence count member for FLAGS. */
#define CYAML__SEQUENCE_COUNT_FLAGS(_member, _count)          _member
/** Fake a valid sequence count member for FLOAT. */
#define CYAML__SEQUENCE_COUNT_FLOAT(_member, _count)          _member
/** Fake a valid sequence count member for STRING. */
#define CYAML__SEQUENCE_COUNT_STRING(_member, _count)         _member
/** Fake a valid sequence count member for MAPPING. */
#define CYAML__SEQUENCE_COUNT_MAPPING(_member, _count)        _member
/** Fake a valid sequence count member for BITFIELD. */
#define CYAML__SEQUENCE_COUNT_BITFIELD(_member, _count)       _member
/** Create the sequence count member for SEQUENCE. */
#define CYAML__SEQUENCE_COUNT_SEQUENCE(_member, _count)       _member ## _count
/** Fake a valid sequence count member for SEQUENCE_FIXED. */
#define CYAML__SEQUENCE_COUNT_SEQUENCE_FIXED(_member, _count) _member

/** Adapt flags for mapping schema building macro for INT. */
#define CYAML__HANDLE_PTR_INT(_flags)            CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for UINT. */
#define CYAML__HANDLE_PTR_UINT(_flags)           CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for BOOL. */
#define CYAML__HANDLE_PTR_BOOL(_flags)           CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for ENUM. */
#define CYAML__HANDLE_PTR_ENUM(_flags)           CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for FLAGS. */
#define CYAML__HANDLE_PTR_FLAGS(_flags)          CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for FLOAT. */
#define CYAML__HANDLE_PTR_FLOAT(_flags)          CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for STRING. */
#define CYAML__HANDLE_PTR_STRING(_flags)         CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for MAPPING. */
#define CYAML__HANDLE_PTR_MAPPING(_flags)        CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for BITFIELD. */
#define CYAML__HANDLE_PTR_BITFIELD(_flags)       CYAML__POINTER_ADD(_flags)
/** Adapt flags for mapping schema building macro for SEQUENCE. */
#define CYAML__HANDLE_PTR_SEQUENCE(_flags)       _flags
/** Adapt flags for mapping schema building macro for SEQUENCE_FIXED. */
#define CYAML__HANDLE_PTR_SEQUENCE_FIXED(_flags) _flags

/**
 * Create an expression for a variable of the type of a structure member.
 *
 * \param[in]  _structure  The structure containing _member.
 * \param[in]  _member     A member of _structure.
 * \return variable of type `_structure._member`.
 */
#define CYAML__MEMBER(_structure, _member) \
	(((_structure *)NULL)->_member)

#ifdef __cplusplus
}
#endif

#endif
