#!/usr/bin/env python3
"""Verify matrix-load miss counts in a cold, set-associative LRU cache.

Only W reads enter the cache. There is no prefetching or other traffic.
Run with --out rowcol_cache_model.json to save results, or use stdout JSON.
Importing this module does not run the simulation.
"""

import argparse
from collections import Counter
from itertools import islice
import json
from pathlib import Path
import sys

ADDRESS_BITS = 64
MATRIX_SIZE = 512
ELEMENT_BYTES = 4
BASE_ADDRESS = 0x0000000100000000
BASE_ALIGNMENT = 4096
SET_COUNT = 64
WAY_COUNT = 12
BLOCK_BYTES = 64
BLOCK_BITS = BLOCK_BYTES.bit_length() - 1
SET_BITS = SET_COUNT.bit_length() - 1
TAG_BITS = ADDRESS_BITS - SET_BITS - BLOCK_BITS


def row_loads(size, coalesce=False):
    """Yield (block number, consecutive load count) for sum_rows."""
    total_bytes = size * size * ELEMENT_BYTES
    if coalesce:
        # Repeated reads of the current MRU block leave the LRU order unchanged.
        assert BASE_ADDRESS % BLOCK_BYTES == 0
        assert total_bytes % BLOCK_BYTES == 0
        first_block = BASE_ADDRESS >> BLOCK_BITS
        block_count = total_bytes // BLOCK_BYTES
        for block_number in range(first_block, first_block + block_count):
            yield block_number, BLOCK_BYTES // ELEMENT_BYTES
    else:
        for address in range(BASE_ADDRESS, BASE_ADDRESS + total_bytes, ELEMENT_BYTES):
            yield address >> BLOCK_BITS, 1


def column_loads(size, row_limit):
    """Keep the physical row stride independent of the number of rows read."""
    row_stride = size * ELEMENT_BYTES
    for column in range(size):
        address = BASE_ADDRESS + column * ELEMENT_BYTES
        for row in range(row_limit):
            yield address >> BLOCK_BITS, 1
            address += row_stride


def simulate(loads, set_count=SET_COUNT, way_count=WAY_COUNT):
    """Use insertion-ordered dictionaries, oldest block first, for true LRU."""
    assert set_count > 0 and set_count & (set_count - 1) == 0
    assert way_count > 0
    cache = [{} for index in range(set_count)]
    set_mask = set_count - 1
    accesses = misses = 0
    for block_number, repetitions in loads:
        assert repetitions > 0
        accesses += repetitions
        lines = cache[block_number & set_mask]
        # Within one set, equal block numbers are equivalent to equal tags.
        if block_number in lines:
            del lines[block_number]
        else:
            misses += 1
            if len(lines) == way_count:
                del lines[next(iter(lines))]
        lines[block_number] = None
    return {
        "accesses": accesses,
        "misses": misses,
        "hits": accesses - misses,
        "miss_rate": misses / accesses,
        "miss_percent": 100 * misses / accesses,
    }


def reference(loads, set_count, way_count):
    """Independent, small-scale list LRU, newest block first."""
    cache = [[] for index in range(set_count)]
    accesses = misses = 0
    for block_number, repetitions in loads:
        for repeat in range(repetitions):
            accesses += 1
            lines = cache[block_number % set_count]
            if block_number in lines:
                lines.remove(block_number)
            else:
                misses += 1
                if len(lines) == way_count:
                    lines.pop()
            lines.insert(0, block_number)
    return accesses, misses


def crosscheck():
    """Check LRU hits, set conflicts, and coalescing against individual loads."""
    trace = [(block, 1) for block in (0, 1, 0, 2, 1)]
    assert simulate(trace, 1, 2)["misses"] == 4
    conflict = [(block, 1) for block in (0, 4, 0, 4)]
    assert simulate(conflict, 4, 1)["misses"] == 4
    assert simulate(conflict, 1, 4)["misses"] == 2
    checked = 0
    for size in (8, 16, 32, 64):
        for sets, ways in ((4, 1), (4, 3), (1, 8)):
            rows = simulate(row_loads(size), sets, ways)
            assert rows == simulate(row_loads(size, coalesce=True), sets, ways)
            assert (rows["accesses"], rows["misses"]) == reference(
                row_loads(size), sets, ways)
            columns = simulate(column_loads(size, size), sets, ways)
            assert (columns["accesses"], columns["misses"]) == reference(
                column_loads(size, size), sets, ways)
            checked += 1
    for row_limit in (12, 13):
        columns = simulate(column_loads(64, row_limit), 4, 12)
        assert (columns["accesses"], columns["misses"]) == reference(
            column_loads(64, row_limit), 4, 12)
        checked += 1
    return checked


