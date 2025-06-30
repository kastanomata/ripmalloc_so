from pathlib import Path
import random

allocator_type = "bitmap"
init_line = f"i,{allocator_type}\n"
memory_size = 16276
num_levels = 10
parameter_line = f"p,{memory_size},{num_levels}\n" 
allocator_header = init_line + parameter_line


# Calculate minimum bucket size based on memory size and num_levels
min_bucket_size = memory_size // (2 ** (num_levels - 1))
# Ensure minimum bucket size is at least 8
min_bucket_size = max(16, min_bucket_size)

# Generate base sizes as powers of two starting from min_bucket_size
base_sizes = [min_bucket_size * (2 ** i) for i in range(int(num_levels*7/10))]

# Reinitialize
trace_lines = []
allocated_indices = set()
freed_indices = set()
index = 0
line_count = 0
max_lines = 1000

# Generate randomized allocations with interleaved frees
while line_count < max_lines - 100:  # reserve lines to ensure room for all frees
    # Allocate a random number of allocations per iteration
    num_allocs = random.randint(1, 5)
    for _ in range(num_allocs):
        if index not in allocated_indices or index in freed_indices:
            base_size = base_sizes[index % len(base_sizes)]
            proposed_size = base_size + random.randint(-base_size // 4, base_size // 4)
            size = max(8, proposed_size)
            trace_lines.append(f"a,{index},{size}")
            allocated_indices.add(index)
            if index in freed_indices:
                freed_indices.remove(index)
            line_count += 1
            index += 1
            if line_count >= max_lines - 100:
                break
    # Adjust index so it doesn't increment twice per loop
    index -= 1
    available_to_free = len(allocated_indices - freed_indices)
    if allocated_indices - freed_indices and available_to_free > 10:
        # Increase number to free as line_count increases
        base_free = 3
        # Scale up to a max of 30 as line_count approaches max_lines
        scale = min(1.0, line_count / (max_lines - 100))
        max_free = int(base_free + scale * 27)  # 3 to 30
        num_to_free = random.randint(base_free, min(max_free, available_to_free))
        to_free = random.sample(sorted(allocated_indices - freed_indices), num_to_free)
        for i in to_free:
            trace_lines.append(f"f,{i}")
            freed_indices.add(i)
            line_count += 1
            if line_count >= max_lines - 100:
                break
    index += 1

# Free all remaining allocated indices explicitly
remaining_to_free = sorted(allocated_indices - freed_indices)
for i in remaining_to_free:
    trace_lines.append(f"f,{i}")
    line_count += 1
    if line_count >= max_lines:
        break

# Combine header and trace
full_trace = allocator_header + "\n".join(trace_lines) + "\n"

# Save to file
fixed_output_path = Path("./benchmarks/trace.alloc")
fixed_output_path.parent.mkdir(parents=True, exist_ok=True)
fixed_output_path.write_text(full_trace)