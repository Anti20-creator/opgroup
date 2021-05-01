#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>

#define INFILE "vdb.data"

// global egyirányú pipeok mindkét buszhoz és -tól
int pipe_to_bus1[2];
int pipe_to_bus2[2];
int pipe_from_bus1[2];
int pipe_from_bus2[2];

// ezek a flagek a signalok kezeléséhez
static bool bus_is_ready = false;
static bool bus_departs = false;

int count_records()
{
    FILE* file = fopen(INFILE, "r");
    if (!file) return -1;

    char* line = NULL;
    size_t len = 0;
    int result = 0;

    while ((getline(&line, &len, file)) != -1)
    {
        if (strncmp("[OLTVA]", line, 7))
            ++result;
    }

    fclose(file);
    free(line);

    return result;
}

/* int mark_vaccinated(const char* record)
{
    FILE *datafile = fopen(INFILE, "r");
    if (!datafile)
        return 1;

    FILE *tempfile = fopen(".temp.data", "w");
    if (!tempfile)
        return 2;

    char *line = NULL;
    size_t len = 0;
    unsigned int n = 0;

    while ((getline(&line, &len, datafile)) != -1)
    {
        if (!strcmp(line, record))
        {
            fprintf(tempfile, "[OLTVA]\t%s", line);
        }
        else
        {
            fprintf(tempfile, "%s", line);
        }
        ++n;
    }

    free(line);
    fclose(datafile);
    fclose(tempfile);

    remove(INFILE);
    rename(".temp.data", INFILE);

    return 0;
}
*/

// megjelöli a beoltott embereket az inputfájlban
int mark_vaccinated(const char* data[5])
{
    FILE *datafile = fopen(INFILE, "r");
    if (!datafile)
        return 1;

    FILE *tempfile = fopen(".temp.data", "w");
    if (!tempfile)
        return 2;

    char *line = NULL;
    size_t len = 0;
    unsigned int n = 0;

    while ((getline(&line, &len, datafile)) != -1)
    {
        printf("%d\t%s\t%s\n", n, line, data[n]);
        if (n < 5 && !strcmp(line, data[n]))
        {
            printf("Sikeres oltás: %s\n", line);
            fprintf(tempfile, "%s", line);
            // fprintf(tempfile, "[OLTVA]\t%s", line);
            ++n;
        }
        else
        {
            fprintf(tempfile, "%s", line);
        }
    }

    free(line);
    fclose(datafile);
    fclose(tempfile);

    remove(INFILE);
    rename(".temp.data", INFILE);

    return 0;
}

// signalhandler - a busz indulásra kész
void READY(int sig)
{
    bus_is_ready = true;
}

// signalhandler - a busz elindul
void GO(int sig)
{
    bus_departs = true;
}

/* void DONE(int sig)
{
    char* data = NULL;
    if (sig == SIGUSR1)
    {
        read(pipe_from_bus1[0], &data, sizeof(data));
    }
    else // SIGUSR2
    {
        read(pipe_from_bus2[0], &data, sizeof(data));
    }
    printf("Sikeres oltás: %s\n", data);
    mark_vaccinated(data);
}
*/

void DONE(int sig)
{
    // ez még úgy van megírva, hogy tömbben olvassa le piperól az eredményt
    // ezzel egyelőre ne foglalkozz, a gond a child processnél van
    char data[5][100] = {"", "", "", "", ""};
    if (sig == SIGUSR1)
    {
        read(pipe_from_bus1[0], &data, sizeof(data));
    }
    else // SIGUSR2
    {
        read(pipe_from_bus2[0], &data, sizeof(data));
    }
    mark_vaccinated(data);
}

