#include <sys/socket.h> // sockaddr_in
#include <netinet/in.h>
#include <stdio.h>     // FILE
#include <unistd.h>    // write()
#include <string.h>    // strlen()
#include <pwd.h>       // fgetpwent()
#include <sys/types.h> // fgetpwent()
#include <arpa/inet.h> // inet_ntoa()
#include <stdlib.h>    // atoi()
#include <stdbool.h>
#include <sys/wait.h> // wait()
#include <signal.h>
#include <time.h>     // time()
#include <dirent.h>   // rewinddir(), opendir(), closedir(), readdir()
#include <sys/stat.h> // lstat(), stat()
#include <pwd.h>      // passwd
#include <grp.h>      // group
#include <fcntl.h>
#define MAX_BUF 200
#define SEND_BUFF 5000
#define MAX_SIZE 256
#define FLAGS (O_RDWR | O_CREAT | O_TRUNC)
#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

struct processInfo
{
    int pid;
    int port;
    int time;
};

void print_pwd(char *, char *);
char *convert_str_to_addr(char *, unsigned int *);
void print_pwd_except_hidden_file(char *pathname, char *result_buff);           // ls
void print_pwd(char *pathname, char *result_buff);                              // ls -a
void print_pwd_in_detail_except_hidden_file(char *pathname, char *result_buff); // ls -l
void print_pwd_in_detail(char *pathname, char *result_buff);                    // ls -al
int cmd_process(char *, char *);                                                // command execute
void sh_chld(int);                                                              // signal handler for SIGCHLD
void sh_alrm(int);                                                              // signal handler for SIGALRM
void sh_int(int);                                                               // signal handler for SIGINT
int log_auth(int);
int username_match(char *);
int passwd_match(char *, char *);
int client_info(struct sockaddr_in *);
int cli_num = 0; // current number of client
int client_fd, ctrlfd;
int datafd;
struct processInfo pInfo[5];

