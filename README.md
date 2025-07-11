# ripmalloc_so
Progetto finale per Sistemi Operativi, A.A. 2023/2024, tenuto dal professor Grisetti per il corso di laurea in Ingegneria Informatica e Automatica presso l'Università degli Studi Sapienza di Roma.

## Assignment: Pseudo Malloc
This is a malloc replacement. The system relies on mmap for the physical allocation of memory, but handles the requests in 2 ways:
- for small requests (< 1/4 of the page size) it uses a buddy allocator. Clearly, such a buddy allocator can manage at most page-size bytes. For simplicity use a single buddy allocator, implemented with a bitmap that manages 1 MB of memory for these small allocations.
- for large request (>=1/4 of the page size) uses a mmap.


## Implementation
```
Allocator (Interface)
  ├── SlabAllocator
  ├── BuddyAllocator
  │    ├── BitmapBuddyAllocator
  │    └── TreeBuddyAllocator (TODO?)
  └── LinearAllocator (TODO?) 

```

### Allocator
**Allocator** è un'interfaccia base (struttura in C) che definisce il contratto per i diversi tipi di allocatori di memoria. Viene estesa da implementazioni concrete. La struttura contiene quattro puntatori a funzione essenziali:
- init: Funzione di inizializzazione dell'allocatore
- dest: Funzione di distruzione/pulizia dell'allocatore
- malloc: Funzione per l'allocazione di memoria
- free: Funzione per la liberazione della memoria
Questa interfaccia permette di implementare diversi tipi di allocatori mantenendo un'API consistente.

### SlabAllocator
**SlabAllocator** è un'implementazione dell'interfaccia Allocator che gestisce la memoria utilizzando il metodo "slab allocation". Questo allocatore:
- Utilizza mmap per allocare la memoria fisica sottostante
- Gestisce la memoria in "slab" di dimensione fissa stabilita alla creazione
- Mantiene una lista di slab liberi utilizzando una struttura DoubleLinkedList
Include funzionalità per:
- Richiesta di utilizzo per uno slab libero (SlabAllocator_alloc)
- Liberazione di slab esistenti (SlabAllocator_release)
- Monitoraggio dell'utilizzo della memoria (SlabAllocator_print_state)
- Visualizzazione della memorya (SlabAllocator_print_memory_map)


### BuddyAllocator
**BuddyAllocator** è una classe astratta che implementa l'interfaccia Allocator e fornisce una primitiva per l'allocazione di blocchi di memoria più grandi di un quarto della page size. Le primitive di allocazione per blocchi più piccoli sono delegate alle classi figlie TreeBuddyAllocator e BitmapBuddyAllocator. 

#### BitmapBuddyAllocator
WIP

#### TreeBuddyAllocator
WIP
     
### LinearAllocator
WIP
