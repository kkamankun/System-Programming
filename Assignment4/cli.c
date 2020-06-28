#include <sys/socket.h> // sockaddr_in
#include <netinet/in.h>
#include <unistd.h>    // write(), getpass()
#include <stdlib.h>    // exit()
#include <string.h>    // strlen()
#include <stdio.h>     // printf()
#include <arpa/inet.h> // inet_addr()
#include <fcntl.h>
#define MAX_BUF 200
#define MAX_SIZE 200
#define RCV_BUFF 5000
#define CONT_PORT 12345
#define FLAGS (O_RDWR | O_CREAT | O_TRUNC)
#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void log_in(int);
char *convert_addr_to_str(unsigned long, unsigned int);
int conv_cmd(char *, char *); // Convert the entered command to FTP command and save it in cmd_buff

int main(int argc, char *argv[])
{
    int ctrlfd, datafd, p_pid;
    int connfd;
    struct sockaddr_in servaddr, cliaddr, tmp;
    char userCmd[MAX_BUF]; char portCmd[MAX_BUF];
    char buff[MAX_BUF]; // command enterend by USER
    char cmd_buff[MAX_BUF];
    char rcv_buff[RCV_BUFF];
    char buf[MAX_BUF];

    char *hostport;

    if (argc != 3) // exception handling
    {
        write(STDERR_FILENO, "Error: another argument is required\n", 37);
        exit(1);
    }

    ////////// Open Control Connection //////////
    /* open socket */
    if ((ctrlfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(STDERR_FILENO, "can't create socket.\n", strlen("can't create socket.\n"));
        exit(1);
    }
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    /* connect */
    connect(ctrlfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    write(STDOUT_FILENO, "Connected to sswlab.kw.ac.kr.\n", 31);

    ////////// IP access control, User authentication //////////
    log_in(ctrlfd);

    hostport = convert_addr_to_str(servaddr.sin_addr.s_addr, CONT_PORT);

    for(;;)
    {        
        /* receives user command from standard input */
        bzero(buff, sizeof(buff));
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(rcv_buff, sizeof(rcv_buff));

        write(STDOUT_FILENO, "ftp> ", 6);
        read(STDIN_FILENO, buff, MAX_BUF);

        if(!strcmp(buff,"\n"))
        {
            write(STDERR_FILENO, "Error: please enter a command\n", 31);
            continue;
        }

        /* data connection */
        if (!strncmp(buff, "ls", 2) || !strncmp(buff, "dir", 3) || !strncmp(buff, "get", 3) || !strncmp(buff, "put", 3))
        {
            /* transmit PORT command */
            bzero(portCmd, sizeof(portCmd));
            strcpy(portCmd, "PORT ");
            strcat(portCmd, hostport);
            write(ctrlfd, portCmd, MAX_BUF);

            write(STDOUT_FILENO, "---> ", 6);
            write(STDOUT_FILENO, portCmd, strlen(portCmd));
            write(STDOUT_FILENO, "\n", 2);

            /* receive '200 PORT command performed successfully.' */
            bzero(buf, sizeof(buf));
            read(ctrlfd, buf, MAX_BUF);
            write(STDOUT_FILENO, buf, strlen(buf));

            if (conv_cmd(buff, cmd_buff) < 0)
                write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));

            /* send FTP command */
            write(ctrlfd, cmd_buff, MAX_BUF);

            bzero(buf, sizeof(buf));
            sprintf(buf, "---> %s\n", cmd_buff);
            write(STDOUT_FILENO, buf, strlen(buf));

            ////////// Open Data Connection //////////
            /* open socket */
            if ((datafd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
            {
                write(STDERR_FILENO, "Server: Can't open stream socket.\n", strlen("Server: Can't open stream socket.\n"));
                return 0;
            }
            bzero((char *)&tmp, sizeof(tmp));
            tmp.sin_family = AF_INET;
            tmp.sin_addr.s_addr = htonl(INADDR_ANY);
            tmp.sin_port = htons(CONT_PORT);

            /* bind */
            if (bind(datafd, (struct sockaddr *)&tmp, sizeof(tmp)) < 0)
            {
                write(STDERR_FILENO, "Server: Can't bind local address.\n", strlen("Server: Can't bind local address.\n"));
                return 0;
            }
            /* listen */
            listen(datafd, 5);

            /* accept */
            connfd = accept(datafd, NULL, NULL);
            if (connfd < 0)
            {
                write(STDERR_FILENO, "Server: accept failed.\n", strlen("Server: accept failed.\n"));
                return 0;
            }

            /* receive '150 Opening data connection ...' */
            bzero(buf, sizeof(buf));
            read(ctrlfd, buf, MAX_BUF);
            write(STDOUT_FILENO, buf, strlen(buf));

            if(!strncmp(buff, "put", 3))
            {

                char *argu;
                int fp;

                strtok(cmd_buff, " ");
                argu = strtok(NULL, " ");

                fp = open(argu, O_RDONLY, MODE);
                read(fp, rcv_buff, RCV_BUFF);
                close(fp);

                /* send result to server through data connection */
                write(connfd, rcv_buff, RCV_BUFF);
            }
            else
            {
                /* receive result from server through data connection */
                // read(connfd, rcv_buff, RCV_BUFF);
                if (!strncmp(buff, "get", 3))
                {
                    char *argu;
                    int fp;

                    strtok(cmd_buff, " ");
                    argu = strtok(NULL, " ");

                    char path[MAX_BUF];
                    bzero(path, sizeof(path));
                    getcwd(path, MAX_BUF);
                    strcat(path, "/");
                    strcat(path, argu);

                    fp = open(path, FLAGS, MODE);
                    for(;;)
                    {
                        bzero(rcv_buff, sizeof(rcv_buff));
                        read(connfd, rcv_buff, RCV_BUFF);
                        rcv_buff[RCV_BUFF] = '\0';

                        if (strlen(rcv_buff) < RCV_BUFF){
                            write(fp, rcv_buff, strlen(rcv_buff));      
                            break;
                        }
                        write(fp, rcv_buff, RCV_BUFF);    
                        write(connfd, "test", 5);
                    }
                    //write(fp, rcv_buff, strlen(rcv_buff));
                    close(fp);
                }
                else
                    write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
            }
            
            /* receive '226 Complete transmission.' or 'Failed transmission */
            bzero(buf, sizeof(buf));
            read(ctrlfd, buf, MAX_BUF);
            write(STDOUT_FILENO, buf, strlen(buf));

            if(!strncmp(buff, "put", 3))
            {
                bzero(buf, sizeof(buf));
                sprintf(buf, "OK. %ld bytes is sent.\n", strlen(rcv_buff));
                write(STDOUT_FILENO, buf, strlen(buf));
            }
            else
            {
                bzero(buf, sizeof(buf));
                sprintf(buf, "OK. %ld bytes is received.\n", strlen(rcv_buff));
                write(STDOUT_FILENO, buf, strlen(buf));
            }

            close(connfd);
            close(datafd);
        }
        /* control connection */
        else
        {
            /* send converted command to server through control connection */
            if (conv_cmd(buff, cmd_buff) < 0)
            {
                write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
                continue;
            }

            if(!strncmp(cmd_buff, "RNFR", 4))
            {
                char *tok1 = strtok(cmd_buff, "\n");
                char *tok2 = strtok(NULL, "\n");
                /* send RNFR */
                write(ctrlfd, tok1, MAX_BUF);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "---> %s\n", tok1);
                write(STDOUT_FILENO, buf, strlen(buf));

                /* receive result from server through control connection */
                bzero(rcv_buff, sizeof(rcv_buff));
                read(ctrlfd, rcv_buff, RCV_BUFF);
                write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));

                if(!strncmp(rcv_buff, "350", 3))
                {
                    /* send RNTO */
                    write(ctrlfd, tok2, MAX_BUF);
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "---> %s\n", tok2);
                    write(STDOUT_FILENO, buf, strlen(buf));

                    /* receive result from server through control connection */
                    bzero(rcv_buff, sizeof(rcv_buff));
                    read(ctrlfd, rcv_buff, RCV_BUFF);
                    write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
                }
            }
            else
            {
                /* send FTP command */
                write(ctrlfd, cmd_buff, MAX_BUF);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "---> %s\n", cmd_buff);
                write(STDOUT_FILENO, buf, strlen(buf));

                /* receive result from server through control connection */
                bzero(rcv_buff, sizeof(rcv_buff));
                read(ctrlfd, rcv_buff, RCV_BUFF);
                write(STDOUT_FILENO, rcv_buff, RCV_BUFF);

                if(!strncmp(rcv_buff, "221", 3))
                    break;
            }
        }  
    } // end of for(;;)

    close(ctrlfd);
    return 0;
} // end of main

