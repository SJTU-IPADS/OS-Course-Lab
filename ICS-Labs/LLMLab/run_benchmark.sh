#!/bin/bash

# Configuration parameters
RUNS=1                # Number of runs
INPUT_TOKENS="31373 612"  # Test input
MODEL_PATH="./gpt2_124M.bin"
NUM_TOKENS_1=10
NUM_TOKENS_2=20
NUM_TOKENS_3=30
KVCACHE_TOKENS=30     # Tokens for KV cache test
CPU_CORES="0-3"      # Limit CPU cores to use
BASELINE_SPEEDUP_1=2  # Minimum speedup for NUM_TOKENS_1
TARGET_SPEEDUP_1=2.5   # Target speedup for NUM_TOKENS_1 (full score)
BASELINE_SPEEDUP_2=2  # Minimum speedup for NUM_TOKENS_2
TARGET_SPEEDUP_2=2.5   # Target speedup for NUM_TOKENS_2 (full score)
BASELINE_SPEEDUP_3=2.2  # Minimum speedup for NUM_TOKENS_3
TARGET_SPEEDUP_3=2.7   # Target speedup for NUM_TOKENS_3 (full score)
TOKENS_1_WEIGHT=25
TOKENS_2_WEIGHT=25
TOKENS_3_WEIGHT=35
KVCACHE_WEIGHT=15
KVCACHE_SPEEDUP=25.00
TIMEOUT=180          # 3 minutes timeout in seconds

# Clean previous results
rm -f benchmark_results.txt opt_output_*.txt gpt_naive_output_*.txt timing.txt

echo "Starting benchmark..."
echo "Number of runs: $RUNS"
echo "Input tokens: $INPUT_TOKENS"
echo "Using CPU cores: $CPU_CORES"

# Global variables to store results
declare -A results

# Function to run benchmark with specific token count
run_benchmark() {
    local num_tokens=$1
    local output_suffix=$2
    
    echo "Testing with $num_tokens tokens..."
    
    # Run optimized version
    echo "Testing optimized version (opt)..."
    total_time_opt=0
    for i in $(seq 1 $RUNS); do
        # Use time command and store output in a temporary file
        { time timeout $TIMEOUT taskset -c $CPU_CORES ./gpt -m $MODEL_PATH -n $num_tokens $INPUT_TOKENS > opt_output_${output_suffix}.txt ; } 2> timing.txt
        
        # Check if the command timed out
        if [ $? -eq 124 ]; then
            echo "Optimized version timed out after $TIMEOUT seconds"
            results["time_opt_${output_suffix}"]="timeout"
            break
        fi
        
        # Extract real time from time command output
        real_time=$(grep "real" timing.txt | awk '{print $2}')
        # Convert time format (Xm Ys.ZZZs) to seconds
        minutes=$(echo $real_time | cut -d'm' -f1)
        seconds=$(echo $real_time | cut -d'm' -f2 | cut -d's' -f1)
        duration=$(echo "$minutes * 60 + $seconds" | bc)
        
        total_time_opt=$(echo "$total_time_opt + $duration" | bc)
        echo "Run $i: $duration seconds"
    done
    
    # Only calculate average if no timeout occurred
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ]; then
        avg_time_opt=$(echo "scale=6; $total_time_opt / $RUNS" | bc)
        results["time_opt_${output_suffix}"]=$avg_time_opt
    fi

    # Run naive version
    echo -e "\nTesting naive version (gpt_naive)..."
    total_time_naive=0
    for i in $(seq 1 $RUNS); do
        # Use time command and store output in a temporary file
        { time timeout $TIMEOUT taskset -c $CPU_CORES ./gpt_naive -m $MODEL_PATH -n $num_tokens $INPUT_TOKENS > gpt_naive_output_${output_suffix}.txt ; } 2> timing.txt
        
        # Check if the command timed out
        if [ $? -eq 124 ]; then
            echo "Naive version timed out after $TIMEOUT seconds"
            results["time_naive_${output_suffix}"]="timeout"
            break
        fi
        
        # Extract real time from time command output
        real_time=$(grep "real" timing.txt | awk '{print $2}')
        # Convert time format (Xm Ys.ZZZs) to seconds
        minutes=$(echo $real_time | cut -d'm' -f1)
        seconds=$(echo $real_time | cut -d'm' -f2 | cut -d's' -f1)
        duration=$(echo "$minutes * 60 + $seconds" | bc)
        
        total_time_naive=$(echo "$total_time_naive + $duration" | bc)
        echo "Run $i: $duration seconds"
    done
    
    # Only calculate average if no timeout occurred
    if [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        avg_time_naive=$(echo "scale=6; $total_time_naive / $RUNS" | bc)
        results["time_naive_${output_suffix}"]=$avg_time_naive
    fi

    # Calculate speedup only if both versions completed without timeout
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ] && [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        speedup=$(echo "scale=2; $avg_time_naive / $avg_time_opt" | bc)
        results["speedup_${output_suffix}"]=$speedup
        
        # Check output correctness
        if diff opt_output_${output_suffix}.txt gpt_naive_output_${output_suffix}.txt >/dev/null; then
            correctness="correct"
        else
            correctness="incorrect"
        fi
        results["correctness_${output_suffix}"]=$correctness
    else
        results["speedup_${output_suffix}"]="N/A"
        results["correctness_${output_suffix}"]="N/A"
    fi

    # Print intermediate results for debugging
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ]; then
        echo "Optimized Time: ${results["time_opt_${output_suffix}"]}"
    else
        echo "Optimized Time: TIMEOUT"
    fi
    
    if [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        echo "Naive Time: ${results["time_naive_${output_suffix}"]}"
    else
        echo "Naive Time: TIMEOUT"
    fi
    
    echo "Speedup: ${results["speedup_${output_suffix}"]}"
    echo "Correctness: ${results["correctness_${output_suffix}"]}"
}

