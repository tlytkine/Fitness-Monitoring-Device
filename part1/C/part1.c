// Libraries needed to make code work properly 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
// Specifies what to return / print upon an error 
#define ERROR(x) \
    do { \
        perror(x); \
        ret = -1; \
        goto done; \
    } while (0)

int init_tty(int fd); // sets the baud rate / configures serial port settings
int main_loop(int fd); // contains program with prompt to send commands from terminal to arduino
int send_cmd(int fd, char *cmd, size_t len); // method to send / receive commands and responses from terminal / arduino 

int
main(int argc, char **argv) {
   int fd;
    char *device; 
    int ret;


    // Connection established to port on /dev/tty.usbmodem1411
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

    char *buffer; // buffer which input string is read into 
    size_t bytes_in; // number of bytes read 
    size_t buffer_size = 128; // size of buffer made to 128 to assure functionality when entering multiple commands 
    // size_t characters; 
    buffer = (char *)malloc(buffer_size * sizeof(char)); // memory allocated for buffer 
    if(buffer == NULL){
        perror("Unable to allocate buffer");
        exit(1);
    }
        // Set of strings that specify valid commands that can be entered
        // These strings are compared to the buffer input 
        // The labeled numbers correspond to the options labeled within
        // the program loop.
        char *showX = "showX\n"; // 1
        char *pause = "pause\n"; // 2 
        char *resume = "resume\n"; // 3 
        char *exit = "exit\n"; // 4 
        char *help = "help\n"; // 5 
        // Set of strings which are sent to the arduino 
        // These strings are compared in the arduino code 
        // and the corresponding function is performed. 
        char *command1 = "shX\r";
        char *command2 = "PAU\r";
        char *command3 = "RES\r";

        // variable used in order to keep program running in a loop
        int running = 1;
        // return value 
        int ret;
        // program loop 
        while(running!=0){
            // *  - Prompt user for input
            printf("Please input one of the following options\n");
            printf("showX\n"); // 1 
            printf("pause\n"); // 2 
            printf("resume\n"); // 3 
            printf("exit\n"); // 4 
            printf("help\n"); // 5 
            printf("\n");
            // gets number of bytes in and puts them into the buffer
            bytes_in = getline(&buffer,&buffer_size,stdin);
            printf("\n");
            // prints number of characters read 
            printf("%zu characters were read.\n",bytes_in);
            // prints command that was typed 
            printf("You typed: %s\n",buffer);

            

             int n; // number of bytes sent to arduino (greater than 0 if successful)
             // Descriptions of what each command should do shown

            /* showX: : Set the output device to show X as the current heart rate 
            instead of the current real-time value.
            This will be useful while debugging.
            */
            /* After string comparison is performed, in each of the if statements below,
            the size of the string which was read in is used in order to send
            the proper number of bytes to the arduino.

            ret is set equal to either 0 or 1. 
            0 means that a valid number of bytes was read in 
            1 means that nothing was read in  */
            if(strcmp(buffer,showX)==0){
                size_t c1 = strlen(command1);
                n = send_cmd(fd,command1,c1);
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
                size_t c2 = strlen(command2);
                n = send_cmd(fd,command2,c2);
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
                size_t c3 = strlen(command3);
                n = send_cmd(fd,command3,c3);
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
                running = 0; // exits loop upon user entering exit 
            }
      
    }
        return ret;
}
// function that sends command 
int
send_cmd(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino 
    char buf[5]; // buffer to store response from arduino 
    // this if statement sends the command to the arduino and 
    // returns an error upon failure 
    if (write(fd, cmd, len) == -1) {
        perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(1);
    // response read in, number of bytes read set equal to count
    count = read(fd, &buf, 5);
    // Error if read fails or no response is received 
    if (count == -1) {
        perror("serial-read");
        return -1;
    } else if (count == 0) {
        fprintf(stderr, "No data returned\n");
        return -1;
    }

    // Responses aka the BPM, Signal, IBI needs to be stored
    printf("Response: %s\n", buf);
    return count;
}

int
init_tty(int fd) {
    struct termios tty;
    

    // * Configure the serial port.
    // * First, get a reference to options for the tty
    cfsetospeed(&tty, (speed_t)B9600);

    
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
