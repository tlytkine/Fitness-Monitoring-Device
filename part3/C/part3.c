// Libraries needed to make code work properly
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h> 
#include <signal.h>

// Specifies what to return / print upon an error
#define ERROR(x) \
    do { \
        perror(x); \
        ret = -1; \
        goto done; \
    } while (0)


clock_t Pre, next; // to count time between operations 
int hist[96][5]; // histogram to store values
int histStore[480]; // 1D array to mmap
int buffer_size = 64;


int init_tty(int fd); // sets the baud rate / configures serial port settings
int parent_loop(int fd); // contains program with prompt to send commands from terminal to arduino
int child_loop(int fd);
int send_cmd(int fd, char *cmd, size_t len); // method to send / receive commands and responses from terminal / arduino
int send_cmd_env(int fd, char *cmd, size_t len);  // second method to send / receive commands for environment sensor 
int readline(int serial_fd, char *buf);  
int pid; //Forks the program.

// Global variables to store information from sensors 
int hourVar; // hour 
int minuteVar; // minute 
int secondVar; // second
int tempVar; // temperature 
int humidVar; // humidity 
const char *filepath = "/tmp/mmapped.txt"; // mmap file path 
int mapfd; // map file descriptor 
char *map; 
int BPM; // BPM value 

int pipeFd[2];
int pipeFd2[2];



// Writes values to map 
void mapWrite(){

    mapfd = open(filepath, O_RDWR | O_CREAT, (mode_t)0600);
    if (mapfd == -1){
        perror("Error opening file for writing\n");
    }
    size_t textsize = 481; // histogram size 
    if(lseek(mapfd,textsize-1,SEEK_SET)==-1)
    {
        close(mapfd);
        perror("Error calling lseek() to stretch the file.\n");
    }

    if (write(mapfd, "", 1) == -1){
        close(mapfd);
        perror("Error writing last byte of the file\n");                   
     }
     map = mmap(0, textsize, PROT_READ | PROT_WRITE, MAP_SHARED, mapfd, 0);
     if (map == MAP_FAILED)
     {
        close(mapfd);
        printf("Error mmapping the file\n");                      
    }
}

// Syncs map values to filepath 
void mapSync(){
        size_t textsize = 481;
        if (msync(map, textsize, MS_SYNC) == -1)
        {
            perror("Could not sync the file to disk");
        }       
}

// Unmaps file and closes map file descriptor 
void mapClose(){
/*struct stat fileInfo;
if (fstat(mapfd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }*/
if (munmap(map, sizeof(map)) == -1)
    {
        close(mapfd);
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }
    close(mapfd);

}

// Reads in data from map initially 
void mapRead(){
    // mmap read 
	// printf("map read\n");
    mapfd = open(filepath, O_RDWR | O_CREAT, (mode_t)0600);


    if (mapfd == -1)
    {
        perror("Error opening file for reading");
        return;
    }

    struct stat fileInfo;
 
    if (fstat(mapfd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }
    if (fileInfo.st_size == 0)
    {
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        return;
    }
	else{
		map = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, mapfd, 0);
		if (map == MAP_FAILED)
    	{
        	close(mapfd);
        	perror("Error mmapping the file");
        	exit(EXIT_FAILURE);
    	}
	}

   //  printf("File size is %ji\n", (intmax_t)fileInfo.st_size);
    
    /*
        for (off_t i = 0; i < fileInfo.st_size; i++)
    {
        printf("Found character %c at %ji\n", map[i], (intmax_t)i);
	
    }
    */
        // Don't forget to free the mmapped memory

}