# Function to run KV cache benchmark
run_kvcache_benchmark() {
    local num_tokens=$1
    local output_suffix="kvcache"
    
    echo "Testing KV cache with $num_tokens tokens..."
    
    # Run optimized version with KV cache
    echo "Testing optimized version (opt) with KV cache..."
    total_time_opt=0
    for i in $(seq 1 $RUNS); do
        # Use time command and store output in a temporary file
        { time timeout $TIMEOUT taskset -c $CPU_CORES ./gpt -m $MODEL_PATH -n $num_tokens -kvcache $INPUT_TOKENS > opt_output_${output_suffix}.txt ; } 2> timing.txt
        
        # Check if the command timed out
        if [ $? -eq 124 ]; then
            echo "Optimized version with KV cache timed out after $TIMEOUT seconds"
            results["time_opt_${output_suffix}"]="timeout"
            break
        fi
        
        # Extract real time from time command output
        real_time=$(grep "real" timing.txt | awk '{print $2}')
        # Convert time format (Xm Ys.ZZZs) to seconds
        minutes=$(echo $real_time | cut -d'm' -f1)
        seconds=$(echo $real_time | cut -d'm' -f2 | cut -d's' -f1)
        duration=$(echo "$minutes * 60 + $seconds" | bc)
        
        total_time_opt=$(echo "$total_time_opt + $duration" | bc)
        echo "Run $i: $duration seconds"
    done
    
    # Only calculate average if no timeout occurred
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ]; then
        avg_time_opt=$(echo "scale=6; $total_time_opt / $RUNS" | bc)
        results["time_opt_${output_suffix}"]=$avg_time_opt
    fi

    # Run naive version (without KV cache parameter as it won't support it)
    echo -e "\nTesting naive version (gpt_naive)..."
    total_time_naive=0
    for i in $(seq 1 $RUNS); do
        # Use time command and store output in a temporary file
        { time timeout $TIMEOUT taskset -c $CPU_CORES ./gpt_naive -m $MODEL_PATH -n $num_tokens $INPUT_TOKENS > gpt_naive_output_${output_suffix}.txt ; } 2> timing.txt
        
        # Check if the command timed out
        if [ $? -eq 124 ]; then
            echo "Naive version timed out after $TIMEOUT seconds"
            results["time_naive_${output_suffix}"]="timeout"
            break
        fi
        
        # Extract real time from time command output
        real_time=$(grep "real" timing.txt | awk '{print $2}')
        # Convert time format (Xm Ys.ZZZs) to seconds
        minutes=$(echo $real_time | cut -d'm' -f1)
        seconds=$(echo $real_time | cut -d'm' -f2 | cut -d's' -f1)
        duration=$(echo "$minutes * 60 + $seconds" | bc)
        
        total_time_naive=$(echo "$total_time_naive + $duration" | bc)
        echo "Run $i: $duration seconds"
    done
    
    # Only calculate average if no timeout occurred
    if [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        avg_time_naive=$(echo "scale=6; $total_time_naive / $RUNS" | bc)
        results["time_naive_${output_suffix}"]=$avg_time_naive
    fi

    # Calculate speedup only if both versions completed without timeout
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ] && [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        speedup=$(echo "scale=2; $avg_time_naive / $avg_time_opt" | bc)
        results["speedup_${output_suffix}"]=$speedup
        
        # Check output correctness
        if diff opt_output_${output_suffix}.txt gpt_naive_output_${output_suffix}.txt >/dev/null; then
            correctness="correct"
        else
            correctness="incorrect"
        fi
        results["correctness_${output_suffix}"]=$correctness
    else
        results["speedup_${output_suffix}"]="N/A"
        results["correctness_${output_suffix}"]="N/A"
    fi

    # Print intermediate results for debugging
    if [ "${results["time_opt_${output_suffix}"]}" != "timeout" ]; then
        echo "Optimized Time (with KV cache): ${results["time_opt_${output_suffix}"]}"
    else
        echo "Optimized Time (with KV cache): TIMEOUT"
    fi
    
    if [ "${results["time_naive_${output_suffix}"]}" != "timeout" ]; then
        echo "Naive Time: ${results["time_naive_${output_suffix}"]}"
    else
        echo "Naive Time: TIMEOUT"
    fi
    
    echo "Speedup: ${results["speedup_${output_suffix}"]}"
    echo "Correctness: ${results["correctness_${output_suffix}"]}"
}

