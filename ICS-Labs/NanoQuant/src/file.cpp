/* Files: reading and writing at a byte offset, on POSIX systems and on
 * Windows; nq.h describes the functions.
 *
 * Windows has no pread or pwrite, so there a read or write at an offset is a
 * seek followed by a read or write. The seek moves the descriptor's position,
 * which is harmless here: the programs are single threaded and give every
 * read and write its offset. */
#define _FILE_OFFSET_BITS 64    /* 64-bit off_t on 32-bit Linux as well */

#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif

#include "nq.h"

namespace {

/* Windows counts bytes in an unsigned int, and macOS refuses more than
   INT_MAX bytes in one call, so a long read or write is split. */
const size_t MAX_IO = (size_t)1 << 30;

#ifdef _WIN32
int64_t read_once(int fd, void *buf, size_t n, uint64_t off) {
    if (_lseeki64(fd, (long long)off, SEEK_SET) < 0) return -1;
    return _read(fd, buf, (unsigned)n);
}
int64_t write_once(int fd, const void *buf, size_t n, uint64_t off) {
    if (_lseeki64(fd, (long long)off, SEEK_SET) < 0) return -1;
    return _write(fd, buf, (unsigned)n);
}
#else
int64_t read_once(int fd, void *buf, size_t n, uint64_t off) {
    return pread(fd, buf, n, (off_t)off);
}
int64_t write_once(int fd, const void *buf, size_t n, uint64_t off) {
    return pwrite(fd, buf, n, (off_t)off);
}
#endif

} /* namespace */

#ifdef _WIN32
int nq_open_read(const char *path) { return _open(path, _O_RDONLY | _O_BINARY); }
int nq_open_write(const char *path) {
    return _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
}
int64_t nq_file_size(int fd) { return _lseeki64(fd, 0, SEEK_END); }
int  nq_close(int fd) { return _close(fd); }
bool nq_is_terminal(int fd) { return _isatty(fd) != 0; }
void nq_stdout_binary(void) { fflush(stdout); _setmode(_fileno(stdout), _O_BINARY); }
#else
int nq_open_read(const char *path) { return open(path, O_RDONLY); }
int nq_open_write(const char *path) { return open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); }
int64_t nq_file_size(int fd) { return (int64_t)lseek(fd, 0, SEEK_END); }
int  nq_close(int fd) { return close(fd); }
bool nq_is_terminal(int fd) { return isatty(fd) != 0; }
void nq_stdout_binary(void) {}
#endif

int nq_read_at(int fd, void *buf, size_t n, uint64_t off) {
    uint8_t *p = (uint8_t *)buf;
    size_t done = 0;
    while (done < n) {
        size_t want = n - done < MAX_IO ? n - done : MAX_IO;
        int64_t got = read_once(fd, p + done, want, off + done);
        if (got <= 0) return -1;        /* an error, or the file ends early */
        done += (size_t)got;
    }
    return 0;
}

int nq_write_at(int fd, const void *buf, size_t n, uint64_t off) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t done = 0;
    while (done < n) {
        size_t want = n - done < MAX_IO ? n - done : MAX_IO;
        int64_t put = write_once(fd, p + done, want, off + done);
        if (put <= 0) return -1;
        done += (size_t)put;
    }
    return 0;
}
