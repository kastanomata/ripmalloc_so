import os
import matplotlib.pyplot as plt

def parse_log_file(filepath):
    allocations = []
    frag_list = []  # List to track internal fragmentation
    failure_indices = []  # Track operation indices of fragmentation failures
    current_alloc = {
        'allocator': None,
        'params': {},
        'mem': {},
        'allocated': 0,
        'total_size': 0
    }
    total_alloc_requests = 0  # Total size of successful allocations
    operation_count = 0  # Track total operations
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                # Parse initialization comments
                if line.startswith('# type='):
                    current_alloc['allocator'] = line.split('=')[1].strip()
                elif line.startswith('# total_size='):
                    parts = line.split(',')
                    current_alloc['total_size'] = int(parts[0].split('=')[1])
                    current_alloc['params']['memory_size'] = current_alloc['total_size']
                    current_alloc['params']['max_levels'] = int(parts[1].split('=')[1])
                continue
            
            parts = line.split(',')
            cmd = parts[0]
            operation_count += 1  # Count this operation
            
            if cmd == 'a':
                # Allocation command: a,<index>,<size>,<status>,<frag>
                if len(parts) < 5:
                    continue
                status = parts[3]
                frag = int(parts[4])
                
                # Track fragmentation failures (status=1 with enough total memory)
                if status == '1' and (current_alloc['total_size'] - current_alloc['allocated'] >= int(parts[2])):
                    failure_indices.append(operation_count - 1)  # Record operation index
                
                # Skip failed operations
                if status != '0':
                    continue
                
                idx = int(parts[1])
                size = int(parts[2])
                
                current_alloc['mem'][idx] = size
                current_alloc['allocated'] += size
                allocations.append(current_alloc['allocated'])
                frag_list.append(frag)
                total_alloc_requests += size
                
            elif cmd == 'f':
                # Free command: f,<index>,<status>,<frag>
                if len(parts) < 4:
                    continue
                status = parts[2]
                frag = int(parts[3])
                
                # Skip failed operations
                if status != '0':
                    continue
                    
                idx = int(parts[1])
                size = current_alloc['mem'].pop(idx, 0)
                current_alloc['allocated'] -= size
                allocations.append(current_alloc['allocated'])
                frag_list.append(frag)
    
    return allocations, frag_list, current_alloc['total_size'], total_alloc_requests, failure_indices

def produce_graph():
    benchmarks_dir = './benchmarks'
    for filename in os.listdir(benchmarks_dir):
        if filename.endswith('.log'):
            filepath = os.path.join(benchmarks_dir, filename)
            try:
                allocations, frag_list, total_size, total_alloc_requests, failure_indices = parse_log_file(filepath)
                
                # Calculate statistics
                highest_peak = max(allocations) if allocations else 0
                num_operations = len(allocations)
                avg_frag = sum(frag_list) / len(frag_list) if frag_list else 0
                max_frag = max(frag_list) if frag_list else 0
                
                # Function to format bytes with appropriate unit
                def format_bytes(size):
                    unit = 'bytes'
                    for u in ['bytes', 'KB', 'MB', 'GB']:
                        unit = u
                        if size < 1024.0 or unit == 'GB':
                            break
                        size /= 1024.0
                    return f"{size:.2f} {unit}" if unit != 'bytes' else f"{int(size)} {unit}"
                
                fig, ax1 = plt.subplots(figsize=(16, 12))  # Increased figure size
                
                # Plot allocated memory (primary axis)
                ax1.plot(range(len(allocations)), allocations, marker='o', markersize=3, 
                        linestyle='-', linewidth=1, color='b', label='Allocated Memory')
                ax1.axhline(y=total_size, color='r', linestyle='--', 
                        label=f'Total Memory: {format_bytes(total_size)}')
                                
                ax1.set_title(f'Memory Allocation Pattern\n{filename}', pad=20)
                ax1.set_xlabel('Operation Sequence')
                ax1.set_ylabel('Memory Allocated (bytes)', color='b')
                ax1.tick_params(axis='y', labelcolor='b')
                ax1.grid(True, which='both', linestyle='--', alpha=0.7)

                # Plot fragmentation on the same axis
                ax1.plot(range(len(frag_list)), frag_list, marker='s', markersize=2, 
                         linestyle='-', linewidth=1, color='purple', label='Internal Fragmentation')

                # Add vertical lines for fragmentation failures
                for idx in failure_indices:
                    ax1.axvline(x=idx, color='red', linestyle='-', alpha=0.7, linewidth=0.5)
                
                # Add text-box with statistics
                stats_text = (
                    f"Total Allocated: {format_bytes(total_alloc_requests)}\n"
                    f"Highest Peak:    {format_bytes(highest_peak)}\n"
                    f"Max Frag:        {format_bytes(max_frag)}\n"
                    f"Avg Frag:        {format_bytes(avg_frag)}\n"
                    f"Operations:      {num_operations:,}\n"
                    f"Utilization:     {highest_peak/total_size:.1%}\n"
                    f"Frag Failures:   {len(failure_indices)}"
                )
                
                ax1.annotate(
                    stats_text,
                    xy=(0.1, 0.85), xycoords='axes fraction',
                    fontsize=11, family='monospace',
                    horizontalalignment='left', verticalalignment='top',
                    bbox=dict(alpha=0.5, pad=1, boxstyle='square')
                )

                ax1.legend(loc='upper right')
                
                plt.tight_layout(rect=[0, 0.05, 1, 1])  # Adjust for footer
                
                outname = filename.replace('.log', '.png')
                outpath = os.path.join(outname)
                plt.savefig(outpath, dpi=150, bbox_inches='tight')
                plt.close()
                
                print(f"Generated plot for {filename}")
                
            except Exception as e:
                print(f"Error processing {filename}: {str(e)}")

if __name__ == "__main__":
    produce_graph()
    print("All graphs generated successfully.")