# Run benchmarks for all token counts
echo -e "\nRunning benchmark for $NUM_TOKENS_1 tokens..."
run_benchmark $NUM_TOKENS_1 $NUM_TOKENS_1

echo -e "\nRunning benchmark for $NUM_TOKENS_2 tokens..."
run_benchmark $NUM_TOKENS_2 $NUM_TOKENS_2

echo -e "\nRunning benchmark for $NUM_TOKENS_3 tokens..."
run_benchmark $NUM_TOKENS_3 $NUM_TOKENS_3

echo -e "\nRunning KV cache benchmark for $KVCACHE_TOKENS tokens..."
run_kvcache_benchmark $KVCACHE_TOKENS

# Get results from associative array
avg_time_opt_1=${results["time_opt_$NUM_TOKENS_1"]}
avg_time_naive_1=${results["time_naive_$NUM_TOKENS_1"]}
speedup_1=${results["speedup_$NUM_TOKENS_1"]}
correctness_1=${results["correctness_$NUM_TOKENS_1"]}

avg_time_opt_2=${results["time_opt_$NUM_TOKENS_2"]}
avg_time_naive_2=${results["time_naive_$NUM_TOKENS_2"]}
speedup_2=${results["speedup_$NUM_TOKENS_2"]}
correctness_2=${results["correctness_$NUM_TOKENS_2"]}

avg_time_opt_3=${results["time_opt_$NUM_TOKENS_3"]}
avg_time_naive_3=${results["time_naive_$NUM_TOKENS_3"]}
speedup_3=${results["speedup_$NUM_TOKENS_3"]}
correctness_3=${results["correctness_$NUM_TOKENS_3"]}

