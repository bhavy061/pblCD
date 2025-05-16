SmartAllocator(size_t size, Strategy strat = FIRST_FIT) : strategy(strat) {
        poolSize = size;
#ifdef _WIN32
        void* ptr = _aligned_malloc(poolSize, ALIGNMENT);
        if (!ptr) throw std::bad_alloc();
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, ALIGNMENT, poolSize) != 0) {
            throw std::bad_alloc();
        }
#endif
        memoryPool = ptr;
        freeList = static_cast<MemoryBlock*>(memoryPool);
        freeList->size = poolSize - sizeof(MemoryBlock);
        freeList->free = true;
        freeList->next = nullptr;
        std::cout << "SmartAllocator initialized with pool size: " << poolSize << " bytes\n";
    }

    void setStrategy(Strategy strat) {
        strategy = strat;
    }

    void* allocate(size_t size) {
        lockAllocator();
        std::cout << "\n Allocating " << size << " bytes...\n";
        MemoryBlock* block = findBlock(size);

        if (!block) {
            std::cout << " Allocation failed! No suitable block found.\n";
            unlockAllocator();
            return nullptr;
        }

        if (block->size > s