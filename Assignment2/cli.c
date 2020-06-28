//////////////////////////////////////////////////////////
// File Name	: cli.c					//
// Date		: 2020/05/07				//
// OS		: Ubuntu 16.04.5 LTS 64bits		//
// Author	: Park Tae Sung				//
// Student ID	: 2015722031				//
// ---------------------------------------------------- //
// Title: System Programming Assignment #2 (cli)	//
// Description: convert user command to FTP commands	//
//////////////////////////////////////////////////////////
#include <string.h> // strcmp(), strcpy(), strcat()
#include <unistd.h> // write()
#include <stdlib.h> // exit()
#define MAX_SIZE 256
#define BUF_SIZE 1024

//////////////////////////////////////////////////////////////////
// Main								//
// =============================================================//
// Input: argv							//
// Output: 0 success						//
// Purpose: Main						//
//////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	char buf[128]; // converted command

	char error[128];
	strcpy(error, "Error: another argument is required\n");

	///// check arguments /////
	if (argc < 2) // Error: command does not exist
	{
		write(STDOUT_FILENO, "Error: another argument is required\n", strlen("Error: another argument is required\n"));
		exit(-1);
	}

	///// change USER command to FTP command /////
	if (strcmp(argv[1], "ls") == 0) // ls
		strcpy(buf, "NLST");
	else if (strcmp(argv[1], "dir") == 0) // dir
		strcpy(buf, "LIST");
	else if (strcmp(argv[1], "pwd") == 0) // pwd
		if (argc > 2)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else 
			strcpy(buf, "PWD");
	else if (strcmp(argv[1], "cd") == 0) // cd
		if (argc < 3)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else if (strcmp(argv[2], "..") == 0) // cd..
			if(argc > 3)
			{
				write(STDOUT_FILENO, error, strlen(error));
				exit(-1);
			}
			else
				strcpy(buf, "CDUP");
		else
			strcpy(buf, "CWD");
	else if (strcmp(argv[1], "mkdir") == 0) // mkdir
		if (argc < 3)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else
			strcpy(buf, "MKD");
	else if (strcmp(argv[1], "delete") == 0) // delete
		if (argc < 3)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else
			strcpy(buf, "DELE");
	else if (strcmp(argv[1], "rmdir") == 0) // rmdir
		if (argc < 3)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else
			strcpy(buf, "RMD");
	else if (strcmp(argv[1], "rename") == 0) // rename
		if (argc != 4)
		{
			write(STDOUT_FILENO, error, strlen(error));
			exit(-1);
		}
		else
			strcpy(buf, "RNFR");
	else if (strcmp(argv[1], "quit") == 0) // quit
		strcpy(buf, "QUIT");
	else{
		write(STDOUT_FILENO, error, strlen(error));
		exit(-1); // Error: invalid command
	}

	///// combine FTP command and optional argument /////
	if (argc == 3 && strcmp(argv[2], "..") != 0)
	{
		strcat(buf, " ");
		strcat(buf, argv[2]);
	}
	else if (argc > 3 && strcmp(argv[2], "..") != 0)
	{
		if(strcmp(buf, "RNFR") == 0) // rename
		{
			strcat(buf, " ");
			strcat(buf, argv[2]);
			strcat(buf, "\n");
			strcat(buf, "RNTO");
			strcat(buf, " ");
			strcat(buf, argv[3]);
		}
		else // mkdir, rmdir
			for (int i = 2; i < argc; i++)
			{
				strcat(buf, " ");
				strcat(buf, argv[i]);
			}
	}

	///// check all inputs for exception handling /////
	char getStr[BUF_SIZE]; // FTP command
	char temp[BUF_SIZE];
	strcpy(getStr, buf);
	strcpy(temp, buf);

    ///// execute function /////
    char *cmd = strtok(getStr, " ");
    char *argu1 = strtok(NULL, " \n");
	opterr = 0;

    /////////////////// ls ////////////////////
    if (strcmp(cmd, "NLST") == 0)
    {
        int aflag = 0, lflag = 0;
        int c;

        ///// ls: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(temp, " ");

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
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "dir") == 0) // dir
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
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "pwd") == 0) // pwd
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
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "cd") == 0) // cd
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
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "mkdir") == 0) // mkdir
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

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // mkdir: wrong option
				write(STDOUT_FILENO, "mkdir: invalid option\n", strlen("mkdir: invalid option\n"));
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "delete") == 0) // delete
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

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // delete: wrong option
				write(STDOUT_FILENO, "delete: invalid option\n", strlen("delete: invalid option\n"));
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "rmdir") == 0) // rmdir
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

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // rmdir: wrong option
				write(STDOUT_FILENO, "rmdir: invalid option\n", strlen("rmdir: invalid option\n"));
                exit(-1);
                break;
            default:
                break;
            }
        }
	}
	else if (strcmp(argv[1], "rename") == 0) // rename
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

        while ((c = getopt(fargc, fargv, " ")) != -1)
        {
            switch (c)
            {
            case '?': // rename: wrong option
				write(STDOUT_FILENO, "rename: invalid option\n", strlen("rename: invalid option\n"));
                exit(-1);
                break;
            default:
                break;
            }
        }

	}
	else if (strcmp(argv[1], "quit") == 0) // quit
    {
        int c;

        ///// quit: parsing program execution option /////
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
            case '?': // quit: wrong option
				write(STDOUT_FILENO, "quit: invalid option\n", strlen("quit: invalid option\n"));
                exit(-1);
                break;
            default:
                break;
            }
        }

		///// check arguments /////
		if (argc > 2) // Error: command does not exist
		{
			write(STDOUT_FILENO, "Error: another argument is not required\n", strlen("Error: another argument is not required\n"));
			exit(-1);
		}
	}
	else
		exit(-1); // Error: invalid command

	///// write the converted command to STDOUT /////
	write(1, buf, strlen(buf));
	write(1, "\n", 1);
	
	exit(0);
} // end of main
