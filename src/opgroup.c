#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>

#define INFILE "vdb.data"

int count_records()
{
    FILE* file = fopen(INFILE, "r");
    if (!file) return -1;

    char* line = NULL;
    size_t len = 0;
    int result = 0;

    while ((getline(&line, &len, file)) != -1)
    {
        if (strcmp("[OLTVA]", line, 7))
            ++result;
    }

    fclose(file);
    free(line);

    return result;
}

int mark_vaccinated(int n)
{
    FILE *datafile = fopen(INFILE, "r");
    if (!datafile)
        return 1;

    FILE *tempfile = fopen(".temp.data", "w");
    if (!tempfile)
        return 2;

    char *line = NULL;
    size_t len = 0;
    unsigned int idx = 0;

    while ((getline(&line, &len, datafile)) != -1)
    {
        if (idx == n)
        {
            fprintf(tempfile, "[OLTVA]\t%s", line);
        }
        else
        {
            fprintf(tempfile, "%s", line);
        }
        ++idx;
    }

    free(line);
    fclose(datafile);
    fclose(tempfile);

    remove(INFILE);
    rename(".temp.data", INFILE);

    return 0;
}

// signalok
void READY(int sig) { printf("A busz indulásra kész\n"); }
void DONE(int sig) { printf("Sikeres oltás\n"); }

int main()
{
    int count;

    int pipe_bus1[2];
    if (pipe(pipe_bus1) == -1)
    {
        printf("Hiba: sikertelen pipe (1)\n");
        return 1;
    }

    int pipe_bus2[2];
    if (pipe(pipe_bus2) == -1)
    {
        printf("Hiba: sikertelen pipe (2)\n");
        return 2;
    }

    do 
    {
        count = count_records();
        if (mark_vaccinated(2) == 2) printf("temp open failed\n");
        
        if (count < 5)
        {
            printf("5-nél kevesebb jelentkező van; nem indul busz.\n");
            return 0;
        }

        bool send_two_buses = count > 10;
        
        pid_t bus1 = fork();
        if (bus1 < 0)
        {
            printf("Hiba: sikertelen fork (1)\n");
            return 3;
        }
        else if (bus1 == 0) // child process
        {

        }
        else // parent process
        {
            if (send_two_buses)
            {
                pid_t bus2 = fork();
                if (bus2 < 0)
                {
                    printf("Hiba: sikertelen fork (1)\n");
                    return 4;
                }
                else if (bus2 == 0) // child process
                {

                }
                else goto parent_process;
            }
            else
            {
                parent_process:
                break;
            }  
        }
    }
    while ( false );// count > 4);

    return 0;
}