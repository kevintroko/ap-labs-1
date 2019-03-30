#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/** --USEFUL --
https://www.tldp.org/LDP/Linux-Filesystem-Hierarchy/html/proc.html
man proc command
/proc/<PID>/status per-process file
*/

struct proceso
{
    char name[45];
    char state[45];
    char pid[45];
    char p_pid[45];
    char memory[45];
    char threads[45];
};

// Struct for process
static const struct proceso empty_process;
struct proceso processes[500];

void clear();
int read_process();
int print_process();
static void save_process();

int main()
{
    // Place your magic here
    if (signal(SIGINT, save_process) == SIG_ERR) {
        // Not magic
        printf("Error at Signal Handling\n");
    }
    while (1) {   
        // MAGIC
        read_process();
        print_process();
        sleep(3);
        clear();
    }
    return 0;
}

// Clear Function
void clear() {
    for (int x = 0; processes[x].name[0] != '\0'; x++)
    {
        processes[x] = empty_process;
    }
    printf("\e[1;1H\e[2J");
}

// Save proceso
static void save_process() {
    char *header = "|%-6s | %-5s | %-25s | %-10s | %-10s | %-4s |\n";
    char *formato = "| %-5s | %-6s | %-25s | %-10s | %-9.1fM | %-9s |\n";
    char *headDiv = "|-------|--------|---------------------------|------------|------------|-----------|\n";
    FILE *fp;
    time_t t = time(NULL);
    float memo;
    char fn[30];

    // Name of file
    sprintf(fn, "mytop.txt");
    fp = fopen(fn, "w");
    fprintf(fp, header, " Pid", "Parent", "Name", "State", "Memory", "# Threads");
    fprintf(fp, "%s", headDiv);

    for (int i = 0; processes[i].name[0] != '\0'; i++) {
        memo = atof(processes[i].memory) / 1000;
        fprintf(fp, formato, 
                processes[i].pid, 
                processes[i].p_pid, 
                processes[i].name, 
                processes[i].state, 
                memo, 
                processes[i].threads);
    }
    printf("\nsaved to --> %s\n", fn);
    fclose(fp);
    sleep(5);
}

