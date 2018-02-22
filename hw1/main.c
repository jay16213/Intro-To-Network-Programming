#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int process(char *input, char *delimeter)
{
    char *cmd = strtok(input, " \r\n");
    char *str = strtok(NULL, "\r\n");

    //set the last char to '\0'
    if(str != NULL) *(str + strlen(str)) = '\0';

    //parse and execute the command
    if(strcmp(cmd, "reverse") == 0)
    {
        if(str == NULL)
        {
            printf("Please input the string!\n");
            return 1;
        }

        int len = strlen(str);
        int i = 0;
        for(i = len - 1; i >= 0; i--)
            putchar(str[i]);
        putchar('\n');
    }
    else if(strcmp(cmd, "split") == 0)
    {
        if(str == NULL)
        {
            printf("Please input the string!\n");
            return 1; 
        }
        
        int len = strlen(delimeter);
        char *ptr = NULL;

        while(ptr = strstr(str, delimeter))
        {
            *ptr = '\0';
            printf("%s ", str);
            str = ptr + len;
        }

        if(str != NULL && strcmp(str, "\0") != 0) printf("%s", str);
        printf("\n");
    }
    else if(strcmp(cmd, "exit") == 0)
        return 0;
    else
        printf("%s: Command not found.\n", cmd);

    return 1;
}

void help()
{
    printf("\nUSAGE:\n");
    printf("./a.out <filename> <delimeter>\n\n");
    printf("Support command:\n");
    printf("1. reverse <string>\n");
    printf("2. split <string>\n");
    printf("3. exit\n");
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Please input the filename!\n");
        help();
        exit(1);
    }
    else if(argc < 3)
    {
        printf("Please input a delimeter!\n");
        help();
        exit(1);
    }

    FILE *fptr;
    fptr = fopen(argv[1], "r");
    if(fptr == NULL)
    {
        printf("The file does not exist! Please check your filename.\n");
        exit(1);
    }
    
    printf("---------------Input file %s---------------\n", argv[1]);
    char buf[1000];
    while(!feof(fptr))
    {
        if(fgets(buf, 1000, fptr) == NULL) break;
        printf("> %s", buf);

        if(!process(buf, argv[2])) break;
    }
    fclose(fptr);
    printf("------------End of input file %s------------\n", argv[1]);

    printf("------------User input------------\n");
    printf("> ");
    while(fgets(buf, 1000, stdin))
    {
        if(!process(buf, argv[2])) break;
        printf("> ");
    }
    printf("---------End of user input---------\n");
    printf("See you next time!!\n");

    return 0;
}