int conv_cmd(char *buff, char *cmd_buff)
{
    char getStr[MAX_BUF]; // user command
    strcpy(getStr, buff);

    char temp[MAX_BUF];
    strcpy(temp, buff);

    if (!strcmp(getStr, "\n"))
        return -1;

    char *cmd = strtok(getStr, " \n");
    char *tmp = strtok(NULL, " \n");
    char argu[MAX_BUF];
    bzero(argu, sizeof(argu));
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
        if(!strcmp(argu, " .."))
            strcpy(cmd_buff, "CDUP");
        else if (argu)
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
    else if (strcmp(cmd, "bin") == 0 || strcmp(buff, "type binary\n") == 0) // bin / type binary
    {
        char *argu1 = strtok(NULL, " \n");
        int c;

        ///// bin: parsing program execution option /////
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
            case '?': // bin: wrong option
                write(STDOUT_FILENO, "bin: invalid option\n", strlen("bin: invalid option\n"));
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

        strcpy(cmd_buff, "TYPE I");
    }
    else if (strcmp(cmd, "ascii") == 0 || strcmp(buff, "type ascii\n") == 0 ) // ascii / type ascii
    {
        char *argu1 = strtok(NULL, " \n");
        int c;

        ///// ascii: parsing program execution option /////
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
            case '?': // ascii: wrong option
                write(STDOUT_FILENO, "ascii: invalid option\n", strlen("ascii: invalid option\n"));
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

        strcpy(cmd_buff, "TYPE A");
    }
    else if (strcmp(cmd, "get") == 0) // get
    {
        int c;

        ///// get: parsing program execution option /////
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
            case '?': // get: wrong option
                write(STDOUT_FILENO, "get: invalid option\n", strlen("get: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "RETR");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
    }
    else if (strcmp(cmd, "put") == 0) // put
    {
        int c;

        ///// get: parsing program execution option /////
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
            case '?': // get: wrong option
                write(STDOUT_FILENO, "put: invalid option\n", strlen("put: invalid option\n"));
                return -1;
                break;
            default:
                break;
            }
        }

        strcpy(cmd_buff, "STOR");
        ///// combine FTP command and optional argument /////
        if (argu)
            strcat(cmd_buff, argu);
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

char *convert_addr_to_str(unsigned long ip_addr, unsigned int port)
{
    static char addr[MAX_BUF];

    int one = ip_addr << 24;
    one = one >> 24;

    int two = ip_addr << 16;
    two = two >> 24;

    int three = ip_addr << 8;
    three = three >> 24;

    int four = ip_addr >> 24;

    int five = port / 256;
    int six = port % 256;

    sprintf(addr, "%d,%d,%d,%d,%d,%d", one, two, three, four, five, six);

    return addr;
} // end of convert_addr_to_str

void log_in(int sockfd)
{
    int n;
    char username[MAX_BUF], *passwd, buf[MAX_BUF];

    ////////// Receives "REJECTION" or "ACCEPTED" from Server //////////
    read(sockfd, buf, MAX_BUF);
    /* if it received "REJECTION" */
    if (!strncmp(buf, "431", 3))
    {
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, "\n", 2);
        return;
    }
    /* if it received "ACCEPTED" */
    else if (!strncmp(buf, "220", 3))
    {
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, "\n", 2);
    }
    /* exception handling */
    else
    {
        write(STDOUT_FILENO, "** Connection refused **\n", 26);
        exit(0);
    }

    ////////// USER Authentication //////////
    for (;;)
    {
        bzero(buf, sizeof(buf));
        memset(username, 0, sizeof(username));

        ////////// receives username from standard input and passes to server //////////
        /* receives username */
        write(STDOUT_FILENO, "Name : ", 8);
        read(STDIN_FILENO, username, MAX_BUF);
        username[strlen(username) - 1] = '\0';

        /* passes username */
        char userCmd[MAX_BUF];
        sprintf(userCmd, "USER %s", username);
        write(sockfd, userCmd, MAX_BUF);
        write(STDOUT_FILENO, "---> ", 6);
        write(STDOUT_FILENO, userCmd, strlen(userCmd));
        write(STDOUT_FILENO, "\n", 2);

        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';
        if (!strncmp(buf, "331", 3))
        {
            write(STDOUT_FILENO, buf, strlen(buf));
            write(STDOUT_FILENO, "\n", 2);

            /* receives password */
            passwd = getpass("Password : ");

            /* passes password */
            char passCmd[MAX_BUF];
            sprintf(passCmd, "PASS %s", passwd);
            write(sockfd, passCmd, MAX_BUF);
            write(STDOUT_FILENO, "---> ", 6);
            write(STDOUT_FILENO, "PASS ********\n", 15);

            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            /* if password is match */
            if (!strncmp(buf, "230", 3))
            {
                write(STDOUT_FILENO, buf, strlen(buf));
                write(STDOUT_FILENO, "\n", 2);
                break;
            }
            /* if password isn't match */
            else if (!strncmp(buf, "430", 3))
            {
                write(STDOUT_FILENO, buf, strlen(buf));
                write(STDOUT_FILENO, "\n", 2);
            }
            /* three times fail */
            else // "DISCONNECTION"
            {
                write(STDOUT_FILENO, buf, strlen(buf));
                write(STDOUT_FILENO, "\n", 2);
                close(sockfd);
                exit(0);
            }
        }
        else if (!strncmp(buf, "430", 3))
        {
            write(STDOUT_FILENO, buf, strlen(buf));
            write(STDOUT_FILENO, "\n", 2);
        }
        /* three times fail */
        else // "DISCONNECTION"
        {
            write(STDOUT_FILENO, buf, strlen(buf));
            write(STDOUT_FILENO, "\n", 2);
            close(sockfd);
            exit(0);
        }
    }
} // end of log_in
