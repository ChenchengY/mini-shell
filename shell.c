// Modify this file for your assignment
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>

#define BUFFER_SIZE 80
#define built_in_func_num 4

//an string array stores built-in function names
char* built_in[] = {"cd", "exit", "help", "history"};

// the status to check whether the main loop should 
// continue or terminate
int status = 1;

//a 2d array to store last 10 commands
static char hist[11][BUFFER_SIZE + 1];

// trace how many commands do we have for curent shell
// now
static int hist_index = 0;

// the index of the oldest command in the hist[][] 
static int begin = 0;


//handler situation when alarm being activated
//in multiple shell, ensure all the processed are
//killed after alarm
void alarm_handler(int sig){
	kill(0, SIGTERM);
}

//A single handler to help exit
void sigint_handler(int sig){
	if (sig == SIGINT){
		printf("\nmini-shell terminated\n");
		exit(0);
	}	
}

// A built-in command to cd to directory
int shell_cd(char** tokens){
	if (tokens[1] == NULL){
		chdir(getenv("HOME"));
	}else{
		if (chdir(tokens[1]) < 0){
			perror("Error: ");
		}	
	}
	return 1;
}

// exit current shell
int shell_exit(char** tokens){
	return 0;	
}
// print out help message for all built-in functions
int shell_help(char** tokens){
	printf("help for built-in functions\n");
	printf("\tcd: change the working directory," 
			"you could specify the path" 
			"after a blanck space of cd\n");
	printf("\texit: exit the mini-shell\n");
	printf("\thelp: manual page for" 
			" all built-in functions\n");
	printf("\thistory: display the latest 10 commands\n");
	return 1;

}

// add a current command to a list store last ten commands
void shell_add_history(char* input){
	int newIndex = hist_index % 10;
	strcpy(hist[newIndex], input);
	if (hist_index >= 9){
	
		if (newIndex == 9) begin = 0;
		else begin = newIndex + 1;
	}
	hist_index ++;
}

// print out the last ten commands in a right order
int shell_history(){
	int order = 1;
	if (hist_index < 10){
		for (int i = begin; i < hist_index; i ++){
			printf("%d  %s\n", order, hist[i]);
			order ++;
		}
	}
	else{
		for (int k = begin; k < 10; k ++){
			printf("%d  %s\n", order, hist[k]);
			order ++;
		}
	
	
		if (begin != 0){
			for (int j = 0; j < begin; j ++){
		
			printf("%d  %s\n", order, hist[j]);
			order ++;
			}
		}
	}
	return 1;
}

// Add a array of function pointers to store built-in command
int (*built_in_func[])(char**) = {&shell_cd, &shell_exit, 
	&shell_help, &shell_history};

// A function to read the command and store in the char* input
char* read_command(char* input){
	char temp[1024];
	fgets(temp, 1024, stdin);
	if (strcmp(temp, "\n") == 0) return NULL;
	if (strlen(temp) > BUFFER_SIZE){
		printf("Execede 80 character" 
				" limit, please enter" 
				" shorter command, this" 
				" command will not be recorded\n");
		return NULL;
	}
	else strcpy(input, temp);
	shell_add_history(temp);
	return input;
}

// chech whether there is a pipe sign in the command, if no,
// return -1, if yes, then return the index of the pipe sign
int check_pipe(char* input){

	char* temp = strchr(input, '|');
	if (temp == NULL) return -1;
	int index = (int)(temp - input);
	if (index == strlen(input) - 1 || index == 0) return -1;
	return index;
}

//split the first command of a pipe used command  into a token
char* split_command1(char* input, char* command1_buffer){
	
	int index = check_pipe(input);
	if (index < 0) return input;

	strncpy(command1_buffer, input, index);
	command1_buffer[index] = 0;
	
	return command1_buffer;
	
}

//split the second command of a pipe used command into a differnt 
//token
char* split_command2(char* input, char* command2_buffer){
	int index = check_pipe(input);
	if (index < 0) return input;
	strncpy(command2_buffer, input + index + 1, strlen(input));
	return command2_buffer;
	
}

