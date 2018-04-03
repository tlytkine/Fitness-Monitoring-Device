/*
 * From: https://gist.github.com/marcetcheverry/991042
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, const char *argv[])
{


    const char *filepath = "/Users/timothylytkine/Desktop/newFile.txt";


    int fd = open(filepath, O_RDONLY, (mode_t)0600);

    if (fd == -1)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    struct stat fileInfo = {0};

    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }

    if (fileInfo.st_size == 0)
    {
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(EXIT_FAILURE);
    }

    printf("File size is %ji\n", (intmax_t)fileInfo.st_size);

    char *map = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    // TODO: Read all characters from the mmap and print to terminal
        for (off_t i = 0; i < fileInfo.st_size-1; i++)
    {
        printf("Character %c at %ji\n", map[i], (intmax_t)i);
    }

    // Don't forget to free the mmapped memory
    if (munmap(map, fileInfo.st_size) == -1)
    {
        close(fd);
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }

    // Un-mmaping doesn't close the file, so we still need to do that.
    close(fd);

    return 0;
}