int main(int argc, char *argv[])
{
    struct sockaddr_in servaddr, cliaddr, tmp;
    int clilen;
    FILE *fp_checkIP; // FILE stream to check client's IP

    char temp[MAX_BUF];
    char *host_ip;
    unsigned int port_num;
    char result_buff[SEND_BUFF]; // result

    char *ftpCmd;
    char buff[MAX_BUF];

    if (argc != 2) // exception handling
    {
        write(STDERR_FILENO, "Error: another argument is required\n", 37);
        return 0;
    }

    ////////// Open Control Connection //////////
    /* open socket */
    if ((ctrlfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(STDERR_FILENO, "Server: Can't open stream socket.\n", strlen("Server: Can't open stream socket.\n"));
        return 0;
    }

    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    /* bind */
    if (bind(ctrlfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        write(STDERR_FILENO, "Server: Can't bind local address.\n", strlen("Server: Can't bind local address.\n"));
        return 0;
    }

    /* listen */
    listen(ctrlfd, 5);
    for (;;)
    {
        pid_t pid;
        clilen = sizeof(cliaddr);
        client_fd = accept(ctrlfd, (struct sockaddr *)&cliaddr, &clilen);
        if (client_fd < 0)
        {
            write(STDERR_FILENO, "Server: accept failed.\n", strlen("Server: accept failed.\n"));
            return 0;
        }

        write(STDOUT_FILENO, "** Client is trying to connect **\n", 35);

        ////////// Display Client IP and Port //////////
        if (client_info(&cliaddr) < 0)
            write(STDERR_FILENO, "client_info() err!!\n", strlen("client_info() err!!\n"));

        /* create a child process by using fork() */
        pid = fork();
        pInfo[cli_num].pid = pid;
        pInfo[cli_num].port = cliaddr.sin_port;
        pInfo[cli_num].time = time(NULL);
        cli_num++;

        /* Execution codes for child process */
        if (pid == 0)
        {
            signal(SIGINT, sh_int);

            printf("Child Process ID : %d\n", getpid());

            ////////// IP Access Control //////////
            fp_checkIP = fopen("access.txt", "r");
            if (!fp_checkIP)
            {
                printf("** Fail to open 'access.txt' file **\n");
                write(client_fd, "REJECTION", MAX_BUF);
                break;
            }

            if (fp_checkIP)
            {
                char rcvStr[MAX_BUF];
                char buff[MAX_BUF];
                char c2[MAX_BUF];
                int lineCount = 0;
                char *c1;
                int flag = false;

                strcpy(buff, inet_ntoa(cliaddr.sin_addr));

                /* get the contents of a text file line by line*/
                while (fgets(rcvStr, sizeof(rcvStr), fp_checkIP))
                {
                    /* wildcard */
                    memset(c2, 0, sizeof(c2));

                    int idx1, idx2 = 0;
                    c1 = strtok(rcvStr, ".");
                    if (!strcmp(c1, "*"))
                        while (buff[idx2] != '.')
                            idx2++;
                    else
                    {
                        for (int i = 0; buff[idx2] != '.'; i++)
                        {
                            c2[i] = buff[idx2];
                            idx2++;
                        }

                        // printf("c1: %s, c2: %s\n", c1, c2);
                        if (strcmp(c1, c2))
                            continue;
                    }
                    idx2++;

                    for (int i = 0; i < 2; i++)
                    {
                        memset(c2, 0, sizeof(c2));

                        c1 = strtok(NULL, ".");
                        if (!strcmp(c1, "*"))
                            while (buff[idx2] != '.')
                                idx2++;
                        else
                        {
                            for (int i = 0; buff[idx2] != '.'; i++)
                            {
                                c2[i] = buff[idx2];
                                idx2++;
                            }

                            // printf("c1: %s, c2: %s\n", c1, c2);
                            if (strcmp(c1, c2))
                                continue;
                        }
                        idx2++;
                    }

                    memset(c2, 0, sizeof(c2));

                    c1 = strtok(NULL, "\n");
                    if (!strcmp(c1, "*"))
                        while (buff[idx2] != '\0')
                            idx2++;
                    else
                    {
                        for (int i = 0; buff[idx2] != '\0'; i++)
                        {
                            c2[i] = buff[idx2];
                            idx2++;
                        }

                        // printf("c1: %s, c2: %s\n", c1, c2);
                        if (strcmp(c1, c2))
                            continue;
                    }
                    idx2++;

                    flag = true;
                    break;
                }

                /* if the client's IP exists in the access.txt */
                if (flag == true)
                {
                    write(client_fd, "220 sswlab.kw.ac.kr FTP server (version myfpt [1.0] Fri May 30 14:40:36 KST 2014) ready.", MAX_BUF);
                    printf("** Client is connected **\n");
                }
                /* if the client's IP doesn't exist in the access.txt */
                else
                {
                    write(client_fd, "431 This client can't access. Close the session.", MAX_BUF);
                    printf("** It is NOT authenticated client **\n");
                    continue;
                }
                fclose(fp_checkIP);
            }
            ////////// USER Authentication //////////
            if (log_auth(client_fd) == 0) // if 3 times fail (ok : 1, fail : 0)
            {
                printf("** Fail to log-in **\n");
                close(client_fd);
                continue;
            }
            printf("** Success to log-in **\n");

            while (1)
            {
                bzero(result_buff, sizeof(result_buff));
                bzero(temp, sizeof(temp));
                bzero(buff, sizeof(buff));
                
                /* receive PORT command */
                read(client_fd, temp, MAX_BUF);
                /* data connection */
                if(!strncmp(temp, "PORT", 4))
                {              
                    /* if it is connected successfully */
                    write(STDOUT_FILENO, temp, MAX_BUF);
                    write(STDOUT_FILENO, "\n", 2);

                    write(client_fd, "200 PORT command performed successfully.\n", MAX_BUF);
                    write(STDOUT_FILENO, "200 PORT command performed successfully.\n", 42);

                    host_ip = NULL;
                    host_ip = convert_str_to_addr(temp, (unsigned int *)&port_num);

                    /* receive FTP command */
                    read(client_fd, buff, MAX_BUF);

                    // printf("FTP command: %s\n", buff);



                    ////////// Open Data Connection //////////
                    /* open socket */
                    if ((datafd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        write(STDERR_FILENO, "Server: Can't open stream socket.\n", strlen("Server: Can't open stream socket.\n"));
                        return 0;
                    }
                    bzero((char *)&tmp, sizeof(tmp));
                    tmp.sin_family = AF_INET;
                    tmp.sin_addr.s_addr = inet_addr(host_ip);
                    tmp.sin_port = htons(port_num);

                    /* connect */
                    if (connect(datafd, (struct sockaddr *)&tmp, sizeof(tmp)) == 0)
                    {
                        if (!strncmp(buff, "NLST", 4))
                        {
                            /* send '150 Opening data connection ...' */
                            write(client_fd, "150 Opening data connection for directory list.\n", MAX_BUF);
                            write(STDOUT_FILENO, "150 Opening data connection for directory list.\n", 49);
                        }
                        else if (!strncmp(buff, "RETR", 4) || !strncmp(buff, "STOR", 4))
                        {
                            /* send '150 Opening data connection ...' */
                            write(client_fd, "150 Opening binary data connection for directory list.\n", MAX_BUF);
                            write(STDOUT_FILENO, "150 Opening binary data connection for directory list.\n", 56);
                        }
                    }
                    else
                    {
                        /* send '550 Failed to access.' */
                        write(client_fd, "550 Failed to access.\n", MAX_BUF);
                        write(STDOUT_FILENO, "550 Failed to access.\n", 23);
                    }

                                        int n;
                    if (strncmp(buff, "STOR", 4))
                    { /* command execute */
                        if ((n = cmd_process(buff, result_buff)) < 0)
                        {
                            write(client_fd, "cmd_process() err!!\n", MAX_BUF);
                            write(STDERR_FILENO, "cmd_process() err!!\n", strlen("cmd_process() err!!\n"));
                            continue;
                        }
                    }
                    
                    if(!strncmp(buff, "STOR", 4))
                    {
                        /* receive result through data connection */
                        if (-1 == read(datafd, result_buff, SEND_BUFF))
                        {
                            /* if it is fail to transmit result */
                            write(client_fd, "550 Failed transmission.\n", MAX_BUF);
                            write(STDOUT_FILENO, "550 Failed transmission.\n", 26);
                        }
                        else
                        {
                            /* if it is success to transmit result */
                            write(client_fd, "226 Complete transmission.\n", MAX_BUF);
                            write(STDOUT_FILENO, "226 Complete transmission.\n", 28);
                        }

                        // printf("result_buff: %s\n", result_buff);

                        /* command execute */
                        if (cmd_process(buff, result_buff) < 0)
                        {
                            write(client_fd, "cmd_process() err!!\n", MAX_BUF);
                            write(STDERR_FILENO, "cmd_process() err!!\n", strlen("cmd_process() err!!\n"));
                            continue;
                        }
                    }
                    else
                    {
                        if (n != 1)
                        {
                            /* send result through data connection */
                            if (-1 == write(datafd, result_buff, SEND_BUFF))
                            {
                                /* if it is fail to transmit result */
                                write(client_fd, "550 Failed transmission.\n", MAX_BUF);
                                write(STDOUT_FILENO, "550 Failed transmission.\n", 26);
                            }
                            else
                            {
                                /* if it is success to transmit result */
                                write(client_fd, "226 Complete transmission.\n", MAX_BUF);
                                write(STDOUT_FILENO, "226 Complete transmission.\n", 28);
                            }
                        }
                        else
                        {
                             /* if it is success to transmit result */
                                write(client_fd, "226 Complete transmission.\n", MAX_BUF);
                                write(STDOUT_FILENO, "226 Complete transmission.\n", 28);
                            
                        }
                        
                    }
                    
                    /* close the data connection */
                    close(datafd);
                }
                /* control connection */
                else
                {                    
                    ////////// command execute and result //////////
                    if (cmd_process(temp, result_buff) < 0)
                        write(STDERR_FILENO, "cmd_process() err!!\n", strlen("cmd_process() err!!\n"));
                    
                    /* send processed result to client */
                    write(client_fd, result_buff, SEND_BUFF);
                    write(STDOUT_FILENO, result_buff, strlen(result_buff));
                }
            } // end of while(1)

            close(client_fd);
            return 0;

        } // end of child
        else if (pid > 0)
        {
            signal(SIGALRM, sh_alrm);
            alarm(1);
            signal(SIGCHLD, sh_chld);
        } // end of parent
    }
    close(ctrlfd);

    write(STDOUT_FILENO, "OVER\n", 6);
    return 0;
} // end of main

int cmd_process(char *buff, char *result_buff)
{
    char error[128];
    strcpy(error, "Error: system call\n");

    char getStr[MAX_BUF]; // FTP command
    strcpy(getStr, buff);

    char temp[MAX_BUF];
    strcpy(temp, buff);

    char *cmd = strtok(temp, " \n");
    char *argu1 = strtok(NULL, " \n");

    /////////////////// ls ////////////////////
    if (strcmp(cmd, "NLST") == 0)
    {
        int aflag = 0, lflag = 0, errOptflag = 0;
        int c;
        optind = 0, opterr = 0;

        char *regu[MAX_SIZE];
        char *dir[MAX_SIZE];

        ///// ls: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(getStr, " \n");

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
                errOptflag = 1;
                break;
            default:
                break;
            }
        }

        struct stat buf;
        int reguInd = 0, dirInd = 0, errArguflag = 0;
        for (int i = optind; i < fargc; i++)
        {
            if (!lstat(fargv[i], &buf))
            {
                if (S_ISREG(buf.st_mode))
                    regu[reguInd++] = fargv[i];
                else if (S_ISDIR(buf.st_mode))
                    dir[dirInd++] = fargv[i];
            }
            else
            {
                errArguflag = 1;

                char buf[MAX_BUF];
                sprintf(buf, "ls: cannot access '%s' : No such file or directory\n", fargv[i]);
                strcat(result_buff, buf);
            }
        }

        ///// bubbleSort(Ascending) - regu /////
        for (int i = 0; i < reguInd - 1; i++)
        {
            for (int j = 0; j < reguInd - 1 - i; j++)
            {
                if (strcmp(regu[j], regu[j + 1]) > 0)
                {
                    char *temp = regu[j];
                    regu[j] = regu[j + 1];
                    regu[j + 1] = temp;
                }
            }
        }

        ///// bubbleSort(Ascending) - dir /////
        for (int i = 0; i < dirInd - 1; i++)
        {
            for (int j = 0; j < dirInd - 1 - i; j++)
            {
                if (strcmp(dir[j], dir[j + 1]) > 0)
                {
                    char *temp = dir[j];
                    dir[j] = dir[j + 1];
                    dir[j + 1] = temp;
                }
            }
        }

        ///// ls: execute function /////

        ///// ls: no option /////
        if (!aflag && !lflag && !errOptflag)
            if (!reguInd && !dirInd && !errArguflag) // ls
                print_pwd_except_hidden_file(".", result_buff);
            else // ls argu
            {
                if (reguInd == 1 && dirInd == 0)
                {
                    print_pwd_except_hidden_file(regu[0], result_buff);
                    strcat(result_buff, "\n");
                }
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd_except_hidden_file(dir[0], result_buff);
                else
                {
                    int lb = 0; // line break
                    for (int i = 0; i < reguInd; i++)
                    {
                        if (lb && (lb % 5 == 0))
                            strcat(result_buff, "\n");
                        print_pwd_except_hidden_file(regu[i], result_buff);
                        lb++;
                    }

                    if (!errArguflag)
                        strcat(result_buff, "\n");

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[MAX_BUF];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        strcat(result_buff, buf);
                        print_pwd_except_hidden_file(dir[i], result_buff);
                    }
                }
            }

        ///// ls: option a /////
        else if (aflag && !lflag)
            if (!reguInd && !dirInd && !errArguflag) // ls -a
                print_pwd(".", result_buff);
            else // ls -a argu
            {
                if (reguInd == 1 && dirInd == 0)
                {
                    print_pwd(regu[0], result_buff);
                    strcat(result_buff, "\n");
                }
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd(dir[0], result_buff);
                else
                {
                    int lb = 0; // line break
                    for (int i = 0; i < reguInd; i++)
                    {
                        if (lb && (lb % 5 == 0))
                            strcat(result_buff, "\n");
                        print_pwd(regu[i], result_buff);
                        lb++;
                    }

                    if (!errArguflag)
                        strcat(result_buff, "\n");

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[MAX_BUF];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        strcat(result_buff, buf);
                    }
                }
            }

        ///// ls: option l /////
        else if (!aflag && lflag)
            if (!reguInd && !dirInd & !errArguflag) // ls -l
                print_pwd_in_detail_except_hidden_file(".", result_buff);
            else // ls -l argu
            {
                if (reguInd == 1 && dirInd == 0)
                    print_pwd_in_detail_except_hidden_file(regu[0], result_buff);
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd_in_detail_except_hidden_file(dir[0], result_buff);
                else
                {
                    for (int i = 0; i < reguInd; i++)
                        print_pwd_in_detail_except_hidden_file(regu[i], result_buff);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[MAX_BUF];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        strcat(result_buff, buf);
                        print_pwd_in_detail_except_hidden_file(dir[i], result_buff);
                    }
                }
            }

        ///// ls: option al /////
        else if (aflag && lflag)
            if (!reguInd && !dirInd & !errArguflag) // ls -al
                print_pwd_in_detail(".", result_buff);
            else // ls -al argu
            {
                if (reguInd == 1 && dirInd == 0)
                    print_pwd_in_detail(regu[0], result_buff);
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd_in_detail(dir[0], result_buff);
                else
                {
                    for (int i = 0; i < reguInd; i++)
                        print_pwd_in_detail(regu[i], result_buff);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[MAX_BUF];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        strcat(result_buff, buf);
                        print_pwd_in_detail(dir[i], result_buff);
                    }
                }
            }
        return 0;
    }
    /////////////////// pwd ////////////////////
    else if (strcmp(cmd, "PWD") == 0)
    {
        char dirname[200]; // buffer to hold working directory string

        if (getcwd(dirname, 200) == NULL) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }
        else
            sprintf(result_buff, "257 \"%s\" is current directory.\n", dirname);
    }
    /////////////////// cd ////////////////////
    else if (strcmp(cmd, "CWD") == 0)
    {
        if (chdir(argu1) < 0) // exception handling
        {
            sprintf(result_buff, "550 %s: Can't find such file or directory.\n", argu1);
            return -1;
        }

        strcpy(result_buff, "250 CWD command succeeds.\n");
    }
    /////////////////// cd .. ////////////////////
    else if (strcmp(cmd, "CDUP") == 0)
    {
        if (chdir("..") < 0) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }

        strcpy(result_buff, "250 CDUP command performed successfully.\n");
    }
    /////////////////// mkdir ////////////////////
    else if (strcmp(cmd, "MKD") == 0)
    {
        while (argu1)
        {
            if (mkdir(argu1, 0755) == -1) // exception handling, default permission
            {
                char buf[MAX_BUF];
                sprintf(buf, "550 %s: can't create directory.\n", argu1);
                strcpy(result_buff, buf);
                return -1;
            }
            argu1 = strtok(NULL, " ");
        }
        strcpy(result_buff, "250 MKD command performed successfully.\n");
    }
    /////////////////// rmdir ////////////////////
    else if (strcmp(cmd, "RMD") == 0)
    {
        while (argu1)
        {
            if (rmdir(argu1) == -1) // exception handling, default permission
            {
                char buf[MAX_BUF];
                sprintf(buf, "550 %s: can't remove directory.\n", argu1);
                strcpy(result_buff, buf);
                return -1;
            }
            argu1 = strtok(NULL, " ");
        }
        strcpy(result_buff, "250 RMD command performed successfully.\n");
    }
    /////////////////// delete ////////////////////
    else if (strcmp(cmd, "DELE") == 0)
    {
        if (unlink(argu1) == -1) // exception handling, default permission
        {
            char buf[MAX_BUF];
            sprintf(buf, "550 %s: can't remove directory.\n", argu1);
            strcpy(result_buff, buf);
            return -1;
        }
        strcpy(result_buff, "250 DELE command performed successfully.\n");
    }
    /////////////////// rename ////////////////////
    else if (strcmp(cmd, "RNFR") == 0)
    {
        char buf[MAX_BUF];
        bzero(buf, sizeof(buf));

        struct stat buff;
        if(!lstat(argu1, &buff)) // RNFR
        {
                sprintf(buf, "350 File exists, ready to rename.\n");
                strcpy(result_buff, buf);
        }
        else
        {
            sprintf(buf, "550 %s: can't find such file or directory.\n", argu1);
            strcpy(result_buff, buf);
            return -1;
        }

        /* send result of RNFR to client */
        write(client_fd, result_buff, SEND_BUFF);
        write(STDOUT_FILENO, result_buff, strlen(result_buff));

        /* receive RNTO from client */
        bzero(buf, sizeof(buf));
        read(client_fd, buf, MAX_BUF);

        char *argu2 = strtok(buf, " "); // RNTO
        char *argu3 = strtok(NULL, " "); // file name to change
        bzero(result_buff, sizeof(result_buff));

        if (!lstat(argu3, &buff)) // RNTO
        {
            if (S_ISDIR(buff.st_mode))
            {
                sprintf(buf, "550 %s. can't be renamed.\n", argu1);
                strcpy(result_buff, buf);
                write(STDOUT_FILENO, buf, strlen(buf));
                return -1;
            }            
        }
        else if (rename(argu1, argu3) == -1) // exception handling
        {
            sprintf(buf, "550 %s. can't be renamed.\n", argu1);
            strcpy(result_buff, buf);
            write(STDOUT_FILENO, buf, strlen(buf));
            return -1;
        }
        else
            strcpy(result_buff, "250 RNTO command succeeds.\n");
        
    }
    ///////////////////// dir ////////////////////
    else if (strcmp(cmd, "LIST") == 0)
    {
        char *regu[MAX_SIZE];
        char *dir[MAX_SIZE];

        ///// ls: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(getStr, " \n");

        while (fargc < MAX_SIZE && fargv[fargc] != 0)
        {
            fargc++;
            fargv[fargc] = strtok(NULL, " \n");
        }

        struct stat buf;
        int reguInd = 0, dirInd = 0, errArguflag = 0;
        for (int i = optind; i < fargc; i++)
        {
            if (!lstat(fargv[i], &buf))
            {
                if (S_ISREG(buf.st_mode))
                    regu[reguInd++] = fargv[i];
                else if (S_ISDIR(buf.st_mode))
                    dir[dirInd++] = fargv[i];
            }
            else
            {
                errArguflag = 1;

                char buf[MAX_BUF];
                sprintf(buf, "ls: cannot access '%s' : No such file or directory\n", fargv[i]);
                strcat(result_buff, buf);
            }
        }

        ///// bubbleSort(Ascending) - regu /////
        for (int i = 0; i < reguInd - 1; i++)
        {
            for (int j = 0; j < reguInd - 1 - i; j++)
            {
                if (strcmp(regu[j], regu[j + 1]) > 0)
                {
                    char *temp = regu[j];
                    regu[j] = regu[j + 1];
                    regu[j + 1] = temp;
                }
            }
        }

        ///// bubbleSort(Ascending) - dir /////
        for (int i = 0; i < dirInd - 1; i++)
        {
            for (int j = 0; j < dirInd - 1 - i; j++)
            {
                if (strcmp(dir[j], dir[j + 1]) > 0)
                {
                    char *temp = dir[j];
                    dir[j] = dir[j + 1];
                    dir[j + 1] = temp;
                }
            }
        }

        ///// ls: option al /////
        if (!reguInd && !dirInd & !errArguflag) // ls -al
            print_pwd_in_detail(".", result_buff);
        else // ls -al argu
        {
            if (reguInd == 1 && dirInd == 0)
                print_pwd_in_detail(regu[0], result_buff);
            else if (reguInd == 0 && dirInd == 1)
                print_pwd_in_detail(dir[0], result_buff);
            else
            {
                for (int i = 0; i < reguInd; i++)
                    print_pwd_in_detail(regu[i], result_buff);

                for (int i = 0; i < dirInd; i++)
                {
                    char buf[MAX_BUF];
                    sprintf(buf, "\n%s:\n", dir[i]);
                    strcat(result_buff, buf);
                    print_pwd_in_detail(dir[i], result_buff);
                }
            }
        }
        return 0;
    }
    /////////////////// bin ////////////////////
    else if (strcmp(buff, "TYPE I") == 0)
    {
        strcpy(result_buff, "201 Type set to I.\n");
    }
    /////////////////// ascii ////////////////////
    else if (strcmp(buff, "TYPE A") == 0)
    {
        strcpy(result_buff, "201 Type set to A.\n");
    }
    /////////////////// get ////////////////////
    else if (strcmp(cmd, "RETR") == 0)
    {
        int fp;
        int len = 0;
        
        char path[MAX_BUF];
        bzero(path, sizeof(path));
        getcwd(path, MAX_BUF);
        strcat(path, "/");
        strcat(path, argu1);

        fp = open(path, O_RDONLY, MODE);
        while(1)
        {
            bzero(result_buff, sizeof(result_buff));
            read(fp, result_buff, SEND_BUFF);
            result_buff[SEND_BUFF] = '\0';
            write(datafd, result_buff, SEND_BUFF);

            if(strlen(result_buff) < SEND_BUFF)
                break;
            read(datafd, path, 5);
        }
        // en = read(fp, result_buff, SEND_BUFF);
        close(fp);
        return 1;       
    }
    /////////////////// put ////////////////////
    else if (strcmp(cmd, "STOR") == 0)
    {
        int fp;

        fp = open(argu1, FLAGS, MODE);
        write(fp, result_buff, strlen(result_buff));
        close(fp);
    }
    /////////////////// quit ////////////////////
    else if (strcmp(cmd, "QUIT") == 0)
    {
        close(ctrlfd);
        strcpy(result_buff, "221 Goodbye.\n");
    }
    else
        return -1;

    return 0;

} // end of cmd_process()

