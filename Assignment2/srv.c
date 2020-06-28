//////////////////////////////////////////////////////////
// File Name	: srv.c					//
// Date		: 2020/05/07				//
// OS		: Ubuntu 16.04.5 LTS 64bits		//
// Author	: Park Tae Sung				//
// ---------------------------------------------------- //
// Title: System Programming Assignment #2 (srv)	//
// Description: check FTP commands and execute commands //
//////////////////////////////////////////////////////////
#include <unistd.h>   // read(), write(), unlink(), getopt(), getcwd()
#include <stdlib.h>   // exit()
#include <string.h>   // strlen(), strcpy(), strtok(), strcmp()
#include <sys/stat.h> // mkdir()
#include <stdio.h>    // sprintf()
#include <dirent.h>   // rewinddir(), opendir(), closedir(), readdir()
#include <sys/stat.h> // lstat(), stat()
#include <pwd.h>	  // passwd
#include <grp.h>	  // group
#include <time.h>	  // ctime()
#define BUF_SIZE 1024
#define MAX_SIZE 256
void print_pwd_except_hidden_file(char *pathname); // ls
void print_pwd(char *pathname);                    // ls -a
void print_pwd_in_detail_except_hidden_file(char *pathname); // ls -l
void print_pwd_in_detail(char *pathname); // ls -al