avg_time_opt_kvcache=${results["time_opt_kvcache"]}
avg_time_naive_kvcache=${results["time_naive_kvcache"]}
speedup_kvcache=${results["speedup_kvcache"]}
correctness_kvcache=${results["correctness_kvcache"]}

# Calculate final score
if [ "$correctness_1" = "incorrect" ] || [ "$correctness_2" = "incorrect" ] || [ "$correctness_3" = "incorrect" ] || [ "$speedup_1" = "N/A" ] || [ "$speedup_2" = "N/A" ] || [ "$speedup_3" = "N/A" ]; then
    final_score=0
else
    # Calculate score for test 1
    if [ $(echo "$speedup_1 < $BASELINE_SPEEDUP_1" | bc) -eq 1 ]; then
        score_1=0
    else
        # Linear scoring between BASELINE and TARGET
        score_1=$(echo "scale=4; ($speedup_1 - $BASELINE_SPEEDUP_1) / ($TARGET_SPEEDUP_1 - $BASELINE_SPEEDUP_1)" | bc)
        # Limit score to 1.0
        if [ $(echo "$score_1 > 1" | bc) -eq 1 ]; then
            score_1=1.00
        fi
    fi

    # Calculate score for test 2
    if [ $(echo "$speedup_2 < $BASELINE_SPEEDUP_2" | bc) -eq 1 ]; then
        score_2=0
    else
        # Linear scoring between BASELINE and TARGET
        score_2=$(echo "scale=4; ($speedup_2 - $BASELINE_SPEEDUP_2) / ($TARGET_SPEEDUP_2 - $BASELINE_SPEEDUP_2)" | bc)
        # Limit score to 1.0
        if [ $(echo "$score_2 > 1" | bc) -eq 1 ]; then
            score_2=1.00
        fi
    fi

    # Calculate score for test 3
    if [ $(echo "$speedup_3 < $BASELINE_SPEEDUP_3" | bc) -eq 1 ]; then
        score_3=0
    else
        # Linear scoring between BASELINE and TARGET
        score_3=$(echo "scale=4; ($speedup_3 - $BASELINE_SPEEDUP_3) / ($TARGET_SPEEDUP_3 - $BASELINE_SPEEDUP_3)" | bc)
        # Limit score to 1.0
        if [ $(echo "$score_3 > 1" | bc) -eq 1 ]; then
            score_3=1.00
        fi
    fi

    # if the speedup is greater than KVCACHE_SPEEDUP, add the KVCACHE_WEIGHT
    if [ $(echo "$speedup_kvcache > $KVCACHE_SPEEDUP" | bc) -eq 1 ]; then
        if [ "$correctness_kvcache" = "correct" ]; then
            score_4=$KVCACHE_WEIGHT
        else
            score_4=0
        fi
    else
        score_4=0
    fi
    
    # Calculate weighted score
    final_score=$(echo "scale=2; ($score_1 * $TOKENS_1_WEIGHT + $score_2 * $TOKENS_2_WEIGHT + $score_3 * $TOKENS_3_WEIGHT + $score_4)" | bc)
fi

