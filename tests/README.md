#Tests

Unit tests

## Dependencies

* lcov

## Unit test

use minunit

Refer to example

[minunit README](https://github.com/siu/minunit)

## Coverage

Refer to example

Use your own \$PROJECT and \$SOURCES

```
mkdir build
cd build
cmake .. -G Ninja
ninja
ninja lcov
```

if all tests passes, coverage report will be in the `report` directory
