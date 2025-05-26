# ripmalloc_so
Progetto finale per Sistemi Operativi, A.A. 2023/2024, tenuto dal professor Grisetti per il corso di laurea in Ingegneria Informatica e Automatica presso l'Università degli Studi Sapienza di Roma.

## Assignment: Pseudo Malloc
This is a malloc replacement. The system relies on mmap for the physical allocation of memory, but handles the requests in 2 ways:
- for small requests (< 1/4 of the page size) it uses a buddy allocator. Clearly, such a buddy allocator can manage at most page-size bytes. For simplicity use a single buddy allocator, implemented with a bitmap that manages 1 MB of memory for these small allocations.
- for large request (>=1/4 of the page size) uses a mmap.

## Project Management
- [ ] Impostare e configurare un sistema di gestione della memoria tramite mmap per le allocazioni di grandi dimensioni
- [ ] Implementare un buddy allocator, utilizzando una bitmap per gestire un'area di memoria di 1 MB dedicata alle piccole allocazioni
- [ ]  Gestire le richieste di memoria uguali o superiori a un quarto della pagina tramite mmap
- [ ]  Gestire le richieste di memoria inferiori a un quarto della dimensione di pagina tramite il buddy allocator
- [ ]  Testare e verificare il corretto funzionamento delle allocazioni per richieste di diverse dimensioni

## Implementation
```
Allocator (Interface)
  ├── SlabAllocator
  │
  ├── LinearAllocator (?)
  │
  └── BuddyAllocator (Abstract Class)
      ├── TreeBuddyAllocator
      │
      └── BitmapBuddyAllocator

```

### Allocator
TODO 

### SlabAllocator
TODO

### LinearAllocator
WIP

### BuddyAllocator
WIP

#### TreeBuddyAllocator
WIP

#### BitmapBuddyAllocator
WIP
     
