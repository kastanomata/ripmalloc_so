# How to Use the Memory Profiler

**Note:** Files must be in the `./benchmarks` folder and have the `.alloc` extension.  
Lines starting with `%`(percent) will be ignored. Comments should have their own line.

## Commands
- `i,<allocator type>`  
  Allocator type can be:  
  - `slab`
  - `buddy`
  - `bitmap`

- `p,<param1>,<param2>,...`  
  Parameters depend on the allocator type:

  - **For slab:**
    - `param1` = `slab_size`
    - `param2` = `num_slabs`
  - **For buddy and bitmap:**
    - `param1` = `memory_size`
    - `param2` = `max_levels`

> **Note:** Place these commands one after the other.

## Fixed Size Allocation
- `a,<index>`  
  Allocates memory at the given `index` in an internal array.

- `f,<index>`  
  Frees memory at the given `index`.

## Variable Size Allocation

- `a,<index>,<size>`  
  Allocates memory of `size` at the given `index`.

- `f,<index>`  
  Frees memory at the given `index`.

> **Warning:**  
> Do **not** allocate more than once at the same index without freeing first!  
> You will lose track of the pointer and be unable to find it.
