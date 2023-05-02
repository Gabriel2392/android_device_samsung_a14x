#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <libgen.h>

const size_t CHUNK_SIZE = 2 * 1024 * 1024;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(" - Usage: %s filename\n", argv[0]);
        return 2;
    }

    const char* filename = argv[1];
    char *base = basename(argv[1]);

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, " ! %s: Failed to open file %s\n", argv[0], filename);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, " ! %s: Failed to get file size\n", argv[0]);
        return 1;
    }

    if (lseek(fd, -1L, SEEK_END) == -1) {
        printf(" ! %s: Failed to seek to end of file\n", argv[0]);
        close(fd);
        return 1;
    }

    char last_byte;
    if (read(fd, &last_byte, sizeof(char)) != 1) {
        printf(" ! %s: Failed to read last byte of file\n", argv[0]);
        close(fd);
        return 1;
    }

    if (last_byte != '\0') {
        printf(" - %s: %s is already shrunk\n", argv[0], base);
        close(fd);
        return 0;
    }

    printf(" - %s: Shrinking %s\n", argv[0], base);
    off_t file_size = st.st_size;
    off_t last_non_zero_byte = file_size - 1;
    char buffer[CHUNK_SIZE];

    for (off_t pos = file_size - CHUNK_SIZE; pos >= 0; pos -= CHUNK_SIZE) {
        ssize_t n = pread(fd, buffer, CHUNK_SIZE, pos);
        if (n == -1) {
            fprintf(stderr, " ! %s: Failed to read file\n", argv[0]);
            return 1;
        }

        for (off_t i = n - 1; i >= 0; --i) {
            if (buffer[i] != '\0') {
                last_non_zero_byte = pos + i;
                goto done;
            }
        }
    }

    done:
    if (ftruncate(fd, last_non_zero_byte + 1) == -1) {
        fprintf(stderr, " ! %s: Failed to shrink %s\n", argv[0], base);
        return 1;
    }

    printf(" - %s: %s Shrunk to %lld bytes\n", argv[0], base, (long long)(last_non_zero_byte + 1));

    close(fd);
    return 0;
}