# Output results
{
    echo -e "\nBenchmark Results:"
    echo "Using CPU cores: $CPU_CORES"
    
    echo -e "\nResults for $NUM_TOKENS_1 tokens:"
    if [ "$avg_time_opt_1" = "timeout" ]; then
        echo "Optimized version (opt): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Optimized version (opt) average runtime: $avg_time_opt_1 seconds"
    fi
    
    if [ "$avg_time_naive_1" = "timeout" ]; then
        echo "Naive version (gpt_naive): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Naive version (gpt_naive) average runtime: $avg_time_naive_1 seconds"
    fi
    
    if [ "$speedup_1" = "N/A" ]; then
        echo "Speedup: N/A (timeout occurred)"
    else
        echo "Speedup: ${speedup_1}x (baseline: ${BASELINE_SPEEDUP_1}x)"
    fi
    echo "Output consistency: $correctness_1"
    
    echo -e "\nResults for $NUM_TOKENS_2 tokens:"
    if [ "$avg_time_opt_2" = "timeout" ]; then
        echo "Optimized version (opt): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Optimized version (opt) average runtime: $avg_time_opt_2 seconds"
    fi
    
    if [ "$avg_time_naive_2" = "timeout" ]; then
        echo "Naive version (gpt_naive): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Naive version (gpt_naive) average runtime: $avg_time_naive_2 seconds"
    fi
    
    if [ "$speedup_2" = "N/A" ]; then
        echo "Speedup: N/A (timeout occurred)"
    else
        echo "Speedup: ${speedup_2}x (baseline: ${BASELINE_SPEEDUP_2}x)"
    fi
    echo "Output consistency: $correctness_2"
    
    echo -e "\nResults for $NUM_TOKENS_3 tokens:"
    if [ "$avg_time_opt_3" = "timeout" ]; then
        echo "Optimized version (opt): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Optimized version (opt) average runtime: $avg_time_opt_3 seconds"
    fi
    
    if [ "$avg_time_naive_3" = "timeout" ]; then
        echo "Naive version (gpt_naive): TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Naive version (gpt_naive) average runtime: $avg_time_naive_3 seconds"
    fi
    
    if [ "$speedup_3" = "N/A" ]; then
        echo "Speedup: N/A (timeout occurred)"
    else
        echo "Speedup: ${speedup_3}x (baseline: ${BASELINE_SPEEDUP_3}x)"
    fi
    echo "Output consistency: $correctness_3"

    echo -e "\nResults for KV Cache test ($KVCACHE_TOKENS tokens):"
    if [ "$avg_time_opt_kvcache" = "timeout" ]; then
        echo "Optimized version with KV cache: TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Optimized version with KV cache average runtime: $avg_time_opt_kvcache seconds"
    fi
    
    if [ "$avg_time_naive_kvcache" = "timeout" ]; then
        echo "Naive version: TIMEOUT (exceeded $TIMEOUT seconds)"
    else
        echo "Naive version average runtime: $avg_time_naive_kvcache seconds"
    fi
    
    if [ "$speedup_kvcache" = "N/A" ]; then
        echo "Speedup: N/A (timeout occurred)"
        echo "KV Cache score: 0 (timeout occurred)"
    else
        echo "Speedup: ${speedup_kvcache}x (target: ${KVCACHE_SPEEDUP}x)"
        if [ $(echo "$speedup_kvcache > $KVCACHE_SPEEDUP" | bc) -eq 1 ]; then
            echo "KV Cache score: $KVCACHE_WEIGHT (target reached)"
        else
            echo "KV Cache score: 0 (target not reached)"
        fi
    fi
    echo "Output consistency: $correctness_kvcache"
    
    echo "Final Score: $final_score"
    
    # If outputs don't match, save the differences
    if [ "$correctness_1" = "incorrect" ]; then
        echo -e "\nOutput differences for $NUM_TOKENS_1 tokens:"
        diff opt_output_${NUM_TOKENS_1}.txt gpt_naive_output_${NUM_TOKENS_1}.txt
    fi
    
    if [ "$correctness_2" = "incorrect" ]; then
        echo -e "\nOutput differences for $NUM_TOKENS_2 tokens:"
        diff opt_output_${NUM_TOKENS_2}.txt gpt_naive_output_${NUM_TOKENS_2}.txt
    fi

    if [ "$correctness_3" = "incorrect" ]; then
        echo -e "\nOutput differences for $NUM_TOKENS_3 tokens:"
        diff opt_output_${NUM_TOKENS_3}.txt gpt_naive_output_${NUM_TOKENS_3}.txt
    fi
    
    if [ "$correctness_kvcache" = "incorrect" ]; then
        echo -e "\nOutput differences for KV Cache test:"
        diff opt_output_kvcache.txt gpt_naive_output_kvcache.txt
    fi
} | tee benchmark_results.txt