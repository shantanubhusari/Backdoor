#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#include <sys/wait.h>
#include <string.h> 
#include <ctype.h>

// Define Socket Descriptors

int sock_fd_1, sock_fd_2;

// Define standard responses

char resp_not_ok[15000] = "HTTP/1.1 404 Not Found \r\n"
			  "Content-Length: 0\r\n"
			  "Content-Type: text/html; charset=UTF-8\r\n\r\n";
			
// Define Functions
void sigint_handler(int );
char *command_handler(char *);
void response_handler(int );
char hex_conv(char );


// Function Implementations 

// Interrupt Handler

void sigint_handler(int sig_num)
{
	close(sock_fd_1);
	exit(0);
}

// Command Handler
// Reference : http://www.geekhideout.com

char *command_handler(char *string1) 
{
	char *ptr_string1 = string1, *buffer_2 = malloc(strlen(string1) + 1), *ptr_buffer = buffer_2;
	while (*ptr_string1) 
	{
		if (*ptr_string1 == '%') 
		{
      			if (ptr_string1[1] && ptr_string1[2]) 
			{
        			*ptr_buffer++ = hex_conv(ptr_string1[1]) << 4 | hex_conv(ptr_string1[2]);
       				 ptr_string1 += 2;
      			}
    		} 
		else 
			if (*ptr_string1 == '+') 
			{ 
      				*ptr_buffer++ = ' ';
		    	} 
			else {
		      		*ptr_buffer++ = *ptr_string1;
	    	        }
   		 	ptr_string1++;
	}
	*ptr_buffer = '\0';
	return buffer_2;
}

// Hex to integer converter
char hex_conv(char ch) 
{
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

// Response Handler

void response_handler(int fd)
{
	int buffer_size = 1024;
	char *buffer_1 = malloc(buffer_size);
	int x,y,z;

	recv(fd, buffer_1, buffer_size,0);
	printf("-----------------------------\n");
	printf("%s\n", buffer_1);
	printf("-----------------------------\n");

	//extract command from URL
	
	char *cmdline[3];
	cmdline[0] = strtok(buffer_1," \t\n");
	cmdline[1] = strtok(NULL, " \t");
	cmdline[2] = strtok(NULL, " \t\n");

	x = strncmp(cmdline[0], "GET", 3);
	y = strncmp(cmdline[1], "/exec/", 6);
	//if '/exec/' is encoded, check and decode
	if (y != 0)
	{
		char *exec_line = malloc(strlen(cmdline[1])+1);
		exec_line = command_handler(cmdline[1]);
		y = strncmp(exec_line, "/exec", 5);
	}
	z = strncmp(cmdline[2], "HTTP/1.1", 8);

	char *cmdline2[2];
	cmdline2[0] = strtok(cmdline[1],"/");
	cmdline2[1] = strtok(NULL, "\n");
	
	//Check if the request is valid
	if(x==0 && y==0 && z==0)
	{
		char resp_ok[15000];	
		char *cmd = malloc(strlen(cmdline2[1])+1);
		cmd = command_handler(cmdline2[1]);

		strcat(cmd, " 2>&1");

		FILE* file = popen(cmd, "r");
		char buffer[15000];
		int len;
		char *output=(char*)malloc(15000);
		while(fgets(buffer,15000,file)) 
		{
			strcat(output, buffer);
    		}

		len = strlen(output);
		
		sprintf(resp_ok,"HTTP/1.1 200 OK\r\nContent-Length : %d\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n",len);
		sprintf(resp_ok,output,sizeof(output));
		
		fflush(stdout);
	    	pclose(file);
			
		write(fd,resp_ok,strlen(resp_ok));
				
	}
	else
	{
		write(fd, resp_not_ok, strlen(resp_not_ok));
	}
}

// main function

void main(int c, char* argv[])
{
	signal(SIGINT, sigint_handler);  //to handle interrupt

	struct sockaddr_in s_addr, c_addr;
	socklen_t sin_len = sizeof(c_addr);
	pid_t pid;

	//Convert port number from command line arument to int
	char port_number[10];
	strcpy(port_number, argv[1]);
	int port = atoi(port_number);
	printf("port no : %d \n", port);

	//Create Socket
	sock_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd_1 < 0)
		err(1, "Cannot open the socket");

	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = INADDR_ANY;
	s_addr.sin_port = htons(port);

	//Binding Process
	if (bind(sock_fd_1, (struct sockaddr *) &s_addr, sizeof (s_addr)) != 0)
	{
		close(sock_fd_1);
		err(1,"Binding Problem !");
	}

	listen(sock_fd_1, 5);
	while (1)
	{
		
		sock_fd_2 = accept(sock_fd_1, (struct sockaddr *) &c_addr, &sin_len);
		printf("Received connection .... .... .... ....\n");

		if (sock_fd_2 == -1)
		{
			perror("Cannot Accept");
			continue;
		}

		if ((pid = fork()) ==0)
		{
			close(sock_fd_1);

			response_handler(sock_fd_2);

			close(sock_fd_2);

			exit(EXIT_SUCCESS);
		}
		close(sock_fd_2);
	}
}


