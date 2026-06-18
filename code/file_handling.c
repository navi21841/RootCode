#include <stdlib.h>
#include <stdio.h>

int file_exists(char *filename){
    FILE *file;
    file = fopen(filename, "r");

    if (file != NULL)
    {
        fclose(file);

        char answer;
        printf("File already exists. Want to overwrite? (y/n): ");
        scanf(" %c", &answer);

        if (answer == 'Y' || answer == 'y')
        {
            return 1;   // overwrite
        }
        else 
        {
            return -1;  // don't overwrite
        }
    }
    else
    {
        return 2;       // file does not exist
    }
}

void file_saving(char *filename, char buffer[][1024], int total_lines, int line_length[]){
    int a = file_exists(filename);
    if (a == 1)
    {
        FILE *file;
        file = fopen(filename, "w");
        for (int i = 0; i < total_lines; i++)
        {
            fwrite(buffer[i], 1, line_length[i], file);
            fputc('\n', file);
        }
        fclose(file);
        
    } else if (a == -1)
    {
        printf("I dont know what to do?");
    } else if (a == 2)
    {
        FILE *file;
        file = fopen(filename, "w");
        for (int i = 0; i < total_lines; i++)
        {
            fwrite(buffer[i], 1, line_length[i], file);
            fputc('\n', file);
        }
        fclose(file);
    }
}