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
#include <sqlite3.h>
#include <math.h>


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
int send_cmd_date(int fd, char *cmd, size_t len);  // second method to send / receive commands for RTC 
int readline(int serial_fd, char *buf);  
static int callbacksd(void *data, int argc, char **argv, char **azColName);
void tostring(char [], int);
int dbConnect();
int dbClose();
static int callback(void *data, int argc, char **argv, char **azColName);
int send_cmd_hour_minute(int fd, char *cmd, size_t len); 
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
char hour;
char minute;

int pipeFd[2];
int pipeFd2[2];
float temperature;
int timeBlock;
char recTime[5];



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
int hourVar; // hour 
int minuteVar; // minute 

void setHourMinute(int hour, int minute){
    hourVar = hour;
    minuteVar = minute;
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
sqlite3* db; //pointer to our new db



int dbInit(){
    int retSQL;
	// Open the database with the name 'test.db'
	retSQL = sqlite3_open("test.db",&db);
	if (retSQL != SQLITE_OK){
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    else {
        printf("Database connection successful!\n");
    }

	char *sql = "CREATE TABLE IF NOT EXISTS Datapoint("\
				"BPM INT,"\
				"Temperature FLOAT(9,6),"\
				"timeblock INT,"\
				"recTime TIME);";
    int ret1;

    char *errMsg;

    ret1 = sqlite3_exec(db,sql,NULL,0,&errMsg);

    if(ret1 != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n",errMsg);
        sqlite3_free(errMsg);
    }


	return 0;
}
int dbConnect(){
    int retConnect;
    retConnect = sqlite3_open("test.db",&db);
    if (retConnect != SQLITE_OK){
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    return retConnect;
}
int dbClose(){
    int retClose;
    retClose = sqlite3_close(db);
    if(retClose != SQLITE_OK){
        printf("Database connection was not able to close.");
        return -1;
    }
    return retClose;

}

int
main(int argc, char **argv) {

    Pre = clock(); // counts amount of seconds that have passed 
    int fd;
    char *device;
    int ret;
	if(dbInit() < 0) exit(1);
    

    mapWrite();

    // Connection established to port on /dev/tty.usbmodem1411
 
    if(argc == 2){
        //device = "/dev/tty.usbmodem1411";
		//device = "/dev/ttyACM0";
		device = argv[1];
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





void insertData(int BPM, float temperature, int timeblock, char* recTime){

    int ret;
    char *insertErrMsg;

    char insertQuery[200];

    sprintf(insertQuery,"INSERT INTO Datapoint(BPM,Temperature,timeblock,recTime) VALUES(%d,%f,%d,'%s');",BPM,temperature,timeblock,recTime);
    ret = sqlite3_exec(db,insertQuery,NULL,0,&insertErrMsg);


    if(ret != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n",insertErrMsg);
        sqlite3_free(insertErrMsg);
    }

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
        char *date = "date\n";
        char *regressionX = "regressionX\n"; 
        char *statX = "statX\n"; 


        // Set of strings which are sent to the arduino
        // These strings are compared in the arduino code
        // and the corresponding function is performed.
        char *command1 = "shX";
        char *command2 = "PAU\r";
        char *command3 = "RES\r";
        char *command4 = "WRT\r";
        char *command5 = "ENV\r";
     //   char *command6 = "HST\r";
        char *command7 = "DTE\r"; // date command to send to arduino
        char* command8 = "HRM\r"; // hour minute command


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
            printf("date\n");
            printf("regressionX\n");
            printf("statX\n");
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

                int xVal;

                printf("Enter X: ");
                scanf("%d",&xVal);
                printf("\n");
                printf("You typed: %d\n",xVal);
                    
                char* xString = (char*) malloc(sizeof(int));
                tostring(xString,xVal);

                char newCommand[80];
                snprintf(newCommand,sizeof(newCommand),"%s%s",command1,xString);

                // printf("newCommand: %s\n",newCommand);


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
                size_t c8 = strlen(command8);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd_hour_minute(fd, command8, c8); 
                write(pipeFd[1], "r",1);
                if(n>0){
                    ret = 0;
                }
                else {
                    ret = 1;
                }

                int val = 0;

                if(minuteVar < 10){
                    printf("Time: %d:0%d\n",hourVar,minuteVar);
                }
                else {
                    printf("Time: %d:%d\n",hourVar,minuteVar);
                    
                }

                timeBlock = ((hourVar) * 20);

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
               
                

                int freq0 = (int) map[timeBlock];
                int freq1 = (int) map[timeBlock+1];
                int freq2 = (int) map[timeBlock+2];
                int freq3 = (int) map[timeBlock+3];
                int freq4 = (int) map[timeBlock+4];


                if(freq0<0){
                    freq0 = 0;
                }
                if(freq1<0){
                    freq1 = 0;
                }
                if(freq2<0){
                    freq2 = 0;
                }
                if(freq3<0){
                    freq3 = 0;
                }
                if(freq4<0){
                    freq4 = 0;
                }
    
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

                timeBlock = ((hourVal) * 20);

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

                

                int freq0 = (int) map[timeBlock];
                int freq1 = (int) map[timeBlock+1];
                int freq2 = (int) map[timeBlock+2];
                int freq3 = (int) map[timeBlock+3];
                int freq4 = (int) map[timeBlock+4];

                if(freq0<0){
                    freq0 = 0;
                }
                if(freq1<0){
                    freq1 = 0;
                }
                if(freq2<0){
                    freq2 = 0;
                }
                if(freq3<0){
                    freq3 = 0;
                }
                if(freq4<0){
                    freq4 = 0;
                }



                
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
	    // date: Show the current value of the real-time clock on the console
            else if(strcmp(buffer,date)==0){
                size_t c7 = strlen(command7);
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                n = send_cmd_date(fd,command7,c7);
                write(pipeFd[1], "r", 1);
                if(n>0){
                    ret = 0;
                }
                else{
                    ret = 1;
                }

            }

            // regression X: Calculate a linear regression between the 
            // environment and heart rate data for a given time block.
            // Print the regression and the appropriate coeffienct of 
            // determination (r^2). If  is not provided,
            // default to the current time block. 
            // LINEAR REGRESSION FUNCTION !!!!!!
            else if(strcmp(buffer,regressionX)==0){
                write(pipeFd[1], "x", 1);
                read(pipeFd2[0], pipeBuf, 1);
                while(pipeBuf[0]!='s'){}
                int retTb;
                int x = 0, i, j;
                int h = 0;
                int m = 0;
                printf("Enter hour: ");
                // gets number of bytes in and puts them into the buffer
                scanf("%d",&h);
                printf("\n");
                printf("Enter minute: ");
                scanf("%d",&m);
                printf("\n");
                printf("Hour: %d \n",h);
                printf("Minute: %d \n",m);
                int val2 = 0;

                // Calculate time block based on hour and minute entered 
                x = 0;
                x = (h * 20);

                if((0<=m)&&(m<15)){
                    val2 = 0;
                    x = x + val2;

                }
                else if((15<=m)&&(m<30)){
                    val2 = 1;
                    x = x + (val2*5);

                }
                else if((30<=m)&&(m<45)){
                    val2 = 2;
                    x = x + (val2*5);

                }
                else if((45<=m)&&(m<60)){
                    val2 = 3;
                    x = x + (val2*5);
                }

                printf("x:  %d\n",x);

                // Values to query in database 



                char sql[200];
                sprintf(sql, "SELECT BPM, Temperature FROM Datapoint WHERE timeblock = %d",x);

                printf("%s\n",sql);




                sqlite3_stmt *stmt;
                sqlite3_prepare_v2(db,sql,strlen(sql),&stmt,NULL);

              


                int NUM_COLS = 2;
                int NUM_ROWS = 1;
                i = 0;
                int **data_selected = calloc(200,sizeof(int)*NUM_COLS); // place holder for two integers 


		int numElements = 0;
		float x1 = 0, x2 = 0, y1 = 0, y2 = 0, xy = 0; //Variables hold (in order) sigma x, sigma x^2, sigma y, sigma y^2, sigma xy

		// x is the temperature, y is the BPM.

                while( (retTb = sqlite3_step(stmt) == SQLITE_ROW) ){
                    data_selected = realloc(data_selected, (NUM_ROWS*NUM_COLS)* sizeof(int));
                    data_selected[i] = (int*) malloc(NUM_COLS * sizeof(int));
		            float xytemp = 1;
                    for(j = 0; j < NUM_COLS; j++){
                        data_selected[i][j] = sqlite3_column_int(stmt, j);
                        if((j%2)==0){
                          printf("BPM: %d\n",data_selected[i][j]);
			    y1 += data_selected[i][j];
			    y2 += (data_selected[i][j] * data_selected[i][j]);
			    xytemp *= data_selected[i][j];
                        }
                        else if((j%2)==1){
                            printf("Temperature: %d\n",data_selected[i][j]);
			    x1 += data_selected[i][j];
			    x2 += (data_selected[i][j] * data_selected[i][j]);
			    xytemp *= data_selected[i][j];
                        }
                    }
		    xy += xytemp;
                    i++;
                    NUM_ROWS++;
		    numElements++;
                }

		float a;
		float b;
        printf("numElements: %d\n",numElements);
	    if(((numElements*x2) - (x1*x1))==0){
            printf("Linear Regression Function: x = %f\n", x1/numElements);
        }
        else{
		    a = ((y1*x2) - (x1*xy)) / ((numElements*x2) - (x1*x1));
		    b = ((numElements*xy) - (x1*y1))/((numElements*x2)-(x1*x1));
            printf("x1 %f\n", x1);
            printf("x2 %f\n", x2);
            printf("y1 %f\n", y1);
            printf("y2 %f\n", y2);
            printf("xy %f\n", xy);
            printf("a %f\n", b);
            printf("b %f\n", a);
    	   	printf("Linear Regression Function: y = %.4fx + %.4f\n", b, a);
        }
        write(pipeFd[1], "r", 1);





        }
            // stat X: Print the following statistics 
            // for a given time block. (for both heart rate and 
            // environment data): reading count, mean, median,
            // mode, standard deviation. If X is not provided default 
            // to the current time block. 
            else if(strcmp(buffer,statX)==0){

                int retTb;
                int h1 = 0;
                int m1 = 0;
                printf("Enter Hour: ");;
                scanf("%d",&h1);
                printf("\n");
                printf("Enter Minute: ");
                scanf("%d",&m1);
                printf("\n");

                printf("Hour: %d\n",h1);
                printf("Minute: %d\n",m1);

                int val3 = 0;
                int tbs = 0;

                // Calculate time block based on hour and minute entered 
                tbs = (h1 * 20);

                if((0<=m1)&&(m1<15)){
                    val3 = 0;
                    tbs = tbs + val3;

                }
                else if((15<=m1)&&(m1<30)){
                    val3 = 1;
                    tbs = tbs + (val3*5);

                }
                else if((30<=m1)&&(m1<45)){
                    val3 = 2;
                    tbs = tbs + (val3*5);

                }
                else if((45<=m1)&&(m1<60)){
                    val3 = 3;
                    tbs = tbs + (val3*5);
                }

                printf("timeBlock:  %d\n",tbs);



                char *errMsg = 0;

                
    
                const char* data = "\n";



                // Reading Count BPM
                char rcBPM[200];
                sprintf(rcBPM, "SELECT COUNT(*) AS Reading_Count_Of_BPM FROM Datapoint WHERE timeblock = %d;",tbs);
                retTb = sqlite3_exec(db, rcBPM, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }

   


                // Reading Count Temperature 
                char rcTemp[200];
                sprintf(rcTemp, "SELECT COUNT(*) AS Reading_Count_Of_Temperature FROM Datapoint WHERE timeblock = %d;",tbs);
                retTb = sqlite3_exec(db, rcTemp, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Mean BPM
                char meanBPM[200];
                sprintf(meanBPM, "SELECT AVG(BPM) AS Average_BPM FROM Datapoint WHERE timeblock = %d;",tbs);
                retTb = sqlite3_exec(db, meanBPM, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Mean Temperature 
                char meanTemp[200];
                sprintf(meanTemp, "SELECT AVG(Temperature) AS Average_Temperature FROM Datapoint WHERE timeblock = %d;",tbs);
                retTb = sqlite3_exec(db, meanTemp, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Mode of BPM
                char modeBPM[400];
                sprintf(modeBPM, "SELECT BPM AS Mode_Of_BPM, MAX((SELECT COUNT(BPM) FROM Datapoint AS Datapoint1 WHERE BPM = Datapoint2.BPM AND timeblock = %d GROUP BY BPM ORDER BY COUNT(*) DESC LIMIT 1)) AS Frequency FROM (SELECT DISTINCT BPM FROM Datapoint WHERE timeblock = %d) AS Datapoint2;",tbs,tbs);
                retTb = sqlite3_exec(db, modeBPM, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Mode of Temperature 
                char modeTemp[400];
                sprintf(modeTemp, "SELECT Temperature AS Mode_Of_Temperature, MAX((SELECT COUNT(Temperature) FROM Datapoint AS Datapoint1 WHERE Temperature = Datapoint2.Temperature AND timeblock = %d GROUP BY Temperature ORDER BY COUNT(*) DESC LIMIT 1)) AS Frequency FROM (SELECT DISTINCT Temperature FROM Datapoint WHERE timeblock = %d) AS Datapoint2;",tbs,tbs);
                retTb = sqlite3_exec(db, modeTemp, callback, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Standard Deviation^2 of BPM
                char sdBPM[400];
                sprintf(sdBPM, "SELECT AVG(((Datapoint.BPM - Average)*(Datapoint.BPM - Average))) AS VarianceBPM FROM Datapoint, (SELECT AVG(BPM) AS Average FROM Datapoint AS AverageValues WHERE timeblock = %d) WHERE timeblock = %d;",tbs,tbs);
                retTb = sqlite3_exec(db, sdBPM, callbacksd, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }


                // Standard Deviation^2 of Temperature
                char sdTemp[400];
                sprintf(sdTemp, "SELECT AVG(((Datapoint.Temperature - Average)*(Datapoint.Temperature - Average))) AS VarianceTemperature FROM Datapoint, (SELECT AVG(Temperature) AS Average FROM Datapoint AS AverageValues WHERE timeblock = %d) WHERE timeblock = %d;",tbs,tbs);
                retTb = sqlite3_exec(db, sdTemp, callbacksd, (void*)data, &errMsg);
                if( retTb != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", errMsg);
                    sqlite3_free(errMsg);
                }

                




            }

    }
        return ret;
}

static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s", (const char*)data);


   for(i = 0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
      }
   
   
   
   printf("\n");
   return 0;
}

static int callbacksd(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s", (const char*)data);

   char* vBPM = "VarianceBPM";
   char* p1;
   char* p2;
   char* vTemp = "VarianceTemperature";
   char* nullVar = "NULL";

   for(i = 0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
          if(strcmp(azColName[i],vBPM) && strcmp(argv[i],nullVar)){
            float VarianceBPM = strtof(argv[i],&p1);
            float sdBPM = sqrtf(VarianceBPM);
            printf("Standard Deviation of Temperature: %f\n",sdBPM);

          }
          if(strcmp(azColName[i],vTemp) && strcmp(argv[i],nullVar)){
            float VarianceTemperature = strtof(argv[i],&p2);
            float sdTemp = sqrtf(VarianceTemperature);
            printf("Standard Deviation of BPM: %f\n",sdTemp);
          }
      }
   
   
   
   printf("\n");
   return 0;
}

// Function that sends command for heart rate sensor 
int
send_cmd(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino
    char buf[64]; // buffer to store response from arduino
    // this if statement sends the command to the arduino and
    // returns an error upon failure
    tcflush(fd, TCIOFLUSH);
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
    char tmp;




    //printf("%s\n", buf);


    if(buf[6] == '\n'){
    // printf("%c\n", buf[0]+'0');
       // printf("%c\n", buf[1]+'0');
        //printf("%c\n", buf[2]+'0');
      //  printf("%d\n",(int) buf[3]);
       // printf("%d\n",(int) buf[4]);
       // printf("%c\n", buf[5]+'0');
        low = "LOW\r";
        high = "HIG\r";
        hour = buf[3];
        minute = buf[4];
        tmp = buf[5];
        BPM = buf[0]*100 + buf[1]*10 + buf[2];
    }
    else {
        return -1;
    }
	// printf("%d\n", BPM);


    hourVar = (int) hour;
    minuteVar = (int) minute;




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
    if(BPM <= 40){
	//printf("0 through 40\n");
        //  2nd index is 0 
        map[index]++;
        // outlier reading (heart rate too low)
        // sends LOW\r to arduino to print warning to screen
        // char *low = "LOW\r";
        // send a command to arduino to print warning to screen 
        size_t lowCommand = strlen(low);
        send_cmd(fd,low,lowCommand);
    }
    else if((BPM >= 41)&&(BPM <= 80)){
	//printf("41 through 80\n");
        // 2nd index is 1 
        map[index+1]++;
    }
    else if((81 <= BPM)&&(BPM <= 120)){
	//printf("81 through 120\n");
        // 2nd index is 2 
        map[index+2]++;
    }
    else if((121 <= BPM)&&(BPM <= 160)){
	//printf("121 through 160\n");
        // 2nd index is 3 
        map[index+3]++;
    }
    else if(BPM >= 160){
	//printf("above 160\n");
        map[index+4]++;
        // outlier reading 
        // send a command to arduino to print warning to screen   
        // char *high = "HIG\r";
        size_t highCommand = strlen(high);
        send_cmd(fd,high,highCommand);  
        
    }

    mapSync();


    if(hourVar < 10){
        recTime[1] = '0';
        recTime[0] = (char) hourVar + '0';
    }
    else if(hourVar>=10){
        char hourString[2];
        int hv = hourVar;
        tostring(hourString,hv);
        recTime[1] = hourString[1];
        recTime[0] = hourString[0];
    }
    else {
        printf("Error with hourVar.\n");
    }


    recTime[2] = ':';
    if(minuteVar < 10){
        recTime[3] = '0';
        recTime[4] = (char) minuteVar + '0';
    }
    else if(minuteVar>=10) {
        char minuteString[2];
        int mv = minuteVar;
        tostring(minuteString,mv);
        recTime[4] = minuteString[1];
        recTime[3] = minuteString[0];
    }
    else {
        printf("Error with minuteVar.\n");
    }

    

    int val1 = 0;

    timeBlock = ((hourVar) * 20);

    if((0<=minuteVar)&&(minuteVar<15)){
        val1 = 0;
        timeBlock = timeBlock + val1;
    }
    else if((15<=minuteVar)&&(minuteVar<30)){
        val1 = 1;
        timeBlock = timeBlock + (val1*5);

     }
    else if((30<=minuteVar)&&(minuteVar<45)){
         val1 = 2;
         timeBlock = timeBlock + (val1*5);
    }
    else if((45<=minuteVar)&&(minuteVar<60)){
         val1 = 3;
         timeBlock = timeBlock + (val1*5);
    }



    temperature = (float) tmp;
    temperature = temperature * 1.8;
    temperature = temperature + 32;

    

    dbConnect();
    insertData(BPM, temperature, timeBlock, recTime);
    dbClose();


    return count;
}




void tostring(char str[], int num)
{
    int i, rem, len = 0, n;
 
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++){
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}



// Function that sends command for environment sensor 
int
send_cmd_env(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino
    char buf[64]; // buffer to store response from arduino
    // this if statement sends the command to the arduino and
    // returns an error upon failure
    tcflush(fd, TCIOFLUSH);
    if (write(fd, cmd, len) == -1) {
        printf("Serial write error.\n");
        // perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(2);
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
    float humidity;
    char tmp = buf[0];
    temperature = (float) tmp;
    temperature = temperature * 1.8;
    temperature = temperature + 32;
    humidity = (float) buf[1];
    if(humidity < 0){
        humidity = humidity * -1;
    }
    


    printf("Temperature in Farenheit: %.2f \n",temperature);
    printf("Humidity: %.2f Percent\n",humidity);
    
    
    return count;
}

int
send_cmd_date(int fd, char *cmd, size_t len) {
    int count; // number of bytes received as a response from the arduino
    char buf[5]; // buffer to store response from arduino
    // this if statement sends the command to the arduino and
    // returns an error upon failure
    tcflush(fd, TCIOFLUSH);
    if (write(fd, cmd, len) == -1) {
        printf("Serial write error.\n");
        // perror("serial-write");
        return -1;
    }

    // Give the data time to transmit
    // Serial is slow...
    sleep(2);
    // response read in, number of bytes read set equal to count
    //memset(buf, 0, 64);
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
    char month;
    char day;
    char year;

    int x = 4;


    


    if(buf[x] == '\n'){
        day = buf[1];
        month = buf[2];
        year = buf[3];
	printf("Date: %d/%d/%d \n", buf[2], buf[1], buf[3]);
    }
    else {
        month = 'M';
        day = 'D';
        year = 'Y';
	printf("Date: %c/%c/%c \n",month,day,year);

    }
    


    
    
    
    return count;
}

int
send_cmd_hour_minute(int fd, char *cmd, size_t len) {
    int count; 
    char buf[5]; 
    tcflush(fd, TCIOFLUSH);
    if (write(fd, cmd, len) == -1) {
        printf("Serial write error.\n");
        return -1;
    }
    sleep(2);
    count = read(fd, buf, sizeof(buf));
    if (count == -1) {
        perror("serial-read");
        return -1;
    } 
    char hour;
    char minute;

    if(buf[2] == '\n'){
        hour = buf[0];
        minute = buf[1];
    }

    else {
        hour = 'H';
        minute = 'M';
    }


    hourVar = (int) hour;
    minuteVar = (int) minute;

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