void print_pwd_except_hidden_file(char *pathname, char *result_buff)
{
    DIR *dir = opendir(pathname); // opens a directory stream
    struct dirent *dp;
    int dpInd = 0;

    if (!dir) // file
    {
        strcat(result_buff, pathname);
        strcat(result_buff, " ");
        return;
    }

    ///// bubbleSort(Ascending) /////
    while ((dp = readdir(dir)) != 0)
        dpInd++;
    char *fileName[dpInd];

    rewinddir(dir);

    int ind = 0;
    while ((dp = readdir(dir)) != 0)
        fileName[ind++] = dp->d_name;

    rewinddir(dir);

    for (int i = 0; i < dpInd - 1; i++)
    {
        for (int j = 0; j < dpInd - 1 - i; j++)
        {
            if (strcmp(fileName[j], fileName[j + 1]) > 0)
            {
                char *temp = fileName[j];
                fileName[j] = fileName[j + 1];
                fileName[j + 1] = temp;
            }
        }
    }

    ///// separate the file from the directory by appending '/' after the directory /////
    struct stat buf;
    for (int i = 0; i < dpInd; i++)
    {
        char dirname[200]; // buffer to hold working directory string

        if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        strcat(dirname, "/");
        strcat(dirname, fileName[i]);
        // printf("%s\n", dirname);
        if (lstat(dirname, &buf) < 0)
        {
            strcat(result_buff, "lstat error !!!\n");
            continue;
        }
        if (S_ISDIR(buf.st_mode))
            strcat(fileName[i], "/");
    }

    ///// print all files and directories in directory (excluding hidden file) /////
    int lb = 0; // line break
    for (int i = 0; i < dpInd; i++)
    {
        if (fileName[i][0] == '.') // excluding hidden file
            continue;
        else
        {
            char buf[MAX_BUF];
            if (lb && (lb % 5 == 0))
                strcat(result_buff, "\n");
            sprintf(buf, "%-13s", fileName[i]);
            strcat(result_buff, buf);
            lb++;
        }
    }

    if (dpInd > 2)
        strcat(result_buff, "\n");

    if (closedir(dir)) // close the directory stream
        strcat(result_buff, "closed error !!!\n");
} // end of print_pwd_except_hidden_file()

