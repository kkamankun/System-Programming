//////////////////////////////////////////////////////////
// File Name	: srv.c					                //
// Date		: 2020/06/03				                //
// OS		: Ubuntu 16.04.5 LTS 64bits		            //
// Author	: Park Tae Sung				                //
// ---------------------------------------------------- //
// Title: System Programming Assignment #3 (server)	    //
// Description: Socket Programming 			            //
//////////////////////////////////////////////////////////
#include <unistd.h>     // write()
#include <stdlib.h>     // exit()
#include <string.h>     // strlen(), bzero()
#include <sys/socket.h> // accept(), bind(), listen()
#include <netinet/in.h> // htonl()
#include <stdio.h>      // sprintf()
#include <dirent.h>     // rewinddir(), opendir(), closedir(), readdir()
#include <sys/stat.h>   // lstat(), stat()
#include <pwd.h>        // passwd
#include <grp.h>        // group
#include <time.h>       // ctime()
#include <arpa/inet.h>  // inet_ntoa()
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>   // wait()
#include <signal.h>
#include <time.h>       // time()
#define MAX_BUFF 1024
#define SEND_BUFF 5000
#define MAX_SIZE 256
#define TEN_SECOND 10000

struct processInfo
{
    int pid;
    int port;
    int time;
};

void sh_chld(int);                     // signal handler for SIGCHLD
void sh_alrm(int);                     // signal handler for SIGALRM
void sh_int(int);                      // signal handler for SIGINT
int client_info(struct sockaddr_in *); // display client ip and port
int cmd_process(char *, char *);       // command execute
int cli_num = 0;                       // current number of client
int socket_fd, client_fd;
struct processInfo pInfo[5];

void print_pwd_except_hidden_file(char *pathname, char *result_buff);           // ls
void print_pwd(char *pathname, char *result_buff);                              // ls -a
void print_pwd_in_detail_except_hidden_file(char *pathname, char *result_buff); // ls -l
void print_pwd_in_detail(char *pathname, char *result_buff);                    // ls -al

//////////////////////////////////////////////////////////////////
// Main								                            //
// =============================================================//
// Input: argv							                        //
// Output: 0 success						                    //
// Purpose: Main						                        //
//////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    if (argc != 2) // exception handling
    {
        write(STDERR_FILENO, "Error: another argument is required\n", 37);
        return 0;
    }

    char buff[MAX_BUFF];         // FTP command
    char result_buff[SEND_BUFF]; // result
    int n;

    struct sockaddr_in server_addr, client_addr;
    int clilen;

    ////////// open socket and listen //////////
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) // open socket
    {
        write(STDERR_FILENO, "Server: Can't open stream socket.\n", strlen("Server: Can't open stream socket.\n"));
        return 0;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        write(STDERR_FILENO, "Server: Can't bind local address.\n", strlen("Server: Can't bind local address.\n"));
        return 0;
    }

    listen(socket_fd, 5); // listen
    for (;;)
    {
        pid_t pid;
        clilen = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clilen);
        if (client_fd < 0)
        {
            write(STDERR_FILENO, "Server: accept failed.\n", strlen("Server: accept failed.\n"));
            return 0;
        }

        ////////// display client ip and port //////////
        if (client_info(&client_addr) < 0)
            write(STDERR_FILENO, "client_info() err!!\n", strlen("client_info() err!!\n"));

        /* create a child process by using fork() */
        pid = fork();
        pInfo[cli_num].pid = pid;
        pInfo[cli_num].port = client_addr.sin_port;
        pInfo[cli_num].time = time(NULL);
        cli_num++;

        /* Execution codes for child process */
        if (pid == 0)
        {
            signal(SIGINT, sh_int);

            printf("Child Process ID : %d\n", getpid());

            ////////// Receive FTP command from client //////////
            while (1)
            {
                memset(buff, '\0', sizeof(buff));
                memset(result_buff, '\0', sizeof(result_buff));

                n = read(client_fd, buff, MAX_BUFF);
                if (n > 0)
                {
                    if (!strcmp(buff, "QUIT"))
                    {
                        exit(1);
                        break;
                    }
                    else
                    {
                        printf("Child Process ID : %d\n", getpid());
                        ////////// display client's commands //////////
                        write(STDOUT_FILENO, buff, MAX_BUFF);
                        write(STDOUT_FILENO, "\n", 2);
                        buff[n] = '\0';
                    }
                }
                else
                    break;

                ////////// command execute and result //////////
                if (cmd_process(buff, result_buff) < 0)
                {
                    write(STDERR_FILENO, "cmd_process() err!!\n", strlen("cmd_process() err!!\n"));
                    write(client_fd, "cmd_process() err!!\n", strlen("cmd_process() err!!\n"));
                }
                else
                {
                    ////////// send processed result to client //////////
                    write(client_fd, result_buff, SEND_BUFF);
                }
            }
        }
        /* Execution codes for parent process */
        else if (pid > 0)
        {
            signal(SIGALRM, sh_alrm);
            alarm(1);
            signal(SIGCHLD, sh_chld);
        }
    }
    close(socket_fd);

    printf("OVER\n");
    return 0;
} // end of main

