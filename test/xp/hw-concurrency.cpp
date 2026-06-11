// Exercises libcxx/src/thread.cpp on XP.
// XP does not have GetActiveProcessorCount; the patch switches
// std::thread::hardware_concurrency() to GetSystemInfo().dwNumberOfProcessors.

#include <cstdio>
#include <thread>

int main() {
    unsigned n = std::thread::hardware_concurrency();
    std::printf("hardware_concurrency: %u\n", n);
    // On any XP-capable host (virtual or otherwise) we expect at least 1 CPU.
    // The pre-patch path would return 0 on XP because the symbol is missing,
    // but since we statically link libcxx that would actually be a link-time
    // failure rather than a runtime 0. Either way, >=1 is the correct answer.
    if (n < 1) {
        std::printf("hw-concurrency: FAIL (got %u)\n", n);
        return 1;
    }
    std::printf("hw-concurrency: OK\n");
    return 0;
}
