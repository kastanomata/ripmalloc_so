# ripmalloc_so
Progetto finale per Sistemi Operativi, A.A. 2023/2024, tenuto dal professor Grisetti per il corso di laurea in Ingegneria Informatica e Automatica presso l'Università degli Studi Sapienza di Roma.

## Assignment: Pseudo Malloc
This is a malloc replacement. The system relies on mmap for the physical allocation of memory, but handles the requests in 2 ways:
- for small requests (< 1/4 of the page size) it uses a buddy allocator. Clearly, such a buddy allocator can manage at most page-size bytes. 
For simplicity use a single buddy allocator, implemented with a bitmap that manages 1 MB of memory for these small allocations.
- for large request (>=1/4 of the page size) uses a mmap.

## Implementation
**WIP**
### Buddy Allocator
**WIP**

### Slab Allocator
**WIP**
     
