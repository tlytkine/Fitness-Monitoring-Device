
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
        device = "/dev/tty.usbmodem1421";
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

    	char *showX = "showX\n";
    	char *pause = "pause\n";
    	char *resume = "resume\n";
    	char *exit = "exit\n";
    	char *help = "help\n";

        char *command1 = "shX\r";
        char *command2 = "PAU\r";
        char *command3 = "RES\r";


        int running = 1;

        int ret;

        while(running!=0){
            // *  - Prompt user for input
        	printf("Please input one of the following options\n");
        	printf("showX\n"); // 1 
        	printf("pause\n"); // 2 
        	printf("resume\n"); // 3 
        	printf("exit\n"); // 4 
            printf("help\n"); // 5 
            printf("\n");

            bytes_in = getline(&buffer,&buffer_size,stdin);
            printf("\n");
            printf("%zu characters were read.\n",bytes_in);
            printf("You typed: %s\n",buffer);

            

    	     int n; // number of bytes sent to arduino (greater than 0 if successful)

            /* showX: : Set the output device to show X as the current heart rate 
            instead of the current real-time value.
    		This will be useful while debugging.
    		*/
            if(strcmp(buffer,showX)==0){
                n = send_cmd(fd,command1,4);
    		    if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
            }



            /*pause: Pause the output and keep the display device showing the 
            current reading */
            if(strcmp(buffer,pause)==0){
                n = send_cmd(fd,command2,4);
        		if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
            }

            /*resume: Show the real-time heart rate on the display device. 
            This should be the default mode of the system. */
            if(strcmp(buffer,resume)==0){
                n = send_cmd(fd,command3,4);
    		    if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
            }


            // Displays list of available commands 
            if(strcmp(buffer,help)==0){
                printf("Commands available: \n");
                printf("1. showX : Sets LCD to show X as current heart rate instead of the current real-time value. \n");
                printf("2. pause : Pauses the output and keeps the display device showing the current reading. \n");
                printf("3. resume : Shows the real-time heart rate on the display device. (Default mode of system) \n");
                printf("4. exit : Exits the host program \n");
                printf("5. help : Self explanatory \n");
                ret = 0;
            }

            // exit: Exits the host program. 
            if(strcmp(buffer,exit)==0){
                printf("Exiting program...\n");
                ret = 0;
                running = 0;
            }
    }
        return ret;
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