//////////////////////////////////////////////////////////
// print_pwd()(ls -a)					//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory	(including hidden file)	//
//////////////////////////////////////////////////////////
void print_pwd(char *pathname, char *result_buff)
{
    DIR *dir = opendir(pathname); // opens a directory stream
    struct dirent *dp;
    int dpInd = 0;

    if (!dir) // file
    {
        strcat(result_buff, pathname);
        strcat(result_buff, " ");
        return; // exit
    }

    ///// bubbleSort(Ascending) /////
    while ((dp = readdir(dir)) != 0)
        dpInd++;
    char *fileName[dpInd];

    rewinddir(dir);

    int ind = 0;
    while ((dp = readdir(dir)) != 0)
        fileName[ind++] = dp->d_name;

    rewinddir(dir);

    for (int i = 0; i < dpInd - 1; i++)
    {
        for (int j = 0; j < dpInd - 1 - i; j++)
        {
            if (strcmp(fileName[j], fileName[j + 1]) > 0)
            {
                char *temp = fileName[j];
                fileName[j] = fileName[j + 1];
                fileName[j + 1] = temp;
            }
        }
    }

    ///// separate the file from the directory by appending '/' after the directory /////
    struct stat buf;
    for (int i = 0; i < dpInd; i++)
    {
        char dirname[200]; // buffer to hold working directory string

        if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        strcat(dirname, "/");
        strcat(dirname, fileName[i]);
        // printf("%s\n", dirname);
        if (lstat(dirname, &buf) < 0)
        {
            strcat(result_buff, "lstat error !!!\n");
            continue;
        }
        if (S_ISDIR(buf.st_mode))
            strcat(fileName[i], "/");
    }

    ///// print all files and directories in directory /////
    int lb = 0; // line break
    for (int i = 0; i < dpInd; i++)
    {
        char buf[MAX_BUF];
        if (lb && (lb % 5 == 0))
            strcat(result_buff, "\n");
        sprintf(buf, "%-13s", fileName[i]);
        strcat(result_buff, buf);
        lb++;
    }
    strcat(result_buff, "\n");

    if (closedir(dir)) // close the directory stream
        strcat(result_buff, "closed error !!!\n");

} // end of print_pwd()

