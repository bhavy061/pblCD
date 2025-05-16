void* memoryPool;
MemoryBlock* freeList;
size_t poolSize;
Strategy strategy;
std::atomic_flag lock = ATOMIC_FLAG_INIT;

void coalesce() {
    MemoryBlock* current = freeList;
    while (current && current->next) {
    if (current->free && current->next->free) {
     current->size += sizeof(MemoryBlock) + current->next->size;
    current->next = current->next->next;
    std::cout << " Coalesced adjacent free blocks.\n";
    }
    else {
           current = current->next;
         }
        }
    }
