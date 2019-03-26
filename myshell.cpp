/* Jose lopez
*  CS3377 Assignment 3
*
*  The program is a simple linux shell which will allow up to 3 programs to be piped, allow multiple commands as long as they
*  are seperated by special symbol ';', run single commands, and allow for file redirection. The program is able to do this by
*  parsing the command and looking for special symbols. Once a special symbol is found, what the special symbol represents will
*  be stored, and their respective functiosn will be called.
*/




#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

using namespace std;

//Represents the largest size of a command that this program is able to support.
const int MAX_SIZE = 256;

//Enumeral type that represents each special symbol this program supports
enum PipeRedirectAppendMultiple {PIPE, REDIRECT, NONE, MULTIPLE, APPEND, INPUT, OUTPUT};

//Will parse the command and identifies special symbols. It will then return enum type so respective functions can be called 
PipeRedirectAppendMultiple CommandParse(char* command[], char* cmd1[], char* cmd2[], int sizeOfCommand);

//Function responsible for piping commands
void pipe_cmd(char** cmd1, char** cmd2);

//Function responsible for running several commands seperated with ' ; '
void MultipleCommand(char** cmd1, char** cmd2);

//Function responsible for redirection of a file.
void redirect_cmd(char** cmd, char** file);

//Main method, will first parse the complete argument into spaces and store them into a char pointer array. Once it does this
// it will run the parser that looks for special symbols and then run each respective function as necessary.
int main(int argc, char *argv[]){
	char *str = argv[1];						//Will store the complete argument
	char *fullCommand[MAX_SIZE];					//Will store the parsed command
	char *pch;							//Will temporarily store each part of command
	pch = strtok (str," ");						//Will start the parsing by space
	int sizeOfCommand = 0;						//Will store the amount of parts in the command
	//Will run until there is no more parts to the program, incrementing parts and storing them in full command
	while (pch != NULL){
		fullCommand[sizeOfCommand] = pch;
		pch = strtok (NULL, " ");
		sizeOfCommand ++;
	}
	
	char *cmd1[MAX_SIZE];						//Will store a complete command
	char *cmd2[MAX_SIZE];						//Will store a second command
	PipeRedirectAppendMultiple result = CommandParse(fullCommand, cmd1, cmd2, sizeOfCommand);
	
	//Several cases that run depending on what our speical symbol parser returns.
	if(result == PIPE){
		pipe_cmd(cmd1,cmd2);
	}
	else if(result == MULTIPLE){
		MultipleCommand(cmd1, cmd2);
	}
	else if(result == REDIRECT){
		redirect_cmd(cmd1,cmd2);
	}
	else if(result == INPUT){
		pipe_cmd(cmd1,cmd2);
	}
	else if(result == OUTPUT){
		pipe_cmd(cmd2,cmd1);
	}	
	else if(result == NONE){
		execvp(fullCommand[0],fullCommand);
		perror("execvp failed");
	}
	

}

