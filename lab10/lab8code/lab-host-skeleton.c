#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>

#define ERROR(x) \
    do { \
        perror(x); \
        ret = -1; \
        goto done; \
    } while (0)

int init_tty(int fd);
int main_loop(int fd);
int send_cmd(int fd, char *cmd, size_t len);

int
main(int argc, char **argv) {
    int fd;
    char *device;
    int ret;

    /*
     * Read the device path from input,
     * or default to /dev/ttyACM0
     */
    if (argc == 2) {
        device = argv[1];
    } else {
        device = "/dev/ttyACM0";
    }

    /*
     * Need the following flags to open:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial");
        return -1;
    }

    /* Configure settings on the serial port */
    if (init_tty(fd) == -1) {
        ERROR("init");
    }

    ret = main_loop(fd);

done:
    close(fd);
    return ret;
}

int
main_loop(int fd) {

    char *buffer;
    size_t bytes_in;
    size_t buffer_size = 64; // or 32???
    // size_t characters; 
    buffer = (char *)malloc(buffer_size * sizeof(char));
    if(buffer == NULL){
        perror("Unable to allocate buffer");
        exit(1);
    }
    char *input;
    char *on = "on";
    char *off = "off";
    char *help = "help";
    char *exit = "exit";
    int ret;

    // Serial.begin(9600);
    


    /* 

    MARCH 20TH LAB 
    HERE 
    */

        // *  - Prompt user for input
        printf("Input data.\n");
        printf("on - turns on the built in LED");
        bytes_in = getline(&buffer,&buffer_size,stdin);
        printf("%zu characters were read.\n",bytes_in);
        printf("You typed: '%s'\n",buffer);
        // on - turns on the built in LED
        // figure out arduino communication??  
        if(strcmp(input,on)==0){
            /* insert code to turn LED on */

        }
        // off - turns off the built in LED 
        if(strcmp(input,off)==0){
            /* insert code to turn LED off */

        }
        // help - show this list of commands 
        if(strcmp(input,help)==0){
            /* insert code to show list of commands */

        }
        // exit - quit the program 
        if(strcmp(input,exit)==0){
            /* insert code to quit program */

        }

        return(0);
}

int
send_cmd(int fd, char *cmd, size_t len) {
    int count;
    char buf[5];

    if (write(fd, cmd, len) == -1) {
        perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(1);

    count = read(fd, &buf, 5);
    if (count == -1) {
        perror("serial-read");
        return -1;
    } else if (count == 0) {
        fprintf(stderr, "No data returned\n");
        return -1;
    }

    printf("Response: %s\n", buf);
    return count;
}

int
init_tty(int fd) {
    struct termios tty;
    

    // * Configure the serial port.
    // * First, get a reference to options for the tty
    cfsetospeed(&tty, (speed_t)B9600);
   //  * Then, set the baud rate to 9600 (same as on Arduino)
    
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) == -1) {
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

    // 8 bits, no parity, no stop bits
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Set local mode and enable the receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Make raw
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    // Infinite timeout and return from read() with >1 byte available
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    // Update options for the port
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}
