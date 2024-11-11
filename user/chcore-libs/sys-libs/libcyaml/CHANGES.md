LibCYAML: Change Log
====================

## LibCYAML v1.4.1

* **Loading**:
  * Log valid enum values on error parsing enums.
  * Split out storage of scaler values to client data structure.
* **General**:
  * Updated code coverage CI to latest action versions.

No changes are required for client applications to upgrade.


## LibCYAML v1.4.0

* **Loading**:
  * Reject numerical values with trailing junk.
  * New `CYAML_CFG_IGNORED_KEY_WARNING` to generate warnings when mapping
    keys are ignored either explicitly by the `CYAML_IGNORE` type or implicitly
    by the `CYAML_CFG_IGNORE_UNKNOWN_KEYS` config setting.
* **Buildsystem**:
  * Avoid using GNU `-D` install flag.
* **General**:
  * Improved API documentation for `CYAML_SEQUENCE_FIXED`.
  * Minor optimisation for host endian detection for older GCCs.
  * Minor CI updates.

No changes are required for client applications to upgrade.


## LibCYAML v1.3.1

* **Loading**:
  * Fixed value out-of-range detection limits for signed integers.

No changes are required for client applications to upgrade.


## LibCYAML v1.3.0

* **Saving**:
  * New flags allow control over scalar output style.
    - For example to force single or double quote style.
* **General**:
  * Buildsystem changes to allow use of CPPFLAGS from the environment.

No changes are required for client applications to upgrade.


## LibCYAML v1.2.1

* **General**:
  * Support for dynamic library build on Mac OS X.
  * Ordered designated initialisers in public header for C++ compatibility.

No changes are required for client applications to upgrade.


## LibCYAML v1.2.0

* **Loading**:
  * Allow mappings with zero fields in the schema.
  * Improved logging of errors.
  * `CYAML_BOOL` type now treats "Off" as false.
  * Allow loading of float values that overflow or underflow unless
    `CYAML_FLAG_STRICT` set.
  * Added line and column numbers to backtraces.
* **General**:
  * Update tests to handle libyaml 0.2.5 output format change.
  * Buildsystem improvements.
  * Made public header C++ compatible.
  * Test runner supports running individual tests.

No changes are required for client applications to upgrade.


## LibCYAML v1.1.0

* **Loading**:
  * Significantly optimised handling of aliases and anchors.
  * Fixed handling of duplicate mapping keys.
* **Saving**:
  * Increased precision for double precision floating point values.
* **General**:
  * Fixed data handling on big endian systems.

No changes are required for client applications to upgrade.


## LibCYAML v1.0.2

* **Loading**:
  * Fixed invalid read on error path for bitfield handling.
* **Buildsystem**:
  * Fixed to link against libraries after listing objects.
  * Added `check` target as alias for `test`.

No changes are required for client applications to upgrade.


## LibCYAML v1.0.1

* **Loading**:
  * Fixed mapping and sequence values with `CYAML_FLAG_POINTER_NULL`.
* **Buildsystem**:
  * Installation: Explicitly create leading directories.

No changes are required for client applications to upgrade.


## LibCYAML v1.0.0

* Initial release.
