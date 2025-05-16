void lockAllocator() {
        while (lock.test_and_set(memory_order_acquire));
    }

    void unlockAllocator() {
        lock.clear(memory_order_release);
    }

    MemoryBlock* findBlock(size_t size) {
        MemoryBlock* current = freeList;
        MemoryBlock* selected = nullptr;

        while (current) {
            __builtin_prefetch(current->next, 0, 3);
            if (current->free && current->size >= size) {
                if (strategy == FIRST_FIT) return current;

                if (!selected) selected = current;
                else if (strategy == BEST_FIT && current->size < selected->size) selected = current;
                else if (strategy == WORST_FIT && current->size > selected->size) selected = current;
            }
            current = current->next;
        }
        return selected;
    }