//////////////////////////////////////////////////////////
// main()						//
// =====================================================//
// Output: 0 success (termination)			//
// Purpose: Main					//
//////////////////////////////////////////////////////////
int main()
{

    char buf[BUF_SIZE] = {
        0,
    }; // buffer to read the file

    char getStr[BUF_SIZE]; // FTP command

    char error[128];
    strcpy(error, "Error: system call\n");

    char result[200];

    ///// check STANDARD INPUT /////
    if (read(STDIN_FILENO, buf, BUF_SIZE) == -1)
        exit(-1);
    strcpy(getStr, buf);
    buf[strlen(buf) - 1] = '\0';

    ///// execute function /////
    char *cmd = strtok(buf, " ");
    char *argu1 = strtok(NULL, " \n");

    /////////////////// ls ////////////////////
    if (strcmp(cmd, "NLST") == 0)
    {
        int aflag = 0, lflag = 0, errOptflag = 0;
        int c;

        char *regu[MAX_SIZE];
        char *dir[MAX_SIZE];
        ///// display command /////
//        if (write(STDOUT_FILENO, getStr, strlen(getStr)) == -1)
//            exit(-1);

        ///// ls: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(getStr, " ");

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

                char buf[BUF_SIZE];
                sprintf(buf, "ls: cannot access '%s' : No such file or directory\n", fargv[i]);
                write(STDOUT_FILENO, buf, strlen(buf));
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
                    char *temp = regu[j];
                    dir[j] = dir[j + 1];
                    dir[j + 1] = temp;
                }
            }
        }

        ///// ls: execute function /////

        ///// ls: no option /////
        if (!aflag && !lflag && !errOptflag)
            if (!reguInd && !dirInd && !errArguflag) // ls
                print_pwd_except_hidden_file(".");
            else // ls argu
            {
                if (reguInd == 1 && dirInd == 0)
                {
                    print_pwd_except_hidden_file(regu[0]);
                    write(STDOUT_FILENO, "\n", 1);
                }
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd_except_hidden_file(dir[0]);
                else
                {
                    int lb = 0; // line break
                    for (int i = 0; i < reguInd; i++)
                    {
                        if (lb && (lb % 5 == 0))
                            write(STDOUT_FILENO, "\n", 1);
                        print_pwd_except_hidden_file(regu[i]);
                        lb++;
                    }

                    if (!errArguflag)
                        write(STDOUT_FILENO, "\n", 1);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[BUF_SIZE];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        write(STDOUT_FILENO, buf, strlen(buf));
                        print_pwd_except_hidden_file(dir[i]);
                    }
                }
            }

        ///// ls: option a /////
        else if (aflag && !lflag)
            if (!reguInd && !dirInd && !errArguflag) // ls -a
                print_pwd(".");
            else // ls -a argu
            {
                if (reguInd == 1 && dirInd == 0)
                {
                    print_pwd(regu[0]);
                    write(STDOUT_FILENO, "\n", 1);
                }
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd(dir[0]);
                else
                {
                    int lb = 0; // line break
                    for (int i = 0; i < reguInd; i++)
                    {
                        if (lb && (lb % 5 == 0))
                            write(STDOUT_FILENO, "\n", 1);
                        print_pwd(regu[i]);
                        lb++;
                    }

                    if (!errArguflag)
                        write(STDOUT_FILENO, "\n", 1);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[BUF_SIZE];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        write(STDOUT_FILENO, buf, strlen(buf));
                    }
                }
            }

        ///// ls: option l /////
        else if (!aflag && lflag)
            if (!reguInd && !dirInd & !errArguflag) // ls -l
                print_pwd_in_detail_except_hidden_file(".");
            else // ls -l argu
            {
                if (reguInd == 1 && dirInd == 0)
                    print_pwd_in_detail_except_hidden_file(regu[0]);
                else if (reguInd == 0 && dirInd == 1)
                     print_pwd_in_detail_except_hidden_file(dir[0]);
                else
                {
                    for (int i = 0; i < reguInd; i++)
                        print_pwd_in_detail_except_hidden_file(regu[i]);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[BUF_SIZE];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        write(STDOUT_FILENO, buf, strlen(buf));
                        print_pwd_in_detail_except_hidden_file(dir[i]);
                    }
                }
            }

        ///// ls: option al /////
        else if (aflag && lflag)
            if (!reguInd && !dirInd & !errArguflag) // ls -al
                print_pwd_in_detail(".");
            else // ls -al argu
            {
                if (reguInd == 1 && dirInd == 0)
                    print_pwd_in_detail(regu[0]);
                else if (reguInd == 0 && dirInd == 1)
                    print_pwd_in_detail(dir[0]);
                else
                {
                    for (int i = 0; i < reguInd; i++)
                        print_pwd_in_detail(regu[i]);

                    for (int i = 0; i < dirInd; i++)
                    {
                        char buf[BUF_SIZE];
                        sprintf(buf, "\n%s:\n", dir[i]);
                        write(STDOUT_FILENO, buf, strlen(buf));
                        print_pwd_in_detail(dir[i]);
                    }
                }
            }
        exit(0);
    }
    /////////////////// pwd ////////////////////
    else if (strcmp(cmd, "PWD") == 0)
    {
        char dirname[200]; // buffer to hold working directory string

        if (getcwd(dirname, 200) == NULL) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            exit(-1);
        }
        else
            strcpy(result, dirname);
    }
    /////////////////// cd ////////////////////
    else if (strcmp(cmd, "CWD") == 0)
    {
        if (chdir(argu1) < 0) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            exit(-1);
        }
    }
    /////////////////// cd .. ////////////////////
    else if (strcmp(cmd, "CDUP") == 0)
    {
        if (chdir("..") < 0) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            exit(-1);
        }
    }
    /////////////////// mkdir ////////////////////
    else if (strcmp(cmd, "MKD") == 0)
    {
        while (argu1)
        {
            if (mkdir(argu1, 0755) == -1) // exception handling, default permission
            {
                write(STDOUT_FILENO, error, strlen(error));
                exit(-1);
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
                write(STDOUT_FILENO, error, strlen(error));
                exit(-1);
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
            exit(-1);
        }
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
				exit(-1);
        }
        else if (rename(argu1, argu3) == -1) // exception handling
        {
            write(STDOUT_FILENO, error, strlen(error));
            exit(-1);
        }
    }
    /////////////////// dir ////////////////////
    else if (strcmp(cmd, "LIST") == 0)
    {
        
        char *regu[MAX_SIZE];
        char *dir[MAX_SIZE];
        ///// display command /////
//        if (write(STDOUT_FILENO, getStr, strlen(getStr)) == -1)
//            exit(-1);

        ///// dir: parsing program execution option /////
        char *fargv[MAX_SIZE];
        int fargc = 0;

        fargv[fargc] = strtok(getStr, " ");

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

                char buf[BUF_SIZE];
                sprintf(buf, "ls: cannot access '%s' : No such file or directory\n", fargv[i]);
                write(STDOUT_FILENO, buf, strlen(buf));
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
                    char *temp = regu[j];
                    dir[j] = dir[j + 1];
                    dir[j + 1] = temp;
                }
            }
        }
            
        if (!reguInd && !dirInd & !errArguflag) // ls -al
            print_pwd_in_detail(".");
        else // ls -al argu
        {
            if (reguInd == 1 && dirInd == 0)
                print_pwd_in_detail(regu[0]);
            else if (reguInd == 0 && dirInd == 1)
                print_pwd_in_detail(dir[0]);
            else
            {
                for (int i = 0; i < reguInd; i++)
                    print_pwd_in_detail(regu[i]);

                for (int i = 0; i < dirInd; i++)
                {
                    char buf[BUF_SIZE];
                    sprintf(buf, "\n%s:\n", dir[i]);
                    write(STDOUT_FILENO, buf, strlen(buf));
                    print_pwd_in_detail(dir[i]);
                }
            }
        }
        exit(0);
    }
    /////////////////// quit ////////////////////
    else if (strcmp(cmd, "QUIT") == 0)
        strcpy(result, "quit success");
    else
        exit(-1);

    ///// display result /////