// Read proceso
int read_process() {
    DIR *d = opendir("/proc/");
    struct dirent *dir;
    FILE *fp;
    char path[30],
        fdpath[30],
        data[45],
        c;
    int index,
        procNum = 0;

    memset(data, 0, 45);
    strcpy(path, "/proc/");
    strcpy(fdpath, "/proc/");

    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_name[0] >= 48 && dir->d_name[0] <= 57)
        {
            index = 0;
            strcat(path, dir->d_name);
            strcat(path, "/status");
            strcat(fdpath, dir->d_name);
            strcat(fdpath, "/fd");
            fp = fopen(path, "r");

            while (index < 6)
            {
            switch (index) {
            case 0:
                do {
                    c = getc(fp);
                } while (c != 'N' && c != EOF);

                if (c == EOF) {
                    fseek(fp, 0L, SEEK_SET);
                    strcpy(processes[procNum].name, "");
                    break;
                }
                // Name:
                if (getc(fp) == 'a' && 
                    getc(fp) == 'm' && 
                    getc(fp) == 'e' && 
                    getc(fp) == ':') {
                    do {
                        c = getc(fp);
                    } while (c == ' ' || c == '\t');

                    for (int j = 0; c != '\n'; j++) {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].name, data);
                    fseek(fp, 0L, SEEK_SET);
                }
                else
                {
                    index = -1;
                }
                break;
            case 1:
                do
                {
                    c = getc(fp);
                } while (c != 'S' && c != EOF);

                if (c == EOF)
                {
                    fseek(fp, 0L, SEEK_SET);
                    break;
                }

                // State
                if (getc(fp) == 't' && 
                    getc(fp) == 'a' && 
                    getc(fp) == 't' && 
                    getc(fp) == 'e' && 
                    getc(fp) == ':') {
                    do
                    {
                        c = getc(fp);
                    } while (c != '(');
                    c = getc(fp);
                    for (int j = 0; c != ')'; j++)
                    {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].state, data);

                    fseek(fp, 0L, SEEK_SET);
                }
                else
                {
                    index = 0;
                }
                break;
            case 2:
                do
                {
                    c = getc(fp);
                } while (c != 'P' && c != EOF);

                if (c == EOF)
                {
                    fseek(fp, 0L, SEEK_SET);
                    break;
                }

                // Pid:
                if (getc(fp) == 'i' && 
                    getc(fp) == 'd' && 
                    getc(fp) == ':')
                {
                    do
                    {
                        c = getc(fp);
                    } while (c == ' ' || c == '\t');

                    for (int j = 0; c != '\n'; j++)
                    {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].pid, data);
                    fseek(fp, 0L, SEEK_SET);
                }
                else
                {
                    index = 1;
                }
                break;
            case 3:
                do
                {
                    c = getc(fp);
                } while (c != 'P' && c != EOF);

                if (c == EOF)
                {
                    fseek(fp, 0L, SEEK_SET);
                    break;
                }

                // PPid: Parent id
                if (getc(fp) == 'P' && 
                    getc(fp) == 'i' && 
                    getc(fp) == 'd' && 
                    getc(fp) == ':') {
                    do
                    {
                        c = getc(fp);
                    } while (c == ' ' || c == '\t');

                    for (int j = 0; c != '\n'; j++)
                    {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].p_pid, data);
                    fseek(fp, 0L, SEEK_SET);
                }
                else
                {
                    index = 2;
                }
                break;
            case 4:
                do
                {
                    c = getc(fp);
                } while (c != 'V' && c != EOF);
                if (c == EOF)
                {
                    fseek(fp, 0L, SEEK_SET);
                    break;
                }

                // VmRSS:
                if (getc(fp) == 'm' && 
                    getc(fp) == 'R' && 
                    getc(fp) == 'S' && 
                    getc(fp) == 'S' && 
                    getc(fp) == ':')
                {
                    do
                    {
                        c = getc(fp);
                    } while (c == ' ' || c == '\t');

                    for (int j = 0; c != ' '; j++)
                    {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].memory, data);

                    fseek(fp, 0L, SEEK_SET);
                }
                else
                {
                    index = 3;
                }
                break;
            case 5:
                do
                {
                    c = getc(fp);
                } while (c != 'T' && c != EOF);

                if (c == EOF)
                {
                    fseek(fp, 0L, SEEK_SET);
                    break;
                }

                // Checking to see if what was found was the tag "Threads"
                if (getc(fp) == 'h' && 
                    getc(fp) == 'r' && 
                    getc(fp) == 'e' && 
                    getc(fp) == 'a' && 
                    getc(fp) == 'd' && 
                    getc(fp) == 's' && 
                    getc(fp) == ':')
                {

                    do
                    {
                        c = getc(fp);
                    } while (c == ' ' || c == '\t');

                    if (c == '\n')
                    {
                        break;
                    }

                    for (int j = 0; c != '\n'; j++)
                    {
                        data[j] = c;
                        c = getc(fp);
                    }
                    strcpy(processes[procNum].threads, data);
                }
                else
                {
                    index = 4;
                }
                break;
            }
            memset(data, 0, 45);
            index++;
            c = '\0';
            }

            fclose(fp);
            strcpy(fdpath, "/proc/");
            strcpy(path, "/proc/");
            procNum++;
        }
    }
    closedir(d);
    return 0;
}

int print_process()
{
    char *header = "|%-6s | %-5s | %-25s | %-10s | %-10s | %-4s |\n";
    char *formato = "| %-5s | %-6s | %-25s | %-10s | %-9.1fM | %-9s |\n";
    char *headDiv = "|-------|--------|---------------------------|------------|------------|-----------|\n";
    float memo;
    printf(header, " Pid", "Parent", "Name", "State", "Memory", "# Threads");
    printf("%s", headDiv);

    int i = 0;
    while (processes[i].name[0] != '\0')
    {
        memo = atof(processes[i].memory) / 1000;
        printf(formato, 
               processes[i].pid, 
               processes[i].p_pid, 
               processes[i].name, 
               processes[i].state, 
               memo, 
               processes[i].threads);
        i++;
    }
    return 0;
}

