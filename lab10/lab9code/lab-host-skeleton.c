#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define ERROR(lbl, x) \
    do { \
        perror(x); \
        ret = -1; \
        goto lbl; \
    } while (0)
#define BUFFER_LEN 32

int init_tty(int serial_fd);
int main_loop(int serial_fd, FILE *output);
int readline(int serial_fd, char buf[BUFFER_LEN]);

int
main(int argc, char **argv) {
    int serial_fd;
    FILE *output;
    char *device;
    char *path;
    int ret;

    /*
     * Read the device path from input,
     * or default to /dev/ttyACM0
     */
    if (argc >= 2) {
        device = argv[1];
    } else {
        device = "/dev/tty.usbmodem1421";
    }

    /*
     * Read the destination file from input
     * or default to output.txt
     */
    if (argc >= 3) {
        path = argv[2];
    } else {
        path = "output.txt";
    }

    /*
     * Need the following flags to open the serial port:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */
    printf("Connecting to %s\n", device);
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) {
        perror("serial-open");
        return -1;
    }

    // TODO: Open output file for writing (use 'path' as filename)
    output = fopen(path, "w");

    /* Configure settings on the serial port */
    if (init_tty(serial_fd) == -1) {
        ERROR(done, "init");
    }

    ret = main_loop(serial_fd, output);

done:
    close(serial_fd);
    return ret;
}


int
main_loop(int serial_fd, FILE *output) {
    char buf[BUFFER_LEN];
    int16_t value;
    int bytes_available;
    int ret;
    int pos;

    /*
     * Skip the first line we read
     * This makes sure we're starting from a clean
     * point when buffering
     */
    readline(serial_fd, buf);

    /* TODO:
     * Main program loop:
     *  - Read line from the serial connection
     *  - Write data to file
     */
     int i;
     for(i = 0; i < BUFFER_LEN; i++){
       if(buf[i]!='\n'){
         fputc(buf[i], output);

       }
     }
     char x = ' ';
     fputc(x, output);


    return 0;

}

int
readline(int serial_fd, char buf[BUFFER_LEN]) {

        // Zero out the buffer
        memset(buf, 0, BUFFER_LEN);


        // TODO: Read until end of current line
        //       Store line in buf
        //  (HINT: How do we know where the line ends?)

        // Make sure there's something in the buffer
        // If not, try again
        int count;
        count = read(serial_fd, &buf, 5);
        // Error if read fails or no response is received
        if (count == -1) {
            perror("serial-read");
            return -1;
        } else if (count == 0) {
            fprintf(stderr, "No data returned\n");
            return -1;
        }

        if (strlen(buf) == 0) {
            return readline(serial_fd, buf);
        }

        return 0;
}

int
init_tty(int serial_fd) {
    struct termios tty;
    /*
     * Configure the serial port.
     * First, get a reference to options for the tty
     * Then, set the baud rate to 9600 (same as on Arduino)
     */
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(serial_fd, &tty) == -1) {
        perror("tcgetattr");
        return -1;
    }

    if (cfsetospeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetospeed");
        return -1;
    }
    if (cfsetispeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetispeed");
        return -1;
    }

    // Set local mode and enable the receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Update options for the port
    if (tcsetattr(serial_fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}