//    if (write(STDOUT_FILENO, getStr, strlen(getStr)) == -1)
//        exit(-1);

    if (strcmp(cmd, "QUIT") == 0 || strcmp(cmd, "PWD") == 0)
    {
        char lf = '\n'; // line feed
        if (write(STDOUT_FILENO, result, strlen(result)) == -1)
            exit(-1);
        if (write(STDOUT_FILENO, &lf, 1) == -1)
            exit(-1);
    }

    exit(0);
} // end of main()

//////////////////////////////////////////////////////////
// print_pwd_except_hidden_file(ls)						//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory	(excluding hidden file)			//
//////////////////////////////////////////////////////////
void print_pwd_except_hidden_file(char *pathname)
{
	DIR *dir = opendir(pathname); // opens a directory stream
	struct dirent *dp;
	int dpInd = 0;

	if (!dir) // file
	{
		write(STDOUT_FILENO, pathname, strlen(pathname));
		write(STDOUT_FILENO, " ", 1);
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
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
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));
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
            char buf[BUF_SIZE];
			if (lb && (lb % 5 == 0))
				write(STDOUT_FILENO, "\n", 1);
			sprintf(buf, "%-10s", fileName[i]);
            write(STDOUT_FILENO, buf, strlen(buf));
			lb++;
		}
	}

	if (dpInd > 2)
		write(STDOUT_FILENO, "\n", 1);

	if (closedir(dir)) // close the directory stream
        write(STDOUT_FILENO, "closed error !!!\n", strlen("closed error !!!\n"));
} // end of print_pwd_except_hidden_file()

//////////////////////////////////////////////////////////
// print_pwd()(ls -a)					//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory	(including hidden file)	//
//////////////////////////////////////////////////////////
void print_pwd(char *pathname)
{
	DIR *dir = opendir(pathname); // opens a directory stream
	struct dirent *dp;
	int dpInd = 0;

	if (!dir) // file
	{
		write(STDOUT_FILENO, pathname, strlen(pathname));
		write(STDOUT_FILENO, " ", 1);
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
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
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));
			continue;
		}
		if (S_ISDIR(buf.st_mode))
			strcat(fileName[i], "/");
	}

	///// print all files and directories in directory /////
	int lb = 0; // line break
	for (int i = 0; i < dpInd; i++)
	{
        char buf[BUF_SIZE];
		if (lb && (lb % 5 == 0))
			write(STDOUT_FILENO, "\n", 1);
		sprintf(buf, "%-10s", fileName[i]);
        write(STDOUT_FILENO, buf, strlen(buf));
		lb++;
	}
	write(STDOUT_FILENO, "\n", 1);

	if (closedir(dir)) // close the directory stream
        write(STDOUT_FILENO, "closed error !!!\n", strlen("closed error !!!\n"));
} // end of print_pwd()

//////////////////////////////////////////////////////////
// print_pwd_in_detail()(ls -al)			//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory in detail			//
//	    (including hidden file)			//
//////////////////////////////////////////////////////////
void print_pwd_in_detail(char *pathname)
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
			}
			strcat(dirname, "/");
			strcat(dirname, pathname);
		}
		else // absolute path
			strcpy(dirname, pathname);

		if (lstat(dirname, &buf) < 0)
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));

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

		write(STDOUT_FILENO, list, strlen(list));
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
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
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));
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

		write(STDOUT_FILENO, list, strlen(list));
	}

	if (closedir(dir)) // close the directory stream
		write(STDOUT_FILENO, "closed error !!!\n", strlen("closed error !!!\n"));
} // end of print_pwd_in_detail()

//////////////////////////////////////////////////////////
// print_pwd_in_detail_except_hidden_file()(ls -l)	//
// =====================================================//
// Input: pathname (directory name)			//
// Purpose: read the all files and directories in	//
// 	    working directory in detail			//
//		(excluding hidden file)			//
//////////////////////////////////////////////////////////
void print_pwd_in_detail_except_hidden_file(char *pathname)
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
			}
			strcat(dirname, "/");
			strcat(dirname, pathname);
		}
		else // absolute path
			strcpy(dirname, pathname);

		if (lstat(dirname, &buf) < 0)
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));

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

		write(STDOUT_FILENO, list, strlen(list));
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
				write(STDOUT_FILENO, "getcwd error !!!\n", strlen("getcwd error !!!\n"));
				exit(-1);
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
			write(STDOUT_FILENO, "lstat error !!!\n", strlen("getcwd error !!!\n"));
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
        
		write(STDOUT_FILENO, list, strlen(list));
	}

	if (closedir(dir)) // close the directory stream
		write(STDOUT_FILENO, "closed error !!!\n", strlen("closed error !!!\n"));
} // end of print_pwd_in_detail_except_hidden_file()

