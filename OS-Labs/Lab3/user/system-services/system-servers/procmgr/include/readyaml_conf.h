#include <cyaml/cyaml.h>
typedef struct {
    char *filename;
    char **services;
    unsigned int services_count;
    char *boot_time;
    char *registration_method;
    char *type;
} yaml_server_t;

static const cyaml_schema_value_t string_ptr_schema = {
	CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
};

static const cyaml_schema_field_t yaml_server_fields_schema[] = {
    CYAML_FIELD_STRING_PTR("filename", CYAML_FLAG_POINTER,
        yaml_server_t, filename, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE_COUNT("services", CYAML_FLAG_POINTER,
        yaml_server_t, services, services_count, &string_ptr_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("boot_time", CYAML_FLAG_POINTER,
        yaml_server_t, boot_time, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("registration_method", CYAML_FLAG_POINTER,
        yaml_server_t, registration_method, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER,
        yaml_server_t, type, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t yaml_server_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, yaml_server_t, yaml_server_fields_schema),
};


static const cyaml_schema_value_t yaml_servers_schema = {
    CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, yaml_server_t, &yaml_server_schema, 0, CYAML_UNLIMITED),
};

static cyaml_config_t cyaml_config = {
        .log_level = CYAML_LOG_WARNING,     /* Logging errors and warnings only. */
        .log_fn = cyaml_log,                /* Use the default logging function. */
        .mem_fn = cyaml_mem,                /* Use the default memory allocator. */
    };