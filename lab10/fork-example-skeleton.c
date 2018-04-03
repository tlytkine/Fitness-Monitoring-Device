#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


void print_char(char* c, int num);

int main(){
    int status = 0;


    //TODO: fork here
    int pid = fork();
 
    if(pid==0){
    /*TODO check to see if this is the child process */
        //TODO 1. print "c" for child 100 times
	//     2. exit the children process

        print_char("c",100);
        exit(status);
    }
    else{
        //TODO 1. print "p" for parent 100 times
        print_char("p",100);
    }


    wait(&status);
    

    printf("\n");    

    return 0;
}

void print_char(char* c, int num){
    int i = 0;
    
    for(i = 0; i < num; i++){
        printf("%s", c);
	fflush(stdout);
    	usleep(100);
    }

}
