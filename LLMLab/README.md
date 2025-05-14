#####################################################################
# CS:APP LLM Lab
# Handout files for students
#
# Modified Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS), 
# Shanghai Jiao Tong University (SJTU)
# Modified from https://jyywiki.cn/OS/2024/labs/M3.md.
#
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.
#
# Original copyright notice retained above as required.
######################################################################

# GPT-2 Performance Benchmark

This project contains performance comparison benchmarks for two implementations of the GPT-2 model:
- `gpt.c`: Your optimized version (to be implemented with multi-threading optimization)
- `gpt_naive.c`: Naive version (basic implementation, provided as baseline)

## Instructions

1. Compile both versions:
```bash
make all
```

2. Run single benchmark test:
```bash
make benchmark
```

3. Run optimized version with custom token count:
```bash
make run n=<token_count>
```
For example, to generate 20 tokens:
```bash
make run n=20
```
Note: This will run the optimized version only, using CPU cores 0-3 and the default input tokens "31373 612".

## Benchmark Details

The benchmark script will run three different token generation tests:
1. Test 1: Generate 10 tokens (25% weight)
2. Test 2: Generate 20 tokens (25% weight)
3. Test 3: Generate 30 tokens (35% + 15% weight)

For each test, the script will:
1. Run both optimized and naive versions
2. Calculate speedup ratio compared to the naive version
3. Verify output consistency between optimized and naive versions
4. Generate a detailed benchmark report

## Scoring System

The final score is calculated based on three factors:

1. Token-10 test (25% weight):
   - Minimum required speedup: 2x
   - Target speedup: 2.5x
   - Score calculation:
     - If speedup < 2x: score = 0
     - If 2x ≤ speedup ≤ 2.5x: score = (speedup - 2) / (2.5 - 2)
     - If speedup > 2.5x: score = 1.0

2. Token-20 test (25% weight):
   - Minimum required speedup: 2x
   - Target speedup: 2.5x
   - Score calculation:
     - If speedup < 2x: score = 0
     - If 2x ≤ speedup ≤ 2.5x: score = (speedup - 2) / (2.5 - 2)
     - If speedup > 2.5x: score = 1.0

3. Token-30 test (35% + 15% weight):
   - Minimum required speedup: 2.2x
   - Target speedup: 2.7x
   - Score calculation:
     - If speedup < 2.2x: score = 0
     - If 2.2x ≤ speedup ≤ 2.7x: score = (speedup - 2.2) / (2.7 - 2.2)
     - If speedup > 2.7x: score = 1.0

4. KVCACHE test (15% weight):
   - Minimum required speedup: 25.00x
   - Score calculation:
     - If speedup < 25.00x: score = 0
     - If speedup ≥ 25.00x: score = 1.0

Final score = (Score1 * 25 + Score2 * 25 + Score3 * 35 + Score4 * 15)

Note: The score will be 0 if:
- Any test's output is incorrect (differs from naive version)

## Configuration

You can modify the following parameters in `run_benchmark.sh`:
- `RUNS`: Number of test runs (default: 5)
- `INPUT_TOKENS`: Test input tokens (default: "31373 612")
- `CPU_CORES`: CPU cores to use (default: "0-3")
- `MODEL_PATH`: Path to model file (default: "./gpt2_124M.bin")
- `NUM_TOKENS_1`: Number of tokens for Test 1 (default: 10)
- `NUM_TOKENS_2`: Number of tokens for Test 2 (default: 20)
- `NUM_TOKENS_3`: Number of tokens for Test 3 (default: 30)

While we will use the default parameters for grading purposes, you are free to modify these parameters when testing your code. For quicker testing, we suggest setting `RUNS` to 1.

## Test by yourself

You can test your code by yourself by running the following command:
```bash
time taskset -c 0-3 ./gpt -n $num_tokens -kvcache $INPUT_TOKENS 
```
or
```bash
time taskset -c 0-3 ./gpt -n $num_tokens $INPUT_TOKENS 
```
which will run the optimized version with KV cache or not and restrict the program to use only the first 4 cores. It will output the time used to generate the tokens to the console.

You can also run the naive version by:
```bash
time taskset -c 0-3 ./gpt_naive -n $num_tokens $INPUT_TOKENS 
```

You can simply run those commands above while developing your code.

## Compilation Flags

The following compilation flags are used:
- `-O3`: Maximum optimization level
- `-g`: Include debugging information
- `-Wall`: Enable all warnings
- `-pthread`: Enable multi-threading support

## Output Files

The benchmark will generate the following files:
- `benchmark_results.txt`: Detailed benchmark results
- `opt_output_*.txt`: Output from optimized version for each token test
- `gpt_naive_output_*.txt`: Output from naive version for each token test

## Cleanup

Clean up compiled files and test results:
```bash
make clean
```