//Special symbol parser. Will look for special symbols, and if they are found then more than one command found.
//Will store each command and return the meaning of the special symbol found.
PipeRedirectAppendMultiple CommandParse(char* command[], char* cmd1[], char* cmd2[], int sizeOfCommand){
  	PipeRedirectAppendMultiple result = NONE;			//Assume only one command
 	int split = -1;
	//Look through the whole command parts, if found set result to each respective meaning.
	for (int i=0; i<sizeOfCommand; i++) {
    		if (strcmp(command[i], "|") == 0) {
    			result = PIPE;
      			split = i;
   		} 
		else if (strcmp(command[i], ">>") == 0) {
      			result = REDIRECT;
     			split = i;
    		}
		else if(strcmp(command[i], ">") == 0) {
			result = INPUT;
			split = i;
		}
		else if(strcmp(command[i], "<") == 0) {
			result = OUTPUT;
			split = i;
		}
		
  	}
	//If special symbol found, then store each command into seperate arrays
  	if (result != NONE) {
    		for (int i=0; i<split; i++){
      			cmd1[i] = command[i];
		}
   		int count = 0;
    		for (int i=split+1; i<sizeOfCommand; i++) {
     			cmd2[count] = command[i];
      			count++;
   		}
		cmd1[split] = NULL;
   		cmd2[count] = NULL;
  	}
	//If no special symbol is found, then check for special symbol ';' since because it is not seperated by a space have to
	// loop through all letters in a command and see if its present. if it is, seperate each command. if not, then 
	// ony one command.
	else{
		int foundFirst = 0;					//Flag, if ; is found then have one complete command	
		int locationOne = 0;					//Will store the index where the first command ends
		int locationTwo = 0;					//Will store the index where second command ends
		int curEle = 0;						//Will store the index of the seperate command arrays
		//Will loop through each part of the command, and look for ; symbol
		for(int i = 0; i < sizeOfCommand; i++){
			int length = strlen(command[i]);		//Will hold the length of each part of command
			//If have not found ; symbol, keep storing into first command array
			if(foundFirst == 0){
				cmd1[i] = command[i];
			}
			else{
				cmd2[curEle] = command[i];
				locationTwo++;
				curEle++;
			}
			//If not found ; symbol, keep looking for it
			if(foundFirst == 0){
				//Will go through each letter looking for ; symbol
				for(int j = 0; j < length; j++){
					if(command[i][j] == ';' ){
						locationOne = i;
						result = MULTIPLE;
						if(foundFirst == 0){
							cmd1[i][j] = '\0';
						}
						curEle = 0;
						foundFirst = 1;
					}
				}
			}
		}
		cmd1[locationOne+1] = NULL; //Add null terminators, due to execvp requirements
		cmd2[curEle] = NULL;
		
	}
  	return result;
}

//Function responsible for piping commands. It will make use of pipe(), fork(), and dup2() functions to 
//create respective child process and allow execvp to be run more than once
void pipe_cmd(char** cmd1, char** cmd2){
	int fds[2];
        pipe(fds);
	pid_t pid1, pid2;
	//Will create the first child process that will run the first command in the pipe
        if (pid1 = fork() == 0) {
		dup2(fds[1], 1);
		close(fds[0]);
		close(fds[1]);
                execvp(cmd1[0], cmd1);
                perror("execvp failed");
   	} 
	//Will create the second child that process that will run the second command in the pipe
	else if ((pid2 = fork()) == 0) {
                  dup2(fds[0], 0);
                  close(fds[1]);
                  execvp(cmd2[0], cmd2);
                  perror("execvp failed");
 	} 
	else {
    	close(fds[1]); //Close our pipe
    	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
	}
}

//Function responsible for running several commands. It will make use of the fork function to allow for more than one 
//execvp function
void MultipleCommand(char** cmd1, char** cmd2){
	
	int counter = 0;
	pid_t pid = fork();
	
	//Will run the second command
	if (pid == 0){
		execvp(cmd2[0],cmd2);
		perror("execvp failed");
    	}
	//Will run the first command
   	else if (pid > 0){
		execvp(cmd1[0],cmd1);
		perror("execvp failed");
	}
    	else{
       		printf("fork() failed!\n");
   	}
}

//Function responsible for redirection. It will again make use of fork and pipe functions to allow redirection to occur.
void redirect_cmd(char** cmd, char** file) {
	int fds[2]; 		//Create 2 pipes. one for input other for output
  	int count;  		//Count, used to store what character currently on from reading stdout
  	int fd;     	
  	char currChar;    	//Will store the current character that being written
  	pid_t pid;  

  	pipe(fds);

  	if (fork() == 0) {

    		if (fd < 0) {
      			printf("Error: %s\n", strerror(errno));
      			return;
    		}

    		dup2(fds[0], 0);

    		close(fds[1]);
		//Will write to a file until there is no more characters to copy
    		while ((count = read(0, &currChar, 1)) > 0) {
      			write(fd, &currChar, 1); 
		}


 	} 
	else if ((pid = fork()) == 0) {
    		dup2(fds[1], 1);

    		close(fds[0]);
		//Will execute the first command.
    		execvp(cmd[0], cmd);
    		perror("execvp failed");

  	} 
	else {
    		waitpid(pid, NULL, 0);
    		close(fds[0]);
    		close(fds[1]);
  	}
}