//////////////////////////////////////////////////////////////////
// sh_int							                            //
// =============================================================//
// Input: signum						                        //
// Output: none							                        //
// Purpose: signal handler for SIGINT 				            //
//////////////////////////////////////////////////////////////////
void sh_int(int signum)
{
    close(client_fd);
    exit(1);
} // end of sh_int

//////////////////////////////////////////////////////////////////
// sh_chld							                            //
// =============================================================//
// Input: signum						                        //
// Output: none							                        //
// Purpose: signal handler for SIGCHLD 				            //
//////////////////////////////////////////////////////////////////
void sh_chld(int signum)
{
    pid_t pid = wait(NULL);
    close(client_fd);
    printf("Client (%d)'s Release\n\n", pid);
    // printf("pid of killed child process : %d\n", pid);

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

//////////////////////////////////////////////////////////////////
// sh_alrm							                            //
// =============================================================//
// Input: signum						                        //
// Output: none							                        //
// Purpose: signal handler for SIGALRM				            //
//////////////////////////////////////////////////////////////////
void sh_alrm(int signum)
{
    alarm(10);

    /*  print current status */
    printf("Current Number of Client : %d\n", cli_num);
    printf("%6s%6s%6s\n", "PID", "PORT", "TIME");
    for (int i = 0; i < cli_num; i++)
        printf("%6d%6d%6ld\n", pInfo[i].pid, pInfo[i].port, time(NULL) - pInfo[i].time);
    
} // end of sh_alrm

//////////////////////////////////////////////////////////////////
// client_info							                        //
// =============================================================//
// Input: client_addr						                    //
// Output: 0 success						                    //
// Purpose: display client ip and port				            //
//////////////////////////////////////////////////////////////////
int client_info(struct sockaddr_in *client_addr)
{
    char buf[MAX_BUFF];
    write(STDOUT_FILENO, "\n==========Client info==========\n", 34);
    sprintf(buf, "client IP: %s\n", inet_ntoa(client_addr->sin_addr));
    write(STDOUT_FILENO, buf, strlen(buf));
    sprintf(buf, "\nclient port: %d\n", client_addr->sin_port);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, "===============================\n", 33);

    return 0;
} // end of client_info()

