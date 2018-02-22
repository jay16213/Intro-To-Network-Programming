#include "header.h"

int calc(int connfd, char *input)
{
    //fprintf(stdout, "pro: %s", input);
    char *opcode = strtok(input, TOK_DEM);
    
    if(strcmp(opcode, "EXIT") == 0)
    {
        close(connfd);
        return 1;
    }

    int result = 0;
    int overflow = 0;
    unsigned int out = 0;
    if(strcmp(opcode, "ADD") == 0)
    {
        char *oprand = strtok(NULL, TOK_DEM);
        while(oprand != NULL)
        {
            int i = atoi(oprand);
            if(i >= 0)
            {
                //too large
                if((out + i) < out)
                {
                    overflow = 1;
                }
                else
                {

                    out += i;
                    result += i;
                    
                    if(overflow && result > 0)
                    {
                        out = result;
                        overflow = 0;
                    }
                }
            }
            else
            {
                if(result >= 0)
                {
                    //-
                    if(abs(i) > result)
                    {
                        overflow = 1;
                        result += i;
                    }
                    else
                    {
                        result += i;
                        out = result;
                        overflow = 0;
                    }
                }
                else//resuolt < 0 && i < 0
                {
                    result += i;
                    overflow = 1;
                }
            }

            fprintf(stdout, "%u %d\n", out, result);   
            oprand = strtok(NULL, TOK_DEM);
        }
    }
    else if(strcmp(opcode, "MUL") == 0)
    {
        result = 1;
        out = 1;
        char *oprand = strtok(NULL, TOK_DEM);
        while(oprand != NULL)
        {
            int i = atoi(oprand);
            //mul 0 = 0
            if(i == 0)
            {
                out = 0;
                overflow = 0;
                break;
            }

            if(i > 0)
            {
                if(result >= 0)
                {
                    result *= i;
                    //too large
                    if((out * i) < out && i != 0)
                    {
                        overflow = 1;
                        break;
                    }
                    else out *= i;
                    overflow = 0;
                }
                else//res < 0
                {
                    overflow = 1;
                    result *= i;
                }
            }
            else
            {
                // - * - = +
                if(result <= 0)
                {
                    overflow = 0;
                    result *= i;
                    out = result;
                }
                else//res > 0 && i < 0
                {
                    result *= i;
                    overflow = 1;
                }
            }

            out = result;
            fprintf(stdout, "%u %d\n", out, result);
            oprand = strtok(NULL, TOK_DEM);
        }
    }

    char res[MAXLINE];
    

    if(overflow)
    {
        snprintf(res, MAXLINE, "Overflowed\n");
    }
    else
        snprintf(res, MAXLINE, "%u\n", out);
    
    fprintf(stdout, "result: %s\n", res);
    write(connfd, res, strlen(res));
    memset(res, 0, MAXLINE);
    return 0;
}