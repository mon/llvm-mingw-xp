// Exercises libcxx/src/support/win32/support.cpp wcsnrtombs on XP.
// The XP patch swaps the MSVC secure-CRT wcrtomb_s for an inline lambda that
// wraps plain wcrtomb, because wcrtomb_s is not in msvcrt.dll. wcsnrtombs is
// reached via codecvt<wchar_t, char, mbstate_t>::do_out.

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <locale>
#include <string>

static int failures = 0;

#define EXPECT(cond)                                                                 \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

static void test_do_out_ascii() {
    std::locale l = std::locale::classic();
    const auto& cv = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(l);

    const wchar_t src[] = L"hello wide";
    constexpr size_t n = sizeof(src) / sizeof(src[0]) - 1;
    char dst[64] = {};
    std::mbstate_t st{};

    const wchar_t* from_next = nullptr;
    char* to_next = nullptr;
    auto r = cv.out(st, src, src + n, from_next, dst, dst + sizeof(dst), to_next);

    EXPECT(r == std::codecvt_base::ok);
    EXPECT(from_next == src + n);
    EXPECT(static_cast<size_t>(to_next - dst) == n);
    EXPECT(std::string(dst, to_next - dst) == "hello wide");
}

static void test_do_out_truncated() {
    std::locale l = std::locale::classic();
    const auto& cv = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(l);

    const wchar_t src[] = L"abcdefghij";
    constexpr size_t n = sizeof(src) / sizeof(src[0]) - 1;
    char dst[4] = {};
    std::mbstate_t st{};

    const wchar_t* from_next = nullptr;
    char* to_next = nullptr;
    auto r = cv.out(st, src, src + n, from_next, dst, dst + sizeof(dst), to_next);

    EXPECT(r == std::codecvt_base::partial);
    EXPECT(to_next == dst + sizeof(dst));
    EXPECT(std::string(dst, 4) == "abcd");
}

int main() {
    test_do_out_ascii();
    test_do_out_truncated();
    if (failures) {
        std::printf("codecvt: %d failures\n", failures);
        return 1;
    }
    std::printf("codecvt: OK\n");
    return 0;
}
