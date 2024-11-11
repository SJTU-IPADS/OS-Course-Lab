LibCYAML: Schema-based YAML parsing and serialisation
=====================================================

[![Build Status](https://github.com/tlsa/libcyaml/workflows/CI/badge.svg)](https://github.com/tlsa/libcyaml/actions) [![Static Analysis](https://github.com/tlsa/libcyaml/actions/workflows/static-analysis.yaml/badge.svg)](https://github.com/tlsa/libcyaml/actions/workflows/static-analysis.yaml) [![Code Coverage](https://codecov.io/gh/tlsa/libcyaml/branch/main/graph/badge.svg)](https://codecov.io/gh/tlsa/libcyaml)

LibCYAML is a C library for reading and writing structured YAML documents.
It is written in ISO C11 and licensed under the ISC licence.

> **Warning**
>
> The `main` branch is currently undergoing changes for version 2.
> You probably want to be using a
> [tagged release](https://github.com/tlsa/libcyaml/tags) instead.

Overview
--------

The fundamental idea behind CYAML is to allow applications to construct
schemas which describe both the permissible structure of the YAML documents
to read/write, and the C data structure(s) in which the loaded data is
arranged in memory.

### Goals

* Make it easy to load YAML into client's custom C data structures.
* Good compromise between flexibility and simplicity.

### Features

* Load, Save and Free functions.
* Reads and writes arbitrary client data structures.
* Schema-driven, allowing control over permitted YAML, for example:
    - Required / optional mapping fields.
    - Allowed / disallowed values.
    - Minimum / maximum sequence entry count.
    - etc...
* Enumerations and flag words.
* YAML backtraces make it simple for users to fix their YAML to
  conform to your schema.
* Uses standard [`libyaml`](https://github.com/yaml/libyaml) library for
  low-level YAML read / write.
* Support for YAML aliases and anchors.

Building
--------

To build the library, run:

    make

You can control the optimisation and building of asserts by setting
the build variant:

    make VARIANT=debug
    make VARIANT=release

Another debug build variant which is built with address sanitiser (incompatible
with valgrind) can be built with:

    make VARIANT=san

Installation
------------

To install a release version of the library, run:

    make install VARIANT=release

It will install to the PREFIX `/usr/local` by default, and it will use
DESTDIR and PREFIX from the environment if set.

Testing
-------

To run the tests, run any of the following, which generate various
levels of output verbosity (optionally setting `VARIANT=release`, or
`VARIANT=san`):

    make test
    make test-quiet
    make test-verbose
    make test-debug

To run the tests under `valgrind`, a similar set of targets is available:

    make valgrind
    make valgrind-quiet
    make valgrind-verbose
    make valgrind-debug

To run a single test or a subset of tests, use the `TESTLIST` variable, which
expects a space and/or comma separated list of test names:

    make test-debug TESTLIST=test_load_mapping_without_any_fields
    make valgrind-debug TESTLIST="test_load_no_log test_util_state_invalid"

To generate a test coverage report, `gcovr` is required:

    make coverage

Documentation
-------------

To generate both public API documentation, and documentation of CYAML's
internals, `doxygen` is required:

    make docs

Alternatively, the read the API documentation directly from the
[cyaml.h](https://github.com/tlsa/libcyaml/blob/main/include/cyaml/cyaml.h)
header file.

There is also a [tutorial](docs/guide.md).

Examples
--------

In addition to the documentation, you can study the [examples](examples/).
