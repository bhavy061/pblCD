void testStrategy(SmartAllocator& allocator, const std::string& name) {
    cout << "\n=== Testing " << name << " ===\n";
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(allocator.allocate(100 + (i % 3) * 50));
    }
    for (int i = 0; i < ptrs.size(); i += 2) {
        allocator.deallocate(ptrs[i]);
    }
    allocator.printMemoryUsage();
    allocator.printMemoryMap();
}