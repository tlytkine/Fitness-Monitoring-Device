/*
 * From: https://gist.github.com/marcetcheverry/991042
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Need at least one argument to write to the file\n");
        exit(EXIT_FAILURE);
    }

    const char *text = argv[1];
    printf("Will write text '%s'\n", text);

    /* Open a file for writing.
     *  - Creating the file if it doesn't exist.
     *  - Truncating it to 0 size if it already exists. (not really needed)
     *
     * Note: "O_WRONLY" mode is not sufficient when mmaping.
     */

    FILE* fileName;

    fileName = fopen("newFile.txt","w");

    const char *filepath = "/Users/timothylytkine/Desktop/newFile.txt";

    int fd = open(filepath, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

    if (fd == -1)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    // TODO: Explain to a TA why the code below is necessary (from here to "End"):

    // Stretch the file size to the size of the (mmapped) array of char
    size_t textsize = strlen(text) + 1; // + \0 null character

    if (lseek(fd, textsize-1, SEEK_SET) == -1)
    {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
        exit(EXIT_FAILURE);
    }

    /* Something needs to be written at the end of the file to
     * have the file actually have the new size.
     * Just writing an empty string at the current file position will do.
     *
     * Note:
     *  - The current position in the file is at the end of the stretched
     *    file due to the call to lseek().
     *  - An empty string is actually a single '\0' character, so a zero-byte
     *    will be written at the last byte of the file.
     */

    if (write(fd, "", 1) == -1)
    {
        close(fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }

    // End

    // Now the file is ready to be mmapped.
    char *map = mmap(0, textsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    // TODO: Write the input text to the map
    for(size_t i = 0; i < textsize; i++)
    {	
    	map[i] = text[i];
    	if(i == textsize-1){
    		printf("Text written to file.\n");
    	}
    }





    // TODO: Save the changes we made to the mmap back to the file
        if(msync(map,textsize,MS_SYNC)==-1)
    {
    	perror("File not synced to disk.");
    }


    // TODO: Unmap the memory

    if(munmap(map, textsize)==-1)
    {
    	close(fd);
    	perror("Couldn't unmap memory");
    	exit(EXIT_FAILURE);
    }

    // Un-mmaping doesn't close the file, so we still need to do that.
    close(fd);

    return 0;
}