def verify_column_reuse():
    """Compare consecutive columns and a same-capacity fully associative cache."""
    trace = list(islice(column_loads(MATRIX_SIZE, MATRIX_SIZE), 2 * MATRIX_SIZE))
    first, second = trace[:MATRIX_SIZE], trace[MATRIX_SIZE:]
    assert first == second
    groups = Counter(block % SET_COUNT for block, repeat in first)
    assert groups == {0: MATRIX_SIZE // 2, SET_COUNT // 2: MATRIX_SIZE // 2}
    distances = []
    for row, (target, repeat) in enumerate(first):
        group = target % SET_COUNT
        intervening = trace[row + 1:MATRIX_SIZE + row]
        other_blocks = {block for block, repeat in intervening
                        if block % SET_COUNT == group}
        distances.append(len(other_blocks))
        assert len(other_blocks) == groups[group] - 1
    first_misses = simulate(first)["misses"]
    second_misses = simulate(trace)["misses"] - first_misses
    assert first_misses == second_misses == MATRIX_SIZE
    cache_lines = SET_COUNT * WAY_COUNT
    assert MATRIX_SIZE <= cache_lines
    fully_associative = simulate(column_loads(MATRIX_SIZE, MATRIX_SIZE), 1, cache_lines)
    assert fully_associative["misses"] == MATRIX_SIZE ** 2 * ELEMENT_BYTES // BLOCK_BYTES
    exact = reference(column_loads(MATRIX_SIZE, MATRIX_SIZE), SET_COUNT, WAY_COUNT)
    assert exact == (MATRIX_SIZE ** 2, MATRIX_SIZE ** 2)
    return {
        "blocks_per_matrix_row": MATRIX_SIZE * ELEMENT_BYTES // BLOCK_BYTES,
        "groups_per_column": len(groups),
        "column_zero_sets": dict(sorted(groups.items())),
        "blocks_per_used_group": min(groups.values()),
        "same_set_reuse_distance": min(distances),
        "column_zero_misses": first_misses,
        "column_one_misses": second_misses,
        "column_working_set_bytes": MATRIX_SIZE * BLOCK_BYTES,
        "cache_lines": cache_lines,
        "same_capacity_fully_associative_misses": fully_associative["misses"],
    }


def verify():
    assert BLOCK_BYTES & (BLOCK_BYTES - 1) == 0
    assert BASE_ADDRESS % BASE_ALIGNMENT == 0
    assert BASE_ADDRESS + MATRIX_SIZE ** 2 * ELEMENT_BYTES <= 1 << ADDRESS_BITS
    assert (BLOCK_BITS, SET_BITS, TAG_BITS) == (6, 6, 52)
    checked = crosscheck()
    column_analysis = verify_column_reuse()
    results = {
        "sum_rows": simulate(row_loads(MATRIX_SIZE, coalesce=True)),
        "sum_cols": simulate(column_loads(MATRIX_SIZE, MATRIX_SIZE)),
    }
    expected = {
        "sum_rows": (262144, 16384),
        "sum_cols": (262144, 262144),
    }
    for name, (accesses, misses) in expected.items():
        result = results[name]
        assert (result["accesses"], result["misses"]) == (accesses, misses)
        result["row_limit"] = MATRIX_SIZE
        result["column_limit"] = MATRIX_SIZE
    return {
        "schema_version": 1,
        "matrix": {
            "n": MATRIX_SIZE,
            "element_bytes": ELEMENT_BYTES,
            "row_stride_bytes": MATRIX_SIZE * ELEMENT_BYTES,
            "size_bytes": MATRIX_SIZE ** 2 * ELEMENT_BYTES,
            "base_address": BASE_ADDRESS,
            "base_hex": f"0x{BASE_ADDRESS:016x}",
            "base_alignment_bytes": BASE_ALIGNMENT,
        },
        "cache": {
            "address_bits": ADDRESS_BITS,
            "capacity_bytes": SET_COUNT * WAY_COUNT * BLOCK_BYTES,
            "sets": SET_COUNT,
            "ways": WAY_COUNT,
            "block_bytes": BLOCK_BYTES,
            "offset_bits": BLOCK_BITS,
            "index_bits": SET_BITS,
            "tag_bits": TAG_BITS,
            "set_index_placement": "middle",
            "set_index_bits": "address[11:6]",
            "replacement_policy": "LRU",
        },
        "assumptions": {
            "byte_addressed": True,
            "initial_state": "cold_for_each_function",
            "traffic": "W_loads_only",
            "prefetching": False,
            "other_memory_accesses": False,
        },
        "verification": {
            "small_reference_configurations": checked,
            "row_simulation": "coalesced_consecutive_loads_with_identical_LRU_state",
            "column_simulation": "every_matrix_load_simulated_individually",
            "expected_counts_asserted": True,
            "column_zero_one_reuse_verified": True,
            "full_column_trace_crosschecked_with_list_lru": True,
        },
        "column_analysis": column_analysis,
        "results": results,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, help="write JSON here instead of stdout")
    args = parser.parse_args()
    encoded = json.dumps(verify(), indent=2, ensure_ascii=True) + "\n"
    if args.out is None:
        sys.stdout.write(encoded)
    else:
        args.out.write_text(encoded, encoding="ascii")


if __name__ == "__main__":
    main()
