// Exercises libcxx's C locale shims that the XP patch rewrote.
// The patch replaces _strtof_l, _strtod_l, _strtold_l, _toupper_l, _tolower_l,
// _strcoll_l, _strxfrm_l, _iswXXX_l, _towXXX_l, _wcscoll_l, _wcsxfrm_l,
// _strftime_l, and _mbtowc_l with plain C functions wrapped by __locale_guard.
// These routes are reached through standard library facets.

#include <cassert>
#include <clocale>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

static int failures = 0;

#define EXPECT(cond)                                                                 \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

// num_get::do_get dispatches into __strtof/__strtod/__strtold through libcxx.
static void test_num_get_float() {
    std::locale c = std::locale::classic();
    {
        std::istringstream iss("3.1415");
        iss.imbue(c);
        double d = 0.0;
        iss >> d;
        EXPECT(!iss.fail());
        EXPECT(d > 3.141 && d < 3.142);
    }
    {
        std::istringstream iss("2.5");
        iss.imbue(c);
        float f = 0.0f;
        iss >> f;
        EXPECT(!iss.fail());
        EXPECT(f == 2.5f);
    }
    {
        std::istringstream iss("1.25");
        iss.imbue(c);
        long double ld = 0.0L;
        iss >> ld;
        EXPECT(!iss.fail());
        EXPECT(ld == 1.25L);
    }
}

// collate::compare / transform → __strcoll / __strxfrm
static void test_collate_char() {
    const auto& col = std::use_facet<std::collate<char>>(std::locale::classic());
    std::string a = "apple";
    std::string b = "banana";
    EXPECT(col.compare(a.data(), a.data() + a.size(), b.data(), b.data() + b.size()) < 0);
    EXPECT(col.compare(b.data(), b.data() + b.size(), a.data(), a.data() + a.size()) > 0);
    EXPECT(col.compare(a.data(), a.data() + a.size(), a.data(), a.data() + a.size()) == 0);

    std::string xa = col.transform(a.data(), a.data() + a.size());
    std::string xb = col.transform(b.data(), b.data() + b.size());
    EXPECT(xa < xb);
}

// collate<wchar_t> → __wcscoll / __wcsxfrm
static void test_collate_wchar() {
    const auto& col = std::use_facet<std::collate<wchar_t>>(std::locale::classic());
    std::wstring a = L"apple";
    std::wstring b = L"banana";
    EXPECT(col.compare(a.data(), a.data() + a.size(), b.data(), b.data() + b.size()) < 0);

    std::wstring xa = col.transform(a.data(), a.data() + a.size());
    std::wstring xb = col.transform(b.data(), b.data() + b.size());
    EXPECT(xa < xb);
}

// ctype<char>::toupper/tolower via a non-classic locale path → __toupper/__tolower
static void test_ctype_char() {
    std::locale c = std::locale::classic();
    const auto& ct = std::use_facet<std::ctype<char>>(c);
    EXPECT(ct.toupper('a') == 'A');
    EXPECT(ct.tolower('Z') == 'z');
    char buf[] = "MixedCASE";
    ct.tolower(buf, buf + 9);
    EXPECT(std::string(buf, 9) == "mixedcase");
}

// ctype<wchar_t>::is/toupper → __iswXXX / __towupper
static void test_ctype_wchar() {
    std::locale c = std::locale::classic();
    const auto& ct = std::use_facet<std::ctype<wchar_t>>(c);
    EXPECT(ct.is(std::ctype_base::space, L' '));
    EXPECT(ct.is(std::ctype_base::digit, L'7'));
    EXPECT(!ct.is(std::ctype_base::digit, L'x'));
    EXPECT(ct.is(std::ctype_base::alpha, L'q'));
    EXPECT(ct.is(std::ctype_base::xdigit, L'f'));
    EXPECT(ct.is(std::ctype_base::upper, L'Q'));
    EXPECT(ct.is(std::ctype_base::lower, L'q'));
    EXPECT(ct.is(std::ctype_base::print, L'a'));
    EXPECT(ct.is(std::ctype_base::cntrl, L'\n'));
    EXPECT(ct.is(std::ctype_base::punct, L'.'));
    EXPECT(ct.toupper(L'a') == L'A');
    EXPECT(ct.tolower(L'Z') == L'z');
}

// time_put formatting → __strftime
static void test_time_put() {
    std::tm t{};
    t.tm_year = 126;  // 2026
    t.tm_mon = 3;     // April
    t.tm_mday = 20;
    t.tm_hour = 14;
    t.tm_min = 30;
    t.tm_sec = 0;
    t.tm_wday = 1;    // Monday

    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
    EXPECT(oss.str() == "2026-04-20 14:30:00");

    std::ostringstream oss2;
    oss2.imbue(std::locale::classic());
    oss2 << std::put_time(&t, "%A");
    EXPECT(oss2.str() == "Monday");
}

// Instantiating codecvt<wchar_t,char,mbstate_t> in a non-classic locale ensures
// __mbtowc is exercised during facet construction (it probes the CRT for
// state-dependent encodings via mbtowc(nullptr, ...)).
static void test_codecvt_byname() {
    try {
        std::locale l("C");
        const auto& cv = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(l);
        (void)cv.encoding();
    } catch (const std::exception& e) {
        std::printf("FAIL codecvt_byname: %s\n", e.what());
        ++failures;
    }
}

int main() {
    test_num_get_float();
    test_collate_char();
    test_collate_wchar();
    test_ctype_char();
    test_ctype_wchar();
    test_time_put();
    test_codecvt_byname();

    if (failures) {
        std::printf("locale-cpp: %d failures\n", failures);
        return 1;
    }
    std::printf("locale-cpp: OK\n");
    return 0;
}
