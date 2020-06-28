//////////////////////////////////////////////////////////
// File Name	: cli.c					//
// Date		: 2020/06/02				//
// OS		: Ubuntu 16.04.5 LTS 64bits		//
// Author	: Park Tae Sung				//
// ---------------------------------------------------- //
// Title: System Programming Assignment #3 (client)	//
// Description: Socket Programming 			//
//////////////////////////////////////////////////////////
#include <unistd.h>     // write()
#include <stdlib.h>     // exit()
#include <stdio.h>      // fgets()
#include <string.h>     // strlen()
#include <netinet/in.h> // htonl()
#include <arpa/inet.h>  // inet_addr()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_BUFF 1024
#define RCV_BUFF 5000
#define MAX_SIZE 256

int conv_cmd(char *, char *); // Convert the entered command to FTP command and save it in cmd_buff
void process_result(char *);  // display ls (including options) command result
void sh_int(int);             // signal handler for SIGINT
int socket_fd;

//////////////////////////////////////////////////////////////////
// Main								//
// =============================================================//
// Input: argv							//
// Output: none							//
// Purpose: Main						//
//////////////////////////////////////////////////////////////////
void main(int argc, char **argv)
{
    if (argc != 3) // exception handling
    {
        write(STDERR_FILENO, "Error: another argument is required\n", 37);
        exit(1);
    }

    char buff[MAX_BUFF]; // command entered by USER
    char cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int n;

    int len;
    struct sockaddr_in server_addr;

    ////////// open socket and connect to server //////////
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) // open socket
    {
        write(STDERR_FILENO, "can't create socket.\n", strlen("can't create socket.\n"));
        exit(1);
    }

    bzero(rcv_buff, sizeof(rcv_buff));
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) // connect
    {
        write(STDERR_FILENO, "can't connect.\n", strlen("can't connect.\n"));
        exit(1);
    }

    signal(SIGINT, sh_int);

    for (;;)
    {
        bzero(buff, sizeof(buff));
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(rcv_buff, sizeof(rcv_buff));

        write(STDOUT_FILENO, "> ", 2);
        read(STDIN_FILENO, buff, MAX_BUFF); // takes standard input

        ////////// convert ls (including options) to NLST (including options) //////////
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            continue;
        }

        ////////// send FTP command to server //////////
        if (write(socket_fd, cmd_buff, MAX_BUFF) > 0)
        {
            if (!strcmp(cmd_buff, "QUIT")) // loop until 'QUIT' is entered by USER
                break;

            if (read(socket_fd, rcv_buff, RCV_BUFF) <= 0)
            {
                write(STDERR_FILENO, "read() error!!\n", strlen("read() error!!\n"));
                break;
            }

            ////////// display command result //////////
            process_result(rcv_buff);
        }
        else
        {
            write(STDERR_FILENO, "write() error!!\n", strlen("write() error!!\n"));
            break;
        }
    }
    close(socket_fd);
}