int
main(int argc, char **argv) {

    Pre = clock(); // counts amount of seconds that have passed 
    int fd;
    char *device;
    int ret;
    //char *file;
    //int mapfile;


   // mapRead();
    mapWrite();
	// printf("finished map read\n");

    

    // Connection established to port on /dev/tty.usbmodem1411
    if (argc == 3) {
        device = argv[1];
        //file = argv[2];
        //mapfile = open(file, O_RDONLY);
    }
    else if(argc == 2){
        //device = "/dev/tty.usbmodem1411";
	//device = "/dev/ttyACM0";
	device = argv[1];
        //mapfile = open(file, O_RDONLY);
    }
    else {
        printf("Usage: ./part2.o [device] [filename]");
        exit(1);
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


    pipe(pipeFd);
    pipe(pipeFd2);

    


	if((pid = fork()) == -1)
    {
        perror("fork");
        exit(1);
        // printf("%d\n", pid);
    }
	if(pid> 0){
       // printf("Going to parent loop.\n");
       // printf("pid = %d \n",pid);
        ret = parent_loop(fd);

	}
	else {
        // printf("Going to child loop.\n");
        // printf("pid = %d \n",pid);
        ret = child_loop(fd);
	}

done:
    close(fd);
    return ret;
}

void printHist(int hist[96][5]){

    // BPM 
    int freq0 = 0; // 0 through 40
    int freq1 = 0; // 41 through 80
    int freq2 = 0; // 81 through 120
    int freq3 = 0; // 121 through 160
    int freq4 = 0; // above 160

    int hourStart = 0;
    int hourEnd = 0;
    char* minuteStart = "xx";
    char* minuteEnd = "xx";

    hourStart = hourVar;
    hourEnd = hourVar;

    int index = 0;

    if((0<=minuteVar)&&(minuteVar<15)){
        index = hourVar * 4;
        minuteStart = "00";
        minuteEnd = "15";

    }
    else if((15<=minuteVar)&&(minuteVar<30)){
        index = (hourVar*4)+1;
        minuteStart = "15";
        minuteEnd = "30";

    }
    else if((30<=minuteVar)&&(minuteVar<45)){
        index = (hourVar*4)+2;
        minuteStart = "30";
        minuteEnd = "45";

    }
    else if((45<=minuteVar)&&(minuteVar<60)){
        index = (hourVar*4)+3;
        minuteStart = "45";
        minuteEnd = "00";
        hourEnd = hourStart + 1;
    }

    freq0 = hist[index][0];
    freq1 = hist[index][1];
    freq2 = hist[index][2];
    freq3 = hist[index][3];
    freq4 = hist[index][4];


        printf("Histogram for Current Time Interval: ");
        printf("%d:%s-%d:%s\n",hourStart,minuteStart,hourEnd,minuteEnd);
        printf("BPM    0 through 40: ");
        while(freq0!=0){
            printf("X");
            freq0--;
        }
        printf("\n");
        printf("BPM   41 through 80: ");
        while(freq1!=0){
            printf("X");
            freq1--;
        }
        printf("\n");
        printf("BPM  81 through 120: ");
        while(freq2!=0){
            printf("X");
            freq2--;
        } 
        printf("\n");       
        printf("BPM 121 through 160: ");
        while(freq3!=0){
            printf("X");
            freq3--;
        }
        printf("\n");
        printf("BPM       above 160: ");
        while(freq4!=0){
            printf("X");
            freq4--;
        }
        printf("\n");


    /*

    // First index, intervals 
    // 0 - 3 12:00am - 1:00am
    // Example:
    // 0 is 12:00am - 12:15am
    // 1 is 12:15am - 12:30am
    // 2 is 12:30am - 12:45am
    // 3 is 12:45am - 1:00am
    // The rest follow the same pattern 
    // 4 - 7 1:00am - 2:00am
    // 8 - 11 2:00am - 3:00am
    // 12 - 15 3:00am - 4:00am
    // 16 - 19 4:00am - 5:00am
    // 20 - 23 5:00am - 6:00am
    // 24 - 27 6:00am - 7:00am
    // 28 - 31 7:00am - 8:00am
    // 32 - 35 8:00am - 9:00am
    // 36 - 39 9:00am - 10:00am
    // 40 - 43 10:00am - 11:00am
    // 44 - 47 11:00am - 12:00pm
    // 48 - 51  12:00am - 1:00am
    // 52 - 55 1:00am - 2:00am
    // 56 - 59 2:00am - 3:00am
    // 60 - 63 3:00am - 4:00am
    // 64 - 67 4:00am - 5:00am
    // 68 - 71 5:00am - 6:00am
    // 72 - 75 6:00am - 7:00am
    // 76 - 79 7:00am - 8:00am
    // 80 - 83 8:00am - 9:00am
    // 84 - 87 9:00am - 10:00am
    // 88 - 91 10:00am - 11:00am
    // 92 - 95 11:00am - 12:00pm
    
    Second Index      BPM Value          Outlier Warning?
    0                 0 through 40      <- This sends response to Arduino to send BPM low warning
    1                 41 through 80
    2                 81 through 120
    3                 121 through 160
    4                 above 160         <- This sends response to Arduino to send BPM high warning
    */

}

int
child_loop(int fd) {
	
    char *buffer; // buffer which input string is read into
   
    size_t buffer_size = 128; // size of buffer made to 128 to assure functionality when entering multiple commands
    // size_t characters;
    buffer = (char *)malloc(buffer_size * sizeof(char)); // memory allocated for buffer
    if(buffer == NULL){
        perror("Unable to allocate buffer");
        exit(1);
    }
 
        char *command4 = "WRT\r";

        // Warning commands

        // variable used in order to keep program running in a loop
        int running = 1;
        // return value
        // program loop
        char pipeBuf[5];

        while(running!=0){
            
             //int n; // number of bytes sent to arduino (greater than 0 if successful)


            // Requests heart rate value from Arduino every second 
            // (sends command 4 aka write command)
            size_t c4 = strlen(command4);
            write(pipeFd[1], "g", 1);
            read(pipeFd[0], pipeBuf, sizeof(pipeBuf) - 1);
            
          if(pipeBuf[0] == 'x'){
               write(pipeFd2[1], "s", 1);
              do{
                read(pipeFd[0], pipeBuf, sizeof(pipeBuf));
                }
               while(pipeBuf[0] != 'r'); 
            }
            send_cmd(fd,command4,c4);  	
   		}	
        return 0;
}

int
parent_loop(int fd) {
	// printf("parent\n");
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
        char *rate = "rate\n"; 
        char *env = "env\n";
        char *hist1 = "hist\n";
        char *histX = "histX\n";
        char *reset = "reset\n";
        char *exit = "exit\n"; 

        // Set of strings which are sent to the arduino
        // These strings are compared in the arduino code
        // and the corresponding function is performed.
        char *command2 = "PAU\r";
        char *command3 = "RES\r";
        char *command4 = "WRT\r";
        char *command5 = "ENV\r";
        char *command6 = "HST\r";
        


        // Warning commands

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
            printf("resume\n"); 
            printf("rate\n");
            printf("env\n");
            printf("hist\n");
            printf("histX\n");
            printf("reset\n");
            printf("exit\n"); 
            printf("\n");
            
            // gets number of bytes in and puts them into the buffer
            bytes_in = getline(&buffer,&buffer_size,stdin);
            
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
         
             // show X: Set the output device to show X as the current heart rate 
             // instead of the current real-time value.
             char pipeBuf[5];
            if(strcmp(buffer,showX)==0){


                char *command1 = "shX";
                char* newCommand = command1;

                printf("Enter X: ");
                // gets number of bytes in and puts them into the buffer
                bytes_in = getline(&buffer,&buffer_size,stdin);
                printf("\n");
                // prints number of characters read
                printf("%zu characters were read.\n",bytes_in);
                // prints command that was typed
                printf("You typed: %s\n",buffer);
                sprintf(buffer,"\r");
                sprintf(newCommand, "%s", buffer);


        
                printf("newCommand: %s\n",newCommand);


                size_t c1 = strlen(newCommand);
                write(pipeFd[1], "x", 1);
                // printf("wrote to pipe\n");
                read(pipeFd2[0], pipeBuf, 1);
                // printf("Read from pipe\n");
                // printf("%s\n", pipeBuf);
                while(pipeBuf[0]!='s'){}
                n = send_cmd(fd,newCommand,c1);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
                // print the value to the console.
                printf("BPM: %d\n",BPM);

                memset(buffer,0,buffer_size);
            }
            // char *command2 = "PAU\r";
            /*pause: Pause the output and keep the display device showing the
            current reading */
            else if(strcmp(buffer,pause)==0){
                size_t c2 = strlen(command2);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd(fd,command2,c2);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
            }

            // char *command3 = "RES\r";
            /*resume: Show the real-time heart rate on the display device.
            This should be the default mode of the system. */
            else if(strcmp(buffer,resume)==0){
                size_t c3 = strlen(command3);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd(fd,command3,c3);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }
            }

            // char *command4 = "WRT\r";
            // rate: Query the value of the heart rate sensor at the current time
            // and print it to the console. 
            else if(strcmp(buffer,rate)==0){
                size_t c4 = strlen(command4);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd(fd,command4,c4);
                write(pipeFd[1], "r", 1);

                if(n>0){
                    ret = 0;
                }
                else{
                    ret = 1;
                }
                printf("Current BPM: %d\n",BPM);
            }

            // char *command5 = "ENV\r";
            // env: Query the value of the environment sensor from the Arduino 
            // and print it to the console.
            else if(strcmp(buffer,env)==0){
                size_t c5 = strlen(command5);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd_env(fd,command5,c5);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else{
                    ret = 1;
                }

            }
            
            // hist: Print a representation of the current time block's heart rate 
            // histogram to the console.
            else if(strcmp(buffer,hist1)==0){
                size_t c6 = strlen(command6);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd(fd,command6,c6);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else{
                    ret = 1;
                }

                int val = 0;

                 int timeBlock = ((hourVar) * 20);

                if((0<=minuteVar)&&(minuteVar<15)){
                    val = 0;
                    timeBlock = timeBlock + val;

                }
                else if((15<=minuteVar)&&(minuteVar<30)){
                    val = 1;
                    timeBlock = timeBlock + (val*5);

                }
                else if((30<=minuteVar)&&(minuteVar<45)){
                    val = 2;
                    timeBlock = timeBlock + (val*5);

                }
                else if((45<=minuteVar)&&(minuteVar<60)){
                    val = 3;
                    timeBlock = timeBlock + (val*5);
                }
                printf("timeBlock: %d \n",timeBlock);
                

                int freq0 = (int) map[timeBlock];
                int freq1 = (int) map[timeBlock+1];
                int freq2 = (int) map[timeBlock+2];
                int freq3 = (int) map[timeBlock+3];
                int freq4 = (int) map[timeBlock+4];


                
                printf("Histogram for Given Time Block: ");
                printf("\n");
                printf("BPM    0 through 40: ");
                while(freq0!=0){
                    printf("X");
                    freq0--;
                }
                printf("\n");
                printf("BPM   41 through 80: ");
                while(freq1!=0){
                    printf("X");
                    freq1--;
                }
                printf("\n");
                printf("BPM  81 through 120: ");
                while(freq2!=0){
                    printf("X");
                    freq2--;
                } 
                printf("\n");       
                printf("BPM 121 through 160: ");
                while(freq3!=0){
                    printf("X");
                    freq3--;
                }
                printf("\n");
                printf("BPM       above 160: ");
                while(freq4!=0){
                    printf("X");
                    freq4--;
                }
                printf("\n");

            }   
            // histX: Print a representation of the given time block's heart rate histogram to the console
            else if(strcmp(buffer,histX)==0){
                printf("Enter Hour: ");
                // gets number of bytes in and puts them into the buffer
                bytes_in = getline(&buffer,&buffer_size,stdin);
                printf("\n");
                // prints number of characters read
                printf("%zu characters were read.\n",bytes_in);
                // prints command that was typed
                printf("You typed: %s\n",buffer);
                char* hourString = buffer;
                int hourVal = atoi(hourString);

                memset(buffer,0,buffer_size);
                printf("Enter Minute: ");
                // gets number of bytes in and puts them into the buffer
                bytes_in = getline(&buffer,&buffer_size,stdin);
                printf("\n");
                // prints number of characters read
                printf("%zu characters were read.\n",bytes_in);
                // prints command that was typed
                printf("You typed: %s\n",buffer);
                char *minuteString = buffer;
                int minuteVal = atoi(minuteString);

                memset(buffer,0,buffer_size);


                int val = 0;

                 int timeBlock = ((hourVal) * 20);

                if((0<=minuteVal)&&(minuteVal<15)){
                    val = 0;
                    timeBlock = timeBlock + val;

                }
                else if((15<=minuteVal)&&(minuteVal<30)){
                    val = 1;
                    timeBlock = timeBlock + (val*5);

                }
                else if((30<=minuteVal)&&(minuteVal<45)){
                    val = 2;
                    timeBlock = timeBlock + (val*5);

                }
                else if((45<=minuteVal)&&(minuteVal<60)){
                    val = 3;
                    timeBlock = timeBlock + (val*5);
                }
                printf("timeBlock: %d \n",timeBlock);
                

                int freq0 = (int) map[timeBlock];
                int freq1 = (int) map[timeBlock+1];
                int freq2 = (int) map[timeBlock+2];
                int freq3 = (int) map[timeBlock+3];
                int freq4 = (int) map[timeBlock+4];
                printf("freq0: %d\n",freq0);
                printf("freq1: %d\n",freq1);
                printf("freq2: %d\n",freq2);
                printf("freq3: %d\n",freq3);
                printf("freq4: %d\n",freq4);


                
                printf("Histogram for Given Time Block: ");
                printf("\n");
                printf("BPM    0 through 40: ");
                while(freq0!=0){
                    printf("X");
                    freq0--;
                }
                printf("\n");
                printf("BPM   41 through 80: ");
                while(freq1!=0){
                    printf("X");
                    freq1--;
                }
                printf("\n");
                printf("BPM  81 through 120: ");
                while(freq2!=0){
                    printf("X");
                    freq2--;
                } 
                printf("\n");       
                printf("BPM 121 through 160: ");
                while(freq3!=0){
                    printf("X");
                    freq3--;
                }
                printf("\n");
                printf("BPM       above 160: ");
                while(freq4!=0){
                    printf("X");
                    freq4--;
                }
                printf("\n");
                
            }
            // reset: Clear all data from the backing file 
            else if(strcmp(buffer,reset)==0){
                size_t mapSize = 480;
                memset(map, 0, mapSize);
                mapSync();
                printf("Data cleared!\n");


            }
            // exit: Exits the host program 
            else if(strcmp(buffer,exit)==0){
                printf("Exiting program...\n");
				mapClose();
				kill(pid, SIGKILL);
                ret = 0;
                running = 0; // exits loop upon user entering exit

            }

    }
        return ret;
}