//////////////////////////////////////////////////////////
// print_pwd_in_detail()(ls -al)			//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory in detail			//
//	    (including hidden file)			//
//////////////////////////////////////////////////////////
void print_pwd_in_detail(char *pathname, char *result_buff)
{
    DIR *dir = opendir(pathname); // opens a directory stream
    struct dirent *dp;
    int dpInd = 0;

    if (!dir) // file
    {
        ///// print file in detail /////
        struct stat buf;
        char dirname[200]; // buffer to hold working directory string

        if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        if (lstat(dirname, &buf) < 0)
            strcat(result_buff, "lstat error !!!\n");

        char permission[10] = {
            '\0',
        };
        char list[512];
        char temp[512];
        struct passwd *user_information;
        struct group *group_information;

        switch (buf.st_mode & S_IFMT)
        {
        case S_IFREG:
            list[0] = '-';
            break;
        case S_IFDIR:
            list[0] = 'd';
            break;
        case S_IFIFO:
            list[0] = 'p';
            break;
        case S_IFLNK:
            list[0] = 'l';
            break;
        }
        list[1] = '\0';

        for (int i = 0, j = 0; i < 3; i++, j += 3)
        {
            if (buf.st_mode & (S_IREAD >> (i * 3)))
                permission[j] = 'r';
            else
                permission[j] = '-';
            if (buf.st_mode & (S_IWRITE >> (i * 3)))
                permission[j + 1] = 'w';
            else
                permission[j + 1] = '-';
            if (buf.st_mode & (S_IEXEC >> (i * 3)))
                permission[j + 2] = 'x';
            else
                permission[j + 2] = '-';
        }
        permission[9] = '\0';

        /* permissions */
        strcat(list, permission);
        sprintf(temp, "%4lu ", buf.st_nlink);
        strcat(list, temp);

        /* user ID */
        user_information = getpwuid(buf.st_uid);
        strcat(list, user_information->pw_name);

        /* group ID */
        group_information = getgrgid(buf.st_gid);
        sprintf(temp, " %s", group_information->gr_name);
        strcat(list, temp);

        /* size in bytes */
        sprintf(temp, " %8ld", buf.st_size);
        strcat(list, temp);

        /* time of last modification */
        struct tm *ltp;
        ltp = localtime(&buf.st_mtime);

        int date[2];
        int time[2];

        date[0] = (ltp->tm_mon) + 1;
        date[1] = ltp->tm_mday;
        time[0] = ltp->tm_hour;
        time[1] = ltp->tm_min;

        sprintf(temp, " %2d월 %2d일 %02d:%02d ", date[0], date[1], time[0], time[1]);
        strcat(list, temp);

        strcat(list, pathname);
        strcat(list, "\n");

        strcat(result_buff, list);
        return; // exit
    }

    ///// bubbleSort(Ascending) /////
    while ((dp = readdir(dir)) != 0)
        dpInd++;
    char *fileName[dpInd];

    rewinddir(dir);

    int ind = 0;
    while ((dp = readdir(dir)) != 0)
        fileName[ind++] = dp->d_name;

    rewinddir(dir);

    for (int i = 0; i < dpInd - 1; i++)
    {
        for (int j = 0; j < dpInd - 1 - i; j++)
        {
            if (strcmp(fileName[j], fileName[j + 1]) > 0)
            {
                char *temp = fileName[j];
                fileName[j] = fileName[j + 1];
                fileName[j + 1] = temp;
            }
        }
    }

    ///// separate the file from the directory by appending '/' after the directory /////
    struct stat buf;
    for (int i = 0; i < dpInd; i++)
    {
        char dirname[200]; // buffer to hold working directory string

        if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        strcat(dirname, "/");
        strcat(dirname, fileName[i]);
        if (lstat(dirname, &buf) < 0)
        {
            strcat(result_buff, "lstat error !!!\n");
            continue;
        }
        if (S_ISDIR(buf.st_mode))
            strcat(fileName[i], "/");

        ///// print all files and directories in directory in detail /////
        char permission[10] = {
            '\0',
        };
        char list[512];
        char temp[512];
        struct passwd *user_information;
        struct group *group_information;

        switch (buf.st_mode & S_IFMT)
        {
        case S_IFREG:
            list[0] = '-';
            break;
        case S_IFDIR:
            list[0] = 'd';
            break;
        case S_IFIFO:
            list[0] = 'p';
            break;
        case S_IFLNK:
            list[0] = 'l';
            break;
        }
        list[1] = '\0';

        for (int i = 0, j = 0; i < 3; i++, j += 3)
        {
            if (buf.st_mode & (S_IREAD >> (i * 3)))
                permission[j] = 'r';
            else
                permission[j] = '-';
            if (buf.st_mode & (S_IWRITE >> (i * 3)))
                permission[j + 1] = 'w';
            else
                permission[j + 1] = '-';
            if (buf.st_mode & (S_IEXEC >> (i * 3)))
                permission[j + 2] = 'x';
            else
                permission[j + 2] = '-';
        }
        permission[9] = '\0';

        /* permissions */
        strcat(list, permission);
        sprintf(temp, "%4lu ", buf.st_nlink);
        strcat(list, temp);

        /* user ID */
        user_information = getpwuid(buf.st_uid);
        strcat(list, user_information->pw_name);

        /* group ID */
        group_information = getgrgid(buf.st_gid);
        sprintf(temp, " %s", group_information->gr_name);
        strcat(list, temp);

        /* size in bytes */
        sprintf(temp, " %8ld", buf.st_size);
        strcat(list, temp);

        /* time of last modification */
        struct tm *ltp;
        ltp = localtime(&buf.st_mtime);

        int date[2];
        int time[2];

        date[0] = (ltp->tm_mon) + 1;
        date[1] = ltp->tm_mday;
        time[0] = ltp->tm_hour;
        time[1] = ltp->tm_min;

        sprintf(temp, " %2d월 %2d일 %02d:%02d ", date[0], date[1], time[0], time[1]);
        strcat(list, temp);

        strcat(list, fileName[i]);
        strcat(list, "\n");

        strcat(result_buff, list);
    }

    if (closedir(dir)) // close the directory stream
        strcat(result_buff, "closed error !!!\n");

} // end of print_pwd_in_detail()

