// Exercises libcxx/src/filesystem/posix_compat.h on XP.
//
// The XP patch rewrites stat_handle, remove, and realpath, and stubs
// symlink_file_dir and fchmod_handle (XP has neither CreateSymbolicLinkW nor
// SetFileInformationByHandle(FileBasicInfo)).
//
// Tests under RESTORED_SYMLINKS are kept in source but disabled — if/when XP
// ever gains symlink support, define RESTORED_SYMLINKS to enable them.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <windows.h>

namespace fs = std::filesystem;

static int failures = 0;

#define EXPECT(cond)                                                                 \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

static fs::path make_scratch_dir() {
    static int counter = 0;
    DWORD pid = GetCurrentProcessId();
    unsigned salt = static_cast<unsigned>(std::rand());
    for (int attempt = 0; attempt < 32; ++attempt) {
        fs::path p = fs::temp_directory_path() /
                     ("xp-fs-test-" + std::to_string(pid) + "-" +
                      std::to_string(salt) + "-" + std::to_string(counter++));
        std::error_code ec;
        if (fs::create_directory(p, ec))
            return p;
    }
    std::fprintf(stderr, "make_scratch_dir: unable to create a unique dir\n");
    std::abort();
}

static void write_file(const fs::path& p, const std::string& content) {
    std::ofstream f(p, std::ios::binary);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
}

// stat_handle: rewritten to use GetFileInformationByHandle +
// DeviceIoControl(FSCTL_GET_REPARSE_POINT) instead of
// GetFileInformationByHandleEx.
static void test_stat_and_size() {
    fs::path dir = make_scratch_dir();
    fs::path file = dir / "hello.txt";
    std::string payload = "hello from XP";
    write_file(file, payload);

    EXPECT(fs::exists(file));
    EXPECT(fs::is_regular_file(file));
    EXPECT(!fs::is_directory(file));
    EXPECT(fs::file_size(file) == payload.size());
    EXPECT(fs::is_directory(dir));
    EXPECT(!fs::is_regular_file(dir));

    auto t = fs::last_write_time(file);
    (void)t;

    fs::remove(file);
    fs::remove(dir);
}

static void test_directory_iterator() {
    fs::path dir = make_scratch_dir();
    for (int i = 0; i < 3; ++i)
        write_file(dir / ("f" + std::to_string(i) + ".txt"), "x");

    int count = 0;
    for (auto& e : fs::directory_iterator(dir)) {
        (void)e;
        ++count;
    }
    EXPECT(count == 3);

    for (auto& e : fs::directory_iterator(dir))
        fs::remove(e.path());
    fs::remove(dir);
}

// fs::remove on a regular file.
static void test_remove_file() {
    fs::path dir = make_scratch_dir();
    fs::path file = dir / "gone.txt";
    write_file(file, "bye");
    EXPECT(fs::exists(file));
    EXPECT(fs::remove(file));
    EXPECT(!fs::exists(file));
    fs::remove(dir);
}

// fs::remove on an empty directory: dispatched to RemoveDirectoryW by the
// XP-patched remove().
static void test_remove_directory() {
    fs::path dir = make_scratch_dir();
    fs::path subdir = dir / "empty_subdir";
    fs::create_directory(subdir);

    EXPECT(fs::remove(subdir));
    EXPECT(!fs::exists(subdir));

    fs::remove(dir);
}

// fs::remove_all needs fs::remove to handle directories.
static void test_remove_all() {
    fs::path dir = make_scratch_dir();
    fs::path subdir = dir / "nested";
    fs::create_directory(subdir);
    write_file(subdir / "a.txt", "a");
    write_file(subdir / "b.txt", "b");

    std::uintmax_t n = fs::remove_all(subdir);
    EXPECT(n == 3);
    EXPECT(!fs::exists(subdir));

    fs::remove(dir);
}

static void test_rename() {
    fs::path dir = make_scratch_dir();
    fs::path a = dir / "a.txt";
    fs::path b = dir / "b.txt";
    write_file(a, "renamed");
    fs::rename(a, b);
    EXPECT(!fs::exists(a));
    EXPECT(fs::exists(b));
    EXPECT(fs::file_size(b) == 7);
    fs::remove(b);
    fs::remove(dir);
}

static void test_current_path() {
    fs::path cwd = fs::current_path();
    EXPECT(!cwd.empty());
    EXPECT(fs::exists(cwd));
}

// realpath: rewritten to use GetFullPathNameW + an existence check, since
// GetFinalPathNameByHandleW is Vista+. fs::canonical funnels through it.
static void test_canonical() {
    fs::path dir = make_scratch_dir();
    fs::path file = dir / "real.txt";
    write_file(file, "r");

    fs::path c = fs::canonical(file);
    EXPECT(!c.empty());
    EXPECT(fs::equivalent(c, file));

    // canonical() must refuse to resolve a non-existent path.
    std::error_code ec;
    fs::path missing = fs::canonical(dir / "does-not-exist.txt", ec);
    EXPECT(ec);
    EXPECT(missing.empty());

    fs::remove(file);
    fs::remove(dir);
}

// symlink_file_dir is stubbed on XP — CreateSymbolicLinkW doesn't exist. The
// stub must call SetLastError so libcxx produces a real error_code; the link
// must not be created.
static void test_symlink_stub() {
    fs::path dir = make_scratch_dir();
    fs::path target = dir / "target.txt";
    fs::path link = dir / "link.txt";
    write_file(target, "t");

    std::error_code ec;
    fs::create_symlink(target, link, ec);
    EXPECT(ec);
    EXPECT(!fs::exists(link));

    fs::remove(target);
    fs::remove(dir);
}

#ifdef RESTORED_SYMLINKS
// symlink_file_dir creates a real symlink. Only reachable if XP ever gains
// symlink support.
static void test_create_symlink() {
    fs::path dir = make_scratch_dir();
    fs::path target = dir / "target.txt";
    fs::path link = dir / "link.txt";
    write_file(target, "content");

    fs::create_symlink(target, link);
    EXPECT(fs::exists(link));
    EXPECT(fs::is_symlink(fs::symlink_status(link)));
    EXPECT(fs::read_symlink(link) == target);

    fs::remove(link);
    fs::remove(target);
    fs::remove(dir);
}

// fchmod_handle is only reached via the symlink branch in fchmodat, so this
// also requires working symlinks.
static void test_permissions_symlink() {
    fs::path dir = make_scratch_dir();
    fs::path target = dir / "target.txt";
    fs::path link = dir / "link.txt";
    write_file(target, "c");
    fs::create_symlink(target, link);

    fs::permissions(link, fs::perms::owner_read,
                    fs::perm_options::replace | fs::perm_options::nofollow);
    DWORD attrs = GetFileAttributesW(link.wstring().c_str());
    EXPECT(attrs != INVALID_FILE_ATTRIBUTES);
    EXPECT(attrs & FILE_ATTRIBUTE_READONLY);

    fs::permissions(link, fs::perms::owner_all,
                    fs::perm_options::replace | fs::perm_options::nofollow);
    fs::remove(link);
    fs::remove(target);
    fs::remove(dir);
}
#endif

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    test_stat_and_size();
    test_directory_iterator();
    test_remove_file();
    test_remove_directory();
    test_remove_all();
    test_rename();
    test_current_path();
    test_canonical();
    test_symlink_stub();

#ifdef RESTORED_SYMLINKS
    test_create_symlink();
    test_permissions_symlink();
#endif

    if (failures) {
        std::printf("filesystem: %d failures\n", failures);
        return 1;
    }
    std::printf("filesystem: OK\n");
    return 0;
}
