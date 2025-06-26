import os
import matplotlib.pyplot as plt

def parse_log_file(filepath):
    allocations = []
    current_alloc = {
        'allocator': None,
        'params': {},
        'mem': {},
        'allocated': 0,
        'total_size': 0
    }
    
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
            
            if cmd == 'a':
                # Allocation command: a,<index>,<size>,<status>
                idx = int(parts[1])
                size = int(parts[2])
                current_alloc['mem'][idx] = size
                current_alloc['allocated'] += size
                allocations.append(current_alloc['allocated'])
                
            elif cmd == 'f':
                # Free command: f,<index>,<status>
                idx = int(parts[1])
                size = current_alloc['mem'].pop(idx, 0)
                current_alloc['allocated'] -= size
                allocations.append(current_alloc['allocated'])
    
    return allocations, current_alloc['total_size']

def produce_graph():
    benchmarks_dir = './benchmarks'
    for filename in os.listdir(benchmarks_dir):
        if filename.endswith('.log'):
            filepath = os.path.join(benchmarks_dir, filename)
            try:
                allocations, total_size = parse_log_file(filepath)
                
                # Calculate statistics
                total_allocated = sum(size for size in allocations if size > 0)
                highest_peak = max(allocations) if allocations else 0
                num_operations = len(allocations)
                
                # Function to format bytes with appropriate unit
                def format_bytes(size):
                    for unit in ['bytes', 'KB', 'MB', 'GB']:
                        if size < 1024.0 or unit == 'GB':
                            break
                        size /= 1024.0
                    return f"{size:.2f} {unit}" if unit != 'bytes' else f"{int(size)} {unit}"
                
                plt.figure(figsize=(10, 7.5))  # Slightly taller figure
                plt.plot(range(len(allocations)), allocations, marker='o', markersize=3, linestyle='-', linewidth=1)
                
                plt.axhline(y=total_size, color='r', linestyle='--', 
                           label=f'Total Memory: {format_bytes(total_size)}')
                
                plt.title(f'Memory Allocation Pattern\n{filename}', pad=20)
                plt.xlabel('Operation Sequence')
                plt.ylabel('Memory Allocated (bytes)')
                plt.grid(True, which='both', linestyle='--', alpha=0.7)
                
                # Add well-formatted text box with statistics
                stats_text = (
                    f"Total Allocated: {format_bytes(total_allocated)}\n"
                    f"Highest Peak:    {format_bytes(highest_peak)}\n"
                    f"Operations:      {num_operations:,}\n"
                    f"Utilization:     {highest_peak/total_size:.1%}"
                )
                
                plt.annotate(
                    stats_text,
                    xy=(0.1, 0.90), xycoords='axes fraction',
                    fontsize=11, family='monospace',
                    horizontalalignment='left', verticalalignment='top',
                    bbox=dict(alpha=0.5, pad=1, boxstyle='square')
                )

                
                plt.legend(loc='upper right')
                plt.tight_layout(rect=[0, 0.05, 1, 1])  # Adjust for footer
                
                outname = filename.replace('.log', '.png')
                plt.savefig(os.path.join(outname), dpi=150, bbox_inches='tight')
                plt.close()
                
                print(f"Generated plot for {filename}")
                
            except Exception as e:
                print(f"Error processing {filename}: {str(e)}")

if __name__ == "__main__":
    produce_graph()