//////////////////////////////////////////////////////////
// print_pwd_in_detail_except_hidden_file()(ls -l)	//
// =====================================================//
// Input: pathname (directory name),            //
//        result_buff (processed result)			//
// Purpose: read the all files and directories in	//
// 	    working directory in detail			//
//		(excluding hidden file)			//
//////////////////////////////////////////////////////////
void print_pwd_in_detail_except_hidden_file(char *pathname, char *result_buff)
{
    DIR *dir = opendir(pathname); // opens a directory stream
    struct dirent *dp;
    int dpInd = 0;

    if (!dir) // file
    {
        ///// print file in detail /////
        struct stat buf;
        char dirname[200]; // buffer to hold working directory string

        if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        if (lstat(dirname, &buf) < 0)
            strcat(result_buff, "lstat error !!!\n");

        char permission[10] = {
            '\0',
        };
        char list[512];
        char temp[512];
        struct passwd *user_information;
        struct group *group_information;

        switch (buf.st_mode & S_IFMT)
        {
        case S_IFREG:
            list[0] = '-';
            break;
        case S_IFDIR:
            list[0] = 'd';
            break;
        case S_IFIFO:
            list[0] = 'p';
            break;
        case S_IFLNK:
            list[0] = 'l';
            break;
        }
        list[1] = '\0';

        for (int i = 0, j = 0; i < 3; i++, j += 3)
        {
            if (buf.st_mode & (S_IREAD >> (i * 3)))
                permission[j] = 'r';
            else
                permission[j] = '-';
            if (buf.st_mode & (S_IWRITE >> (i * 3)))
                permission[j + 1] = 'w';
            else
                permission[j + 1] = '-';
            if (buf.st_mode & (S_IEXEC >> (i * 3)))
                permission[j + 2] = 'x';
            else
                permission[j + 2] = '-';
        }
        permission[9] = '\0';

        /* permissions */
        strcat(list, permission);
        sprintf(temp, "%4lu ", buf.st_nlink);
        strcat(list, temp);

        /* user ID */
        user_information = getpwuid(buf.st_uid);
        strcat(list, user_information->pw_name);

        /* group ID */
        group_information = getgrgid(buf.st_gid);
        sprintf(temp, " %s", group_information->gr_name);
        strcat(list, temp);

        /* size in bytes */
        sprintf(temp, " %8ld", buf.st_size);
        strcat(list, temp);

        /* time of last modification */
        struct tm *ltp;
        ltp = localtime(&buf.st_mtime);

        int date[2];
        int time[2];

        date[0] = (ltp->tm_mon) + 1;
        date[1] = ltp->tm_mday;
        time[0] = ltp->tm_hour;
        time[1] = ltp->tm_min;

        sprintf(temp, " %2d월 %2d일 %02d:%02d ", date[0], date[1], time[0], time[1]);
        strcat(list, temp);

        strcat(list, pathname);
        strcat(list, "\n");

        strcat(result_buff, list);
        return; // exit
    }

    ///// bubbleSort(Ascending) /////
    while ((dp = readdir(dir)) != 0)
        dpInd++;
    char *fileName[dpInd];

    rewinddir(dir);

    int ind = 0;
    while ((dp = readdir(dir)) != 0)
        fileName[ind++] = dp->d_name;

    rewinddir(dir);

    for (int i = 0; i < dpInd - 1; i++)
    {
        for (int j = 0; j < dpInd - 1 - i; j++)
        {
            if (strcmp(fileName[j], fileName[j + 1]) > 0)
            {
                char *temp = fileName[j];
                fileName[j] = fileName[j + 1];
                fileName[j + 1] = temp;
            }
        }
    }

    ///// separate the file from the directory by appending '/' after the directory /////
    struct stat buf;
    for (int i = 0; i < dpInd; i++)
    {
        char dirname[200]; // buffer to hold working directory string

        if (fileName[i][0] == '.') // excluding hidden file
            continue;
        else if (pathname[0] != '/') // relative path
        {
            if (getcwd(dirname, 200) == NULL)
            { // exception handling
                strcat(result_buff, "getcwd error !!!\n");
                return;
            }
            strcat(dirname, "/");
            strcat(dirname, pathname);
        }
        else // absolute path
            strcpy(dirname, pathname);

        strcat(dirname, "/");
        strcat(dirname, fileName[i]);
        if (lstat(dirname, &buf) < 0)
        {
            strcat(result_buff, "lstat error !!!\n");
            continue;
        }
        if (S_ISDIR(buf.st_mode))
            strcat(fileName[i], "/");

        ///// print all files and directories in directory in detail /////
        char permission[10] = {
            '\0',
        };
        char list[512];
        char temp[512];
        struct passwd *user_information;
        struct group *group_information;

        switch (buf.st_mode & S_IFMT)
        {
        case S_IFREG:
            list[0] = '-';
            break;
        case S_IFDIR:
            list[0] = 'd';
            break;
        case S_IFIFO:
            list[0] = 'p';
            break;
        case S_IFLNK:
            list[0] = 'l';
            break;
        }
        list[1] = '\0';

        for (int i = 0, j = 0; i < 3; i++, j += 3)
        {
            if (buf.st_mode & (S_IREAD >> (i * 3)))
                permission[j] = 'r';
            else
                permission[j] = '-';
            if (buf.st_mode & (S_IWRITE >> (i * 3)))
                permission[j + 1] = 'w';
            else
                permission[j + 1] = '-';
            if (buf.st_mode & (S_IEXEC >> (i * 3)))
                permission[j + 2] = 'x';
            else
                permission[j + 2] = '-';
        }
        permission[9] = '\0';

        /* permissions */
        strcat(list, permission);
        sprintf(temp, "%4lu ", buf.st_nlink);
        strcat(list, temp);

        /* user ID */
        user_information = getpwuid(buf.st_uid);
        strcat(list, user_information->pw_name);

        /* group ID */
        group_information = getgrgid(buf.st_gid);
        sprintf(temp, " %s", group_information->gr_name);
        strcat(list, temp);

        /* size in bytes */
        sprintf(temp, " %8ld", buf.st_size);
        strcat(list, temp);

        /* time of last modification */
        struct tm *ltp;
        ltp = localtime(&buf.st_mtime);

        int date[2];
        int time[2];

        date[0] = (ltp->tm_mon) + 1;
        date[1] = ltp->tm_mday;
        time[0] = ltp->tm_hour;
        time[1] = ltp->tm_min;

        sprintf(temp, " %2d월 %2d일 %02d:%02d ", date[0], date[1], time[0], time[1]);
        strcat(list, temp);

        strcat(list, fileName[i]);
        strcat(list, "\n");

        strcat(result_buff, list);
    }

    if (closedir(dir)) // close the directory stream
        strcat(result_buff, "closed error !!!\n");

} // end of print_pwd_in_detail_except_hidden_file()