//////////////////////////////////////////////////////////////////
// cmd_process								                    //
// =============================================================//
// Input: buff, result_buff							            //
// Output: 0 success                                            //
//        -1 fail						                        //
// Purpose: command execute						                //
//////////////////////////////////////////////////////////////////
int cmd_process(char *buff, char *result_buff)
{
    char error[128];
    strcpy(error, "Error: system call\n");

    memset(result_buff, '\0', sizeof(result_buff));

    char getStr[MAX_BUFF]; // FTP command
    strcpy(getStr, buff);

    char *cmd = strtok(buff, " \n");
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

                char buf[MAX_BUFF];
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
                        char buf[MAX_BUFF];
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
                        char buf[MAX_BUFF];
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
                        char buf[MAX_BUFF];
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
                        char buf[MAX_BUFF];
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
        {
            strcpy(result_buff, dirname);
            strcat(result_buff, "\n");
        }
    }
    /////////////////// cd ////////////////////
    else if (strcmp(cmd, "CWD") == 0)
    {
        if (chdir(argu1) < 0) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }

        strcpy(result_buff, "cd success\n");
    }
    /////////////////// cd .. ////////////////////
    else if (strcmp(cmd, "CDUP") == 0)
    {
        if (chdir("..") < 0) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }

        strcpy(result_buff, "cd .. success\n");
    }
    /////////////////// mkdir ////////////////////
    else if (strcmp(cmd, "MKD") == 0)
    {
        while (argu1)
        {
            if (mkdir(argu1, 0755) == -1) // exception handling, default permission
            {
                char buf[MAX_BUFF];
                sprintf(buf, "mkdir: cannot create directory ‘%s’: File exists\n", argu1);
                strcat(result_buff, buf);
                write(STDOUT_FILENO, buf, strlen(buf));
            }
            argu1 = strtok(NULL, " ");
        }
    }
    /////////////////// rmdir ////////////////////
    else if (strcmp(cmd, "RMD") == 0)
    {
        while (argu1)
        {
            if (rmdir(argu1) == -1) // exception handling, default permission
            {
                char buf[MAX_BUFF];
                sprintf(buf, "rmdir: failed to remove '%s': No such file or directory\n", argu1);
                strcat(result_buff, buf);
                write(STDOUT_FILENO, buf, strlen(buf));
            }
            argu1 = strtok(NULL, " ");
        }
    }
    /////////////////// delete ////////////////////
    else if (strcmp(cmd, "DELE") == 0)
    {
        if (unlink(argu1) == -1) // exception handling, default permission
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }

        strcpy(result_buff, "delete success\n");
    }
    /////////////////// rename ////////////////////
    else if (strcmp(cmd, "RNFR") == 0)
    {
        char *argu2 = strtok(NULL, " ");
        char *argu3 = strtok(NULL, " ");

        struct stat buf;
        if (!lstat(argu3, &buf))
        {
            if (S_ISDIR(buf.st_mode))
                write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }
        else if (rename(argu1, argu3) == -1) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            return -1;
        }

        strcpy(result_buff, "rename success\n");
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

                char buf[MAX_BUFF];
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
                    char buf[MAX_BUFF];
                    sprintf(buf, "\n%s:\n", dir[i]);
                    strcat(result_buff, buf);
                    print_pwd_in_detail(dir[i], result_buff);
                }
            }
        }
        return 0;
    }
    /////////////////// quit ////////////////////
    else if (strcmp(cmd, "QUIT") == 0)
    {
        strcpy(result_buff, "QUIT");
        return 0;
    }
    else
        return -1;

    return 0;

} // end of cmd_process()

//////////////////////////////////////////////////////////
// print_pwd_except_hidden_file(ls)						//
// =====================================================//
// Input: pathname (directory name)			            //
// Purpose: read the all files and directories in	    //
// 	    working directory	(excluding hidden file)		//
//////////////////////////////////////////////////////////
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
            char buf[MAX_BUFF];
            if (lb && (lb % 5 == 0))
                strcat(result_buff, "\n");
            sprintf(buf, "%-10s", fileName[i]);
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
        char buf[MAX_BUFF];
        if (lb && (lb % 5 == 0))
            strcat(result_buff, "\n");
        sprintf(buf, "%-10s", fileName[i]);
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
// print_pwd_in_detail_except_hidden_file()(ls -l)	    //
// =====================================================//
// Input: pathname (directory name),                    //
//        result_buff (processed result)			    //
// Purpose: read the all files and directories in	    //
// 	    working directory in detail			            //
//		(excluding hidden file)			                //
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
