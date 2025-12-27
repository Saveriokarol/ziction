#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 4096

// flags that gonna be parsed using 1st parameter
int creating = 0;
int deconcat = 0;

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffering

    if (argc < 4) {
        fprintf(stderr,
                "usage: %s c output.zict file1 [file2 ...]\n"
                "       %s d input.zict offset-index\n",
                argv[0], argv[0]);
        return 1;
    }

    for (int i = 0; i < strlen(argv[1]); i++) {
        switch (argv[1][i]) {
        case 'c':
            if (deconcat) {
                fprintf(stderr,
                        "you can't deconcatenate and create at once!\n");
                return 1;
            }
            creating = 1;
            break;

        case 'd':
            if (creating) {
                fprintf(stderr,
                        "you can't deconcatenate and create at once!\n");
                return 1;
            }
            deconcat = 1;
            break;
        }
    }

    const char *out_file = argv[2];

    if (creating) {
        int fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("open output");
            return 1;
        }

        printf("building header...\n");

        uint32_t file_count = argc - 3;
        uint32_t vect[file_count]; // offsets, will fill later

        // Reserve space for header: file_count + vect[]
        uint32_t count_le = file_count;
        if (write(fd_out, &count_le, sizeof(uint32_t)) != sizeof(uint32_t)) {
            perror("write file_count");
            close(fd_out);
            return 1;
        }

        // write placeholder offsets
        uint32_t zero = 0;
        for (uint32_t i = 0; i < file_count; i++) {
            if (write(fd_out, &zero, sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("write offset");
                close(fd_out);
                return 1;
            }
        }

        printf("processing %d files...\n", file_count);

        off_t current_offset = 4 + 4 * file_count; // after header
        char buf[BUFSIZE];

        for (int i = 3; i < argc; i++) {
            printf("  "); // keep your elegant spacing

            const char *file = argv[i];

            printf("(%d/%d) %s", i - 2, file_count, file);

            int fd_in = open(file, O_RDONLY);
            if (fd_in < 0) {
                fprintf(stderr, " (open error: %s)\n", strerror(errno));
                vect[i - 3] =
                    current_offset; // skip file, still store current offset
                continue;
            }

            vect[i - 3] = current_offset;

            ssize_t brd;
            ssize_t total = 0;

            while ((brd = read(fd_in, buf, BUFSIZE)) > 0) {
                ssize_t bwr = write(fd_out, buf, brd);
                if (bwr != brd) {
                    perror("write error");
                    close(fd_in);
                    close(fd_out);
                    return 1;
                }
                total += brd;
                current_offset += brd;
            }

            if (brd < 0) {
                fprintf(stderr, " (read error: %s)\n", strerror(errno));
            }

            close(fd_in);

            printf("  [size: %zd bytes]\n", total);
        }

        // Go back and write the actual offsets
        if (lseek(fd_out, sizeof(uint32_t), SEEK_SET) < 0) {
            perror("lseek");
            close(fd_out);
            return 1;
        }

        for (uint32_t i = 0; i < file_count; i++) {
            if (write(fd_out, &vect[i], sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("write offset final");
                close(fd_out);
                return 1;
            }
        }

        close(fd_out);
        printf("\ndone concatenating %d files into %s\n", file_count, out_file);
    } else if (deconcat) {
        int fd_out = open(out_file, O_RDONLY);
        if (fd_out < 0) {
            perror("open input");
            return 1;
        }

        uint32_t indl;
        ssize_t brd;
        brd = read(fd_out, &indl, sizeof(uint32_t));

        if (brd < 0) {
            perror("read table length");
            close(fd_out);
            return 1;
        }

        uint32_t vect[indl];

        read(fd_out, vect, indl * sizeof(uint32_t));

        char* eon;
        errno = 0;

        unsigned long tmp = strtoul(argv[3], &eon, 10);

        if (errno != 0 || *eon != '\0' || tmp > UINT32_MAX) {
            fprintf(stderr, "invalid uint32_t: %s\n", argv[3]);
            close(fd_out);
            return 1;
        }

        uint32_t value = (uint32_t)tmp;

        if (value >= indl) {
            fprintf(stderr, "index out of bounds: %s\n", argv[3]);
            close(fd_out);
            return 1;
        }

        off_t start = vect[value];

        off_t eof = lseek(fd_out, 0, SEEK_END);

        off_t end = (value + 1 < indl) ? vect[value + 1] : eof;

        size_t len = end - start;

        lseek(fd_out, start, SEEK_SET);

        char buf[BUFSIZE];
        size_t remaining = len;

        while (remaining > 0) {
            ssize_t rd = read(
                fd_out, buf, remaining < sizeof(buf) ? remaining : sizeof(buf));
            if (rd <= 0)
                break;

            write(STDOUT_FILENO, buf, rd);
            remaining -= rd;
        }

        close(fd_out);
    }

    return 0;
}
