import re


def parse_malloc_log(log_file_path, max_operations):
    operations = []
    ptr_to_index = {}
    max_index = -1

    with open(log_file_path, "r") as log_file:
        for line in log_file:
            if len(operations) >= max_operations:
                break
            if line.startswith("[INTERCEPT]"):
                if "malloc" in line:
                    match = re.search(r"malloc\((\d+)\) = (0x[0-9a-f]+)", line)
                    if match:
                        size = int(match.group(1))
                        ptr = match.group(2)
                        # if ptr already exists, ignore
                        if ptr not in ptr_to_index:
                            max_index += 1
                            ptr_to_index[ptr] = max_index
                            operations.append(("a", max_index, size))
                elif "realloc" in line:
                    match = re.search(
                        r"realloc\((0x[0-9a-f]+|\(nil\)), (\d+)\) = (0x[0-9a-f]+)", line
                    )
                    if match:
                        old_ptr = match.group(1)
                        size = int(match.group(2))
                        new_ptr = match.group(3)
                        if old_ptr == "(nil)":
                            # if ptr already exists, ignore
                            if new_ptr not in ptr_to_index:
                                max_index += 1
                                ptr_to_index[new_ptr] = max_index
                                operations.append(("a", max_index, size))
                        elif old_ptr in ptr_to_index:
                            index = ptr_to_index[old_ptr]
                            ptr_to_index.pop(old_ptr)
                            ptr_to_index[new_ptr] = index
                            operations.append(("r", index, size))
                elif "free" in line:
                    match = re.search(r"free\((0x[0-9a-f]+)\)", line)
                    if match:
                        ptr = match.group(1)
                        if ptr in ptr_to_index:
                            index = ptr_to_index[ptr]
                            operations.append(("f", index))
                            ptr_to_index.pop(ptr)

    return operations, max_index + 1


def generate_trace_file(operations, num_ids, trace_file_path):
    with open(trace_file_path, "w") as trace_file:
        num_ops = len(operations)
        trace_file.write(f"0\n{num_ids}\n{num_ops}\n1\n")
        for op in operations:
            if op[0] == "a":
                trace_file.write(f"a {op[1]} {op[2]}\n")
            elif op[0] == "r":
                trace_file.write(f"r {op[1]} {op[2]}\n")
            elif op[0] == "f":
                trace_file.write(f"f {op[1]}\n")


log_file_path = "/home/ics/malloclab/malloc-perf/malloc_log"
trace_file_path = "/home/ics/malloclab/malloc-perf/trace_file.txt"

operations, num_ids = parse_malloc_log(log_file_path, max_operations=10000)
generate_trace_file(operations, num_ids, trace_file_path)
