import os
import matplotlib.pyplot as plt
import re
from os.path import join

def parse_log_file(filepath):
    allocations = []
    frag_list = []
    failure_indices = []
    current_alloc = {
        'allocator': None,
        'params': {},
        'mem': {},
        'allocated': 0,
        'total_size': 0
    }
    total_alloc_requests = 0
    operation_count = 0
    elapsed_seconds = user_seconds = sys_seconds = None

    with open(filepath, 'r') as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
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
            operation_count += 1

            if cmd == 'a':
                if len(parts) < 5:
                    continue
                status = parts[3]
                frag = int(parts[4])
                if status == '1' and (current_alloc['total_size'] - current_alloc['allocated'] >= int(parts[2])):
                    failure_indices.append(operation_count - 1)
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
                if len(parts) < 4:
                    continue
                status = parts[2]
                frag = int(parts[3])
                if status != '0':
                    continue
                idx = int(parts[1])
                size = current_alloc['mem'].pop(idx, 0)
                current_alloc['allocated'] -= size
                allocations.append(current_alloc['allocated'])
                frag_list.append(frag)

        last_line = lines[-1].strip()
        match = re.match(
            r"# elapsed_seconds=([\d\.]+) user_seconds=([\d\.]+) sys_seconds=([\d\.]+)", last_line
        )
        if match:
            elapsed_seconds = float(match.group(1))
            user_seconds = float(match.group(2))
            sys_seconds = float(match.group(3))

    # Return allocator name as first value
    return (
        current_alloc['allocator'],
        allocations, frag_list, current_alloc['total_size'], total_alloc_requests,
        failure_indices, elapsed_seconds, user_seconds, sys_seconds, operation_count
    )

def produce_graph():
    graphs_dir = "./graphs"
    benchmarks_dir = os.path.join(graphs_dir, "benchmarks")
    if not os.path.exists(benchmarks_dir):
        print(f"Benchmarks directory '{benchmarks_dir}' does not exist. Please create it and add .log files.")
        return
    for filename in os.listdir(benchmarks_dir):
        if filename.endswith('.log'):
            try:
                # Ask the user for each file
                answer = input(f"Include statistics box below the graph for '{filename}'? [y/N]: ").strip().lower()
                show_stats = (answer == 'y')

                filepath = os.path.join(benchmarks_dir, filename)
                allocator, allocations, frag_list, total_size, total_alloc_requests, failure_indices, elapsed_seconds, user_seconds, sys_seconds, operation_count = parse_log_file(filepath)

                # Calculate statistics
                highest_peak = max(allocations) if allocations else 0
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

                # Throughput calculation
                if elapsed_seconds and elapsed_seconds > 0:
                    throughput = operation_count / elapsed_seconds
                else:
                    throughput = 0

                # Build stats_text before plotting
                stats_text = (
                    f"Allocator: {allocator if allocator else 'Unknown'}\n"
                    f"Total operations: {operation_count}\n"
                    f"Total allocated: {format_bytes(total_alloc_requests)}\n"
                    f"Peak usage: {format_bytes(highest_peak)}\n"
                    f"Average fragmentation: {avg_frag:.2f} bytes\n"
                    f"Max fragmentation: {max_frag} bytes\n"
                    f"Elapsed seconds: {elapsed_seconds}\n"
                    f"User seconds: {user_seconds}\n"
                    f"Sys seconds: {sys_seconds}\n"
                    f"Throughput: {throughput:.2f} ops/sec"
                )

                if show_stats:
                    fig, (ax1, ax2) = plt.subplots(
                        nrows=2, ncols=1, 
                        figsize=(6.3, 4.5), 
                        gridspec_kw={'height_ratios': [4, 1]}
                    )
                else:
                    fig, ax1 = plt.subplots(
                        nrows=1, ncols=1, 
                        figsize=(6.3, 4.5)
                    )
                    ax2 = None

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

                ax1.legend(loc='upper right')

                # Add statistics as text in the lower subplot
                if show_stats and ax2 is not None:
                    ax2.axis('off')
                    ax2.text(
                        0, 1, stats_text,
                        fontsize=9, family='monospace',
                        verticalalignment='top', horizontalalignment='left'
                    )

                plt.tight_layout(rect=[0, 0, 1, 1])  # Use full space
                
                outname = filename.replace('.log', '.png')
                outpath = join(graphs_dir, outname)
                plt.savefig(outpath, dpi=150, bbox_inches='tight')
                plt.close()
                
                # Print timing and throughput info
                print(f"File: {filename}")
                print(f"Elapsed seconds: {elapsed_seconds}")
                print(f"User seconds: {user_seconds}")
                print(f"Sys seconds: {sys_seconds}")
                print(f"Total operations: {operation_count}")
                print(f"Throughput: {throughput:.2f} operations/sec")

            except Exception as e:
                print(f"Error processing {filename}: {str(e)}")

if __name__ == "__main__":
    produce_graph()
    print("All graphs generated successfully.")