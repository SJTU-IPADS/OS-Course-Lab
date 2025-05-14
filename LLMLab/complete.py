import tiktoken
import subprocess
import sys
import argparse

# Create a parser for command line arguments
parser = argparse.ArgumentParser(description='GPT-2 text completion')
parser.add_argument('-m', '--model', default=None, help='Path to model file')
parser.add_argument('-n', '--num-tokens', type=int, default=10, help='Maximum number of tokens to generate')
parser.add_argument('-o', '--optimize', action='store_true', help='Enable optimization')
parser.add_argument('-k', '--kv-cache', action='store_true', help='Enable kv cache')

args = parser.parse_args()

text = input("Text to complete: ")
enc = tiktoken.get_encoding("gpt2")

tokens = [
    str(tok) for tok in enc.encode(text)
]

# Build the command based on the parameters
cmd = ["./gpt_naive"]
if args.optimize:
    cmd = ["./gpt"]
if args.model:
    cmd.extend(["-m", args.model])
if args.num_tokens != 10:  # Only add -n parameter when it's not the default value
    cmd.extend(["-n", str(args.num_tokens)])
if args.kv_cache and args.optimize:
    cmd.extend(["-kvcache"])
cmd.extend(tokens)

proc = subprocess.Popen(
    cmd,
    stdout=subprocess.PIPE,
    text=True
)

while (line := proc.stdout.readline()):
    token = int(line)
    print(enc.decode([token]), end='', flush=True)