//////////////////////////////////////////////////////////////////
// conv_cmd							//
// =============================================================//
// Input: buff, cmd_buff					//
// Output: 0 success        					//
//        -1 fail                           			//
// Purpose: Convert the entered command to FTP command and save //
// it in cmd_buff						//
//////////////////////////////////////////////////////////////////
int conv_cmd(char *buff, char *cmd_buff)
{
    char getStr[MAX_BUFF]; // user command
    strcpy(getStr, buff);

    char temp[MAX_BUFF];
    strcpy(temp, buff);

    if (!strcmp(getStr, "\n"))
        return -1;

    char *cmd = strtok(getStr, " \n");
    char *tmp = strtok(NULL, " \n");
    char *argu = malloc(sizeof(char) * MAX_BUFF);
    while (tmp)
    {
        strcat(argu, " ");
        strcat(argu, tmp);
        tmp = strtok(NULL, " \n");
    }

    opterr = 0;
    optind = 0;

    /////////////////// ls ////////////////////
    if (strcmp(cmd, "ls") == 0)
    {
        int aflag = 0, lflag = 0;
        int c;

        ///// ls: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " \n");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        while ((c = getopt(fargc, fargv, "al")) != -1)
        {
            switch (c)
            {
            case 'a':
                aflag = 1;
                break;
            case 'l':
                lflag = 1;
                break;
            case '?': // ls: wrong option
                write(STDOUT_FILENO, "ls: invalid option\n", strlen("ls: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "NLST");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "dir") == 0) // dir
    {
        int c;

        ///// dir: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // dir: wrong option
                write(STDOUT_FILENO, "dir: invalid option\n", strlen("dir: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "LIST");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "pwd") == 0) // pwd
    {
        int c;

        ///// pwd: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // pwd: wrong option
                write(STDOUT_FILENO, "pwd: invalid option\n", strlen("pwd: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "PWD");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "cd") == 0) // cd
    {
        int c;

        ///// cd: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // cd: wrong option
                write(STDOUT_FILENO, "cd: invalid option\n", strlen("cd: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "CWD");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "mkdir") == 0) // mkdir
    {
        int c;

        ///// mkdir: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        if (fargc < 2)
		{
			write(STDOUT_FILENO, "rmdir: missing operand\n", strlen("rmdir: missing operand\n"));
			return -1;
		}

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // mkdir: wrong option
                write(STDOUT_FILENO, "mkdir: invalid option\n", strlen("mkdir: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }
        strcpy(cmd_buff, "MKD");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "delete") == 0) // delete
    {
        int c;

        ///// delete: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        if (fargc != 2)
        {
            write(STDOUT_FILENO, "delete: wrong operand\n", strlen("delete: wrong operand\n"));
            return -1;
        }

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // delete: wrong option
                write(STDOUT_FILENO, "delete: invalid option\n", strlen("delete: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "DELE");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "rmdir") == 0) // rmdir
    {
        int c;

        ///// rmdir: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        if (fargc < 2)
		{
			write(STDOUT_FILENO, "rmdir: missing operand\n", strlen("rmdir: missing operand\n"));
			return -1;
		}

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // rmdir: wrong option
                write(STDOUT_FILENO, "rmdir: invalid option\n", strlen("rmdir: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "RMD");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "rename") == 0) // rename
    {
        int c;

        ///// rename: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

		if (fargc != 3)
		{
            write(STDOUT_FILENO, "rename: wrong operand\n", strlen("rename: extra operand\n"));
            return -1;
		}

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // rename: wrong option
                write(STDOUT_FILENO, "rename: invalid option\n", strlen("rename: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "RNFR");
        ///// combine FTP command and optional argument /////
        char *argu1 = strtok(argu, " \n");
        char *argu2 = strtok(NULL, " \n");
        strcat(cmd_buff, " ");
        strcat(cmd_buff, argu1);
        strcat(cmd_buff, "\n");
        strcat(cmd_buff, "RNTO");
        strcat(cmd_buff, " ");
        strcat(cmd_buff, argu2);
    }
    else if (strcmp(cmd, "quit") == 0) // quit
    {
        char *argu1 = strtok(NULL, " \n");
        int c;

        ///// quit: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " \n");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // quit: wrong option
                write(STDOUT_FILENO, "quit: invalid option\n", strlen("quit: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        ///// check arguments /////
        if (argu1) // Error: command does not exist
        {
            write(STDOUT_FILENO, "Error: another argument is not required\n", strlen("Error: another argument is not required\n"));
            return -1;
        }

        strcpy(cmd_buff, "QUIT");
    }
    else
        return -1; // Error: invalid command

    return 0;

} // end of conv_cmd()

//////////////////////////////////////////////////////////////////
// process_result						//
// =============================================================//
// Input: rcv_buff						//
// Output: none        						//
// Purpose: display ls (including options) command result	//
//////////////////////////////////////////////////////////////////
void process_result(char *rcv_buff)
{
    write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
    bzero(rcv_buff, sizeof(rcv_buff));
} // end of process_result()

void sh_int(int signum)
{
    write(socket_fd, "QUIT", strlen("QUIT"));
    exit(1);
} // end of sh_int