// Function that sends command for heart rate sensor 
int
send_cmd(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino
    char BPMchar;
    char buf[64]; // buffer to store response from arduino
    // this if statement sends the command to the arduino and
    // returns an error upon failure
    if (write(fd, cmd, len) == -1) {
        perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(2);
    // response read in, number of bytes read set equal to count
    // count = readline(fd, buf);

    count = read(fd, buf, sizeof(buf));

    // Error if read fails or no response is received
    
    if (count == -1) {
        perror("serial-read");
        return -1;
    } 
    
    /*
    else if (count == 0) {
        fprintf(stderr, "No data returned\n");
        return -1;
    }
    */
    

    // Responses aka the BPM, Signal, IBI needs to be stored
    //  printf("Response: %s\n", buf);
    char *low;
    char *high;
    char hour;
    char minute;
    char second;



    if(buf[4] == '\n'){
        BPMchar = buf[0];
        low = "LOW\r";
        high = "HIG\r";
        hour = buf[1];
        minute = buf[2];
        second = buf[3];
        BPM = (int)BPMchar;
    }
    else {
        return -1;
    }



    hourVar = (int) hour;
    minuteVar = (int) minute;
    secondVar = (int) second;
    // printf("Time: %d:%d:%d \n",hourVar,minuteVar,secondVar);



    //printHist(hist);

    // 1st index is hour
    // if minutes are between 0 and 15 first index is equal to hour 
    // between 15 and 30 equal to hour + 1 
    // between 30 and 45 equal to hour + 2 
    // between 45 and 60 equal to hour + 3 


    int index = hourVar * 20;
    
    int val; 

    if((minuteVar >= 0)&&(minuteVar<15)){
        val = 0; 
        index = index + val;

    }
    else if((minuteVar>=15)&&(minuteVar<30)){
        val = 5;
        index = index + val;
    }
    else if((minuteVar>=30)&&(minuteVar<45)){
        val = 10;
        index = index + val;
    }
    else if((minuteVar>=45)&&(minuteVar<60)){
        val = 15;
        index = index + val;

    }

     


    // Hour is first interval
    if((0 >= BPM)&&(BPM <= 40)){
        //  2nd index is 0 
        map[index]++;
        // outlier reading (heart rate too low)
        // sends LOW\r to arduino to print warning to screen
        // char *low = "LOW\r";
        // send a command to arduino to print warning to screen 
        size_t lowCommand = strlen(low);
        send_cmd(fd,low,lowCommand);
    }
    else if((41 >= BPM)&&(BPM <= 80)){
        // 2nd index is 1 
        map[index+1]++;
    }
    else if((81 >= BPM)&&(BPM <= 120)){
        // 2nd index is 2 
        map[index+2]++;
    }
    else if((121 >= BPM)&&(BPM <= 160)){
        // 2nd index is 3 
        map[index+3]++;
    }
    else if(BPM >= 160){
        map[index+4]++;
        // outlier reading 
        // send a command to arduino to print warning to screen   
        // char *high = "HIG\r";
        size_t highCommand = strlen(high);
        send_cmd(fd,high,highCommand);  
        
    }

    mapSync();


    return count;
}



// Function that sends command for environment sensor 
int
send_cmd_env(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino
    char buf[64]; // buffer to store response from arduino
    // this if statement sends the command to the arduino and
    // returns an error upon failure
    if (write(fd, cmd, len) == -1) {
        printf("Serial write error.\n");
        // perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(1);
    // response read in, number of bytes read set equal to count
    
    count = read(fd, buf, sizeof(buf));
    // Error if read fails or no response is received
    if (count == -1) {
        perror("serial-read");
        return -1;
    } 
    /*
    else if (count == 0) {
        fprintf(stderr, "No data returned\n");
        return -1;
    }
    */
    float temperature;
    float humidity;
    char tmp = buf[0];
    temperature = (float) tmp;
    temperature = temperature * 1.8;
    temperature = temperature + 32;
    temperature = temperature - '0';
    humidity = (float) buf[1];
    if(humidity < 0){
        humidity = humidity * -1;
    }
    


    printf("Temperature in Farenheit: %.2f \n",temperature);
    printf("Humidity: %.2f Percent\n",humidity);
    
    
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


int readline(int serial_fd, char buf[buffer_size]){
    int pos, ret;

    
    // Zero out the buffer 
    memset(buf, 0, buffer_size);

    // Read in one character at a time until we get a newline 
    pos = 0;
    while(pos < (buffer_size - 1)){

        ret = read(serial_fd, buf + pos, 1);
        if(ret == -1){
            perror("Error reading.");
        }
        else if (ret > 0){
            if(buf[pos] == '\n') break;
            pos += ret;
        }
        else{
            printf("Not working.\n");
        }
    }

    // Make sure theres something in the buffer 
    // If not try again
    if(strlen(buf)==0){
        printf("Empty buffer.");
        return readline(serial_fd,buf);
    }
    return 0;

}




