import random, os, glob

TARGET_LINE_COUNT = 1000

def write_alloc(lines, idx, alloc_sizes, total_mem, alloc_type, size):
    if alloc_sizes[idx] is not None or size + sum(s for s in alloc_sizes if s) > total_mem:
        return False
    lines.append(f"a,{idx}" + (f",{size}" if alloc_type!="slab" else ""))
    alloc_sizes[idx] = size
    return True

def write_free(lines, idx, alloc_sizes):
    if alloc_sizes[idx] is None:
        return False
    lines.append(f"f,{idx}")
    alloc_sizes[idx] = None
    return True

def available_idxs(alloc_sizes, want_alloc):
    return [i for i, s in enumerate(alloc_sizes) if (s is None) == want_alloc]

def balance(lines, alloc_sizes, total_mem, alloc_type):
    while len(lines) < TARGET_LINE_COUNT:
        if random.random() < 0.5:
            free = available_idxs(alloc_sizes, True)
            if free:
                write_alloc(lines, random.choice(free), alloc_sizes, total_mem, alloc_type, random.randint(16, total_mem//4))
        else:
            used = available_idxs(alloc_sizes, False)
            if used:
                write_free(lines, random.choice(used), alloc_sizes)

def ramp_pattern(lines, alloc_sizes, total_mem, alloc_type, max_slots):
    """
    Simulates a gradual increase in memory usage followed by partial releases,
    testing allocator behavior under incremental load and fragmentation.
    
    Steps:
    1. Gradually allocates blocks (20 steps, each adding `max_slots//20` allocations).
    2. Occasionally frees a few blocks (2 per step) to simulate partial releases.
    3. Balances allocations/frees randomly to reach `TARGET_LINE_COUNT`.
    """
    for step in range(20):  # 20 incremental steps
        # Allocate a fraction of `max_slots` in each step
        for _ in range(max_slots // 20):
            free = available_idxs(alloc_sizes, True)
            if not free: 
                break
            write_alloc(lines, random.choice(free), alloc_sizes, total_mem, alloc_type, random.randint(16, total_mem//4))
        
        # Free 2 random blocks per step
        for _ in range(2):
            used = available_idxs(alloc_sizes, False)
            if used:
                write_free(lines, random.choice(used), alloc_sizes)
    
    # Fill remaining lines with random alloc/free operations
    balance(lines, alloc_sizes, total_mem, alloc_type)

def peak_pattern(lines, alloc_sizes, total_mem, alloc_type, max_slots):
    """
    Simulates a sudden spike in allocations (peak usage) followed by a sharp drop,
    testing allocator's ability to handle high load and coalesce freed blocks.
    
    Steps:
    1. Allocates as many blocks as possible (up to `max_slots`).
    2. Randomly selects 25% of blocks to survive ("survivors").
    3. Frees all other blocks, creating a sharp memory drop.
    4. Balances allocations/frees to reach `TARGET_LINE_COUNT`.
    """
    # Allocate until no more slots are available
    for _ in range(max_slots):
        free = available_idxs(alloc_sizes, True)
        if not free: 
            break
        write_alloc(lines, random.choice(free), alloc_sizes, total_mem, alloc_type, random.randint(16, total_mem//4))
    
    # Select 25% of blocks to keep ("survivors")
    used = [i for i, s in enumerate(alloc_sizes) if s is not None]
    survivors_count = max_slots // 4
    k = min(len(used), survivors_count)
    survivors = set(random.sample(used, k)) if k > 0 else set()
    
    # Free all non-survivor blocks
    for i in used:
        if i not in survivors:
            write_free(lines, i, alloc_sizes)
    
    # Fill remaining lines with random alloc/free operations
    balance(lines, alloc_sizes, total_mem, alloc_type)

def plateau_pattern(lines, alloc_sizes, total_mem, alloc_type, max_slots):
    """
    Simulates sustained high memory usage with frequent alloc/free churn,
    testing fragmentation and allocator performance under steady-state load.
    
    Steps:
    1. Allocates as many blocks as possible (up to `max_slots`).
    2. Performs 500 mixed alloc/free operations to simulate churn.
    3. Balances allocations/frees to reach `TARGET_LINE_COUNT`.
    """
    # Allocate until no more slots are available
    for _ in range(max_slots):
        free = available_idxs(alloc_sizes, True)
        if not free: 
            break
        write_alloc(lines, random.choice(free), alloc_sizes, total_mem, alloc_type, random.randint(16, total_mem//4))
    
    # Simulate churn: 500 alloc/free operations
    for _ in range(500):
        # Free a random block
        used = available_idxs(alloc_sizes, False)
        if used:
            write_free(lines, random.choice(used), alloc_sizes)
        
        # Allocate a new block
        free = available_idxs(alloc_sizes, True)
        if free:
            write_alloc(lines, random.choice(free), alloc_sizes, total_mem, alloc_type, random.randint(16, total_mem//4))
    
    # Fill remaining lines with random alloc/free operations
    balance(lines, alloc_sizes, total_mem, alloc_type)

def generate_file(path, pattern, alloc_type):
    print(f"-> {pattern}/{alloc_type}")
    lines = [f"i,{alloc_type}"]
    if alloc_type == "slab":
        slab_sz, n_slabs = 64, 1000
        total_mem = slab_sz * n_slabs
        lines.append(f"p,{slab_sz},{n_slabs}")
        slots = n_slabs
    else:
        total_mem, levels = 2000, 4
        lines.append(f"p,{total_mem},{levels}")
        slots = 2 ** levels

    alloc_sizes = [None] * slots
    max_slots = slots // 2
    {"ramp": ramp_pattern, "peak": peak_pattern, "plateau": plateau_pattern}[pattern](lines, alloc_sizes, total_mem, alloc_type, max_slots)

    with open(path, "w") as f:
        f.write("\n".join(lines))

for f in glob.glob("./benchmarks/generated_*"):
    os.remove(f)
for pat in ["ramp", "peak", "plateau"]:
    for at in ["buddy", "bitmap", "slab"]:
        generate_file(f"./benchmarks/generated_{at}_{pat}.alloc", pat, at)
print("Done.")