//Parse input into tokens by " " and " \n"
//char** parse_command(char* input){
char** parse_command(char* input, char** tokens){
	int pos = 0;

	char* token = strtok(input, " \n");
	while (token != NULL){
		tokens[pos ++] = token;
       		token = strtok(NULL, " \n");
	}
	tokens[pos] = NULL;
	return tokens;
}

//Launch a process to run non built-in command
int launch_process(char ** tokens){

	pid_t pid;
	pid = fork();
	int status1;
	if (pid < 0){
		perror("mini-shell>");
	}
	else if (pid == 0){
		char path[BUFFER_SIZE + 1];

		if(execvp(tokens[0], tokens) == -1){
			printf("mini-shell>Command" 
					" not found." 
					" Did you mean" 
					" something else?\n");
		}		
		exit(0);
	}
	else{
		waitpid(pid, &status1, 0);
	}

	return 1;


}
//launch a pipe process to execute pipe used command
int launch_pipe_process(char** tokens1, char** tokens2){
	
	int fd[2];
	if (pipe(fd) == -1) exit(EXIT_FAILURE);

	pid_t pid1 = fork();
	int status1;
	if (pid1< 0){
		perror("mini-shell>");
	}
	else if (pid1 == 0){
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		if (execvp(tokens1[0], tokens1) == -1){
			printf("mini-shell>Command not" 
					" found." 
					" Did you mean" 
					" something else?\n");
		}
		exit(EXIT_FAILURE);
	}
	pid_t pid2 = fork();
	int status2;
	if(pid2 < 0){
		perror("mini-shell>");
	}
	else if (pid2 == 0){
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		close(fd[1]);
		if (execvp(tokens2[0], tokens2) == -1){
			printf("mini-shell>Command not" 
					" found." 
					" Did you mean" 
					" something else?\n");
		}
		exit(EXIT_FAILURE);
	}
	else{
		close(fd[0]);
		close(fd[1]);
		waitpid(pid1, &status1, 0);
		waitpid(pid2, &status2, 0);
	}
	return 1;
}


// Determine whether to run built-in command or non built-in command
// if built-in command need to be used, then execute it, otherwise
// run a child process by call the other function
int execute_process(char** tokens){
	
	if (tokens[0] == NULL) return 1;
	
	for (int i = 0; i < built_in_func_num; i ++){
		if (strcmp(tokens[0], built_in[i]) == 0){
			int status = (*built_in_func[i])(tokens);
			return status;
		}
	}
	
	return launch_process(tokens);
}

// A infinite loop to implement the shell. 
void shell_loop(){
	
	char* tokens[BUFFER_SIZE + 1];
	char* tokens1[BUFFER_SIZE + 1];
	char* tokens2[BUFFER_SIZE + 1];
	char* command;
	char* command1;
	char* command2;
	char** parsed_command;
	char** parsed_command1;
	char** parsed_command2;
	int status = 1;

	char buffer[BUFFER_SIZE + 1];
	char command1_buffer[BUFFER_SIZE + 1];
	char command2_buffer[BUFFER_SIZE + 1];
	while(status){
		printf("mini-shell>");
		command = read_command(buffer);
		if (command == NULL) continue;
		if (check_pipe(command) < 0){	
			parsed_command = parse_command(command, tokens);
			status = execute_process(parsed_command);
		}
		else{	

			command1 = split_command1(command, command1_buffer);
			command2 = split_command2(command, command2_buffer);
			parsed_command1 = parse_command(command1, tokens1);
			parsed_command2 = parse_command(command2, tokens2);
			status = launch_pipe_process(
			parsed_command1, parsed_command2);	
		}
	}		

}


int main(){
	signal(SIGALRM, alarm_handler);
	alarm(120);
	signal(SIGINT, sigint_handler);
	shell_loop();
	return 0;
}
