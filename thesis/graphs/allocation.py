import os
import matplotlib.pyplot as plt

def parse_alloc_file(filepath):
  allocations = []
  current_alloc = {}
  with open(filepath, 'r') as f:
    for line in f:
      line = line.strip()
      if not line or line.startswith('%'):
        continue
      parts = line.split(',')
      cmd = parts[0]
      if cmd == 'i':
        allocator = parts[1]
        current_alloc = {'allocator': allocator, 'params': {}, 'mem': {}, 'allocated': 0}
      elif cmd == 'p':
        if current_alloc['allocator'] == 'slab':
          current_alloc['params']['slab_size'] = int(parts[1])
          current_alloc['params']['num_slabs'] = int(parts[2])
        else:
          current_alloc['params']['memory_size'] = int(parts[1])
          current_alloc['params']['max_levels'] = int(parts[2])
      elif cmd == 'a':
        if len(parts) == 2:
          idx = int(parts[1])
          size = current_alloc['params'].get('slab_size', 1)
        else:
          idx = int(parts[1])
          size = int(parts[2])
        current_alloc['mem'][idx] = size
        current_alloc['allocated'] += size
        allocations.append(current_alloc['allocated'])
      elif cmd == 'f':
        idx = int(parts[1])
        size = current_alloc['mem'].pop(idx, 0)
        current_alloc['allocated'] -= size
        allocations.append(current_alloc['allocated'])
  return allocations

def produce_graph():
  benchmarks_dir = './benchmarks'
  for filename in os.listdir(benchmarks_dir):
    if filename.endswith('.alloc'):
      filepath = os.path.join(benchmarks_dir, filename)
      allocations = parse_alloc_file(filepath)
      plt.figure()
      plt.plot(range(len(allocations)), allocations, marker='o')
      plt.title(f'Memory Allocated per Instruction (bytes)\n{filename}')
      plt.xlabel('Instruction')
      plt.ylabel('Memory Allocated')
      plt.grid(True)
      plt.tight_layout()
      outname = filename.replace('.alloc', '.png')
      plt.savefig(os.path.join(benchmarks_dir, outname))
      plt.close()

produce_graph()