int main() // -------------------------------------------------------------------- MAIN STARTS HERE
{
    signal(SIGRTMIN, &READY);
    signal(SIGUSR1, &DONE);
    signal(SIGUSR2, &DONE);

    int count;

    if (pipe(pipe_to_bus1) == -1)
    {
        printf("Hiba: sikertelen pipe (1.1)\n");
        return 1;
    }
    if (pipe(pipe_to_bus2) == -1)
    {
        printf("Hiba: sikertelen pipe (2.1)\n");
        return 2;
    }
    if (pipe(pipe_from_bus1) == -1)
    {
        printf("Hiba: sikertelen pipe (1.2)\n");
        return 1;
    }
    if (pipe(pipe_from_bus2) == -1)
    {
        printf("Hiba: sikertelen pipe (2.2)\n");
        return 2;
    }

    do 
    {
        count = count_records();
        
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
        else if (bus1 > 0) // parent process
        {
            if (send_two_buses)
            {
                pid_t bus2 = fork();
                if (bus2 < 0)
                {
                    printf("Hiba: sikertelen fork (2)\n");
                    return 4;
                }
                else if (bus2 > 0) goto parent_process;
                else // child process
                {
                    // TODO: busz#2
                }
            }
            else
            {
                parent_process:

                close(pipe_from_bus1[0]);
                close(pipe_from_bus1[1]);
                close(pipe_from_bus2[0]);
                close(pipe_from_bus2[1]);
                close(pipe_to_bus1[0]);
                close(pipe_to_bus2[0]);

                pause();
                if (bus_is_ready) printf("A busz indulásra kész;\n");
                printf("Az operatív törzs küldi az adatokat:\n");

                int i = 0;
                char* data_bus1[5];
                char* data_bus2[5];
                for (; i < 5; ++i)
                {
                    data_bus1[i] = calloc(100, sizeof(char));
                    data_bus2[i] = calloc(100, sizeof(char));
                }

                FILE* file = fopen(INFILE, "r");
                char* line = NULL;
                size_t len = 0;
                int n = 0;

                while(getline(&line, &len, file) != -1)
                {
                    if (strncmp(line, "[OLTVA]", 7))
                    {
                        if (n < 5)
                        {
                            printf("SMS@busz1 -> %s", line);
                            strncpy(data_bus1[n], line, 100);
                        }
                        else if (send_two_buses && n < 10)
                        {
                            printf("SMS@busz2 -> %s", line);
                            strncpy(data_bus2[n-5], line, 100);
                        }
                        ++n;
                    }
                }
                fclose(file);

                // i = 0;
                // while (i < 5)
                // {
                //     write(pipe_to_bus1[1], &data_bus1[i], 100);
                //     ++i;
                // }

                char* buf = calloc(500, sizeof(char));
                strcat(buf, data_bus1[0]);
                strcat(buf, data_bus1[1]);
                strcat(buf, data_bus1[2]);
                strcat(buf, data_bus1[3]);
                strcat(buf, data_bus1[4]);
                int buf_len = strlen(buf) + 1;
                write(pipe_to_bus1[1], &buf_len, 1);
                if (write(pipe_to_bus1[1], &buf, buf_len) == -1)
                {
                    printf("Hiba: sikertelen write (buf)\n");
                    return(10);
                }

                printf("Sikeres pipera írás\n");

                // if (write(pipe_to_bus1[1], &data_bus1, sizeof(data_bus1)) == -1)
                // {
                //     printf("Hiba: sikertelen write (parent -> pipe_to_bus1 (data))\n");
                //     return(10);
                // }
                // if (send_two_buses && write(pipe_to_bus2[1], &data_bus2, sizeof(data_bus2)) == -1)
                // {
                //     printf("Hiba: sikertelen write (parent -> pipe_to_bus2 (data))\n");
                //     return(11);
                // }

                close(pipe_to_bus1[1]);
                close(pipe_to_bus2[1]);

                kill(bus1, SIGCONT);
                wait();

                free(line);
                free(buf);
                i = 0;
                for (; i < 5; ++i)
                {
                    free(data_bus1[i]);
                    free(data_bus2[i]);
                }

            }  
        }
        else // child process
        {
            signal(SIGCONT, &GO);

            close(pipe_to_bus2[0]);
            close(pipe_to_bus2[1]);
            close(pipe_from_bus2[0]);
            close(pipe_from_bus2[1]);

            close(pipe_to_bus1[1]);
            close(pipe_from_bus1[0]);

            kill(bus1, SIGRTMIN);
            pause();
            if (bus_departs) printf("A busz útnak indul;\n");

            int buf_len = 0;
            read(pipe_to_bus1[0], &buf_len, 1);
            char* buf = calloc(buf_len, sizeof(char));
            if (read(pipe_to_bus1[0], &buf, buf_len) == -1)
            {
                printf("Error reading from pipe: %d\n", errno);
                return 12;        
            };
            printf("%s\n", buf);
            char* token = strtok(buf, '\n');
            printf("%s\n", token);

            while(token != NULL)
            {
                // data[i] = calloc(100, sizeof(char));
                // read(pipe_to_bus1[0], &data[i], 100);
                
                printf("%s\n", token);
                if (rand() % 10 == 0)
                {
                    // memcpy(data[i], "ABSENT", 100);
                    strcpy(token, "ABSENT");
                }
                token = strtok(NULL, '\0');
            }
            // if (write(pipe_from_bus1[1], &data, sizeof(data)) == -1)
            // {
            //     printf("Hiba: sikertelen write (bus1 -> pipe_from_bus1 (data))\n");
            //     return 6;
            // }
            printf("Sikeres írás pipera\n");

            close(pipe_to_bus1[0]);
            close(pipe_from_bus1[1]);

            kill(bus1, SIGUSR1);
        }
    }
    while ( false ); // count > 4);

    return 0;
}