char *convert_str_to_addr(char *str, unsigned int *port)
{
    static char addr[MAX_BUF];
    bzero(addr, sizeof(addr));

    strtok(str, " ");
    for (int i = 0; i < 3; i++)
    {
        strcat(addr, strtok(NULL, ","));
        strcat(addr, ".");
    }
    strcat(addr, strtok(NULL, ","));

    char *temp = strtok(NULL, ",");
    int a = atoi(temp) << 8;
    temp = strtok(NULL, "\0");
    int b = atoi(temp);
    *port = a + b;

    return addr;
} // end of convert_str_to_addr

void sh_int(int signum)
{
    close(client_fd);
    exit(1);
} // end of sh_int

void sh_chld(int signum)
{
    pid_t pid = wait(NULL);
    close(client_fd);
    printf("Client (%d)'s Release\n\n", pid);

    int idx = cli_num - 1;
    cli_num--;

    int low = 0;
    int high = idx;
    int mid;

    int target;

    while (low <= high)
    {
        mid = (low + high) / 2;

        if (pInfo[mid].pid == pid)
        {
            target = mid;
            break;
        }
        else if (pInfo[mid].pid > pid)
            high = mid - 1;
        else
            low = mid + 1;
    }

    pInfo[target].pid = 0;
    pInfo[target].port = 0;
    pInfo[target].time = 0;

    for (int i = target; i < 5; i++)
    {
        pInfo[i].pid = pInfo[i + 1].pid;
        pInfo[i].port = pInfo[i + 1].port;
        pInfo[i].time = pInfo[i + 1].time;
    }
    pInfo[4].pid = 0;
    pInfo[4].port = 0;
    pInfo[4].time = 0;

} // end of sh_chld

