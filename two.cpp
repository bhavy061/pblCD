void deallocate(void* ptr) {
        if (!ptr) return;
        lockAllocator();
        MemoryBlock* block = reinterpret_cast<MemoryBlock*>(ptr) - 1;
        block->free = true;
        cout << "\n Deallocated memory at address: " << ptr << "\n";
        coalesce();
        unlockAllocator();
    }