void sh_alrm(int signum)
{
    char buf[MAX_BUF];

    alarm(10);

    /*  print current status */
    sprintf(buf, "Current Number of Client : %d\n", cli_num);
    write(STDOUT_FILENO, buf, strlen(buf));
    bzero(buf, sizeof(buf));
    sprintf(buf, "%6s%6s%6s\n", "PID", "PORT", "TIME");
    write(STDOUT_FILENO, buf, strlen(buf));
    bzero(buf, sizeof(buf));
    for (int i = 0; i < cli_num; i++)
    {
        sprintf(buf, "%6d%6d%6ld\n", pInfo[i].pid, pInfo[i].port, time(NULL) - pInfo[i].time);
        write(STDOUT_FILENO, buf, strlen(buf));
    }

} // end of sh_alrm

int log_auth(int connfd)
{
    char userCmd[MAX_BUF], passCmd[MAX_BUF];
    char *username, *passwd;
    int count = 1;

    while (1)
    {
        memset(passCmd, 0, sizeof(passCmd));
        memset(userCmd, 0, sizeof(userCmd));

        printf("** User is trying to login-in (%d/3) **\n", count);

        read(connfd, userCmd, MAX_BUF);
        strtok(userCmd, " ");
        username = strtok(NULL, "\0");

        /* if it finds username */
        if (username != NULL && username_match(username))
        {
            char buf[MAX_BUF];
            sprintf(buf, "331 Password is required for %s.", username);
            write(connfd, buf, MAX_BUF);
        }
        /* if it can't find username */
        else
        {
            printf("** Log-in failed(Error: can't find username) **\n");
            /* if it is the 3 times fail */
            if (count >= 3)
            {
                write(connfd, "530 Failed to log-in.", MAX_BUF);
                return 0;
            }
            write(connfd, "430 Invalid username or password", MAX_BUF);
            count++;
            continue;
        }

        read(connfd, passCmd, MAX_BUF);
        strtok(passCmd, " ");
        passwd = strtok(NULL, "\0");

        /* if it finds password */
        if (passwd != NULL && passwd_match(username, passwd))
        {
            char buf[MAX_BUF];
            sprintf(buf, "230 User %s logged in.", username);
            write(connfd, buf, MAX_BUF);
            break;
        }
        /* if it can't find password */
        else
        {
            printf("** Log-in failed(Error: can't find password) **\n");
            /* if it is the 3 times fail */
            if (count >= 3)
            {
                write(connfd, "530 Failed to log-in.", MAX_BUF);
                return 0;
            }
            write(connfd, "430 Invalid username or password", MAX_BUF);
            count++;
            continue;
        }
    }
    return 1;
} // end of log_auth

int username_match(char *username)
{
    FILE *fp;
    struct passwd *pw;

    /* get the contents of the 'passwd' file line by line*/
    fp = fopen("passwd", "r");
    if (!fp)
    {
        printf("** Fail to open 'passwd' file **\n");
        return 0;
    }
    while ((pw = fgetpwent(fp)) != NULL)
    {
        if (!strcmp(pw->pw_name, username))
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;

} // end of username_match

int passwd_match(char *username, char *passwd)
{
    FILE *fp;
    struct passwd *pw;

    /* get the contents of the 'passwd' file line by line*/
    fp = fopen("passwd", "r");
    if (!fp)
    {
        printf("** Fail to open 'passwd' file **\n");
        return 0;
    }
    while ((pw = fgetpwent(fp)) != NULL)
    {
        if (!strcmp(pw->pw_name, username))
            if (!strcmp(pw->pw_passwd, passwd))
            {
                fclose(fp);
                return 1;
            }
    }
    fclose(fp);
    return 0;
} // end of passwd_match

int client_info(struct sockaddr_in *client_addr)
{
    char buf[MAX_BUF];
    write(STDOUT_FILENO, "\n==========Client info==========\n", 34);
    sprintf(buf, "client IP: %s\n", inet_ntoa(client_addr->sin_addr));
    write(STDOUT_FILENO, buf, strlen(buf));
    sprintf(buf, "\nclient port: %d\n", client_addr->sin_port);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, "===============================\n", 33);

    return 0;

} // end of client_info