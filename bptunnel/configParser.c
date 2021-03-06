/**
 * ****************************************************************************
 *  > File Name: configParser.c
 *  > Author: chengzhycn
 *  > Mail: chengzhycn@gmail.com 
 *  > Created Time: Mon 04 Apr 2016 12:44:36 PM CST
 *  > Functions: read destination nodes informations from config file.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "configParser.h"

/* Private define ------------------------------------------------------------*/
#define LINES       3
#define MAXLINELEN  20

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private prototypes --------------------------------------------------------*/
void        errorMessage(char * message);
nodeInfo *  nodeNew(int num);
char     *  textPreOper(char * pwd, int num);
void        readConfig(char * buf, nodeInfo * n, int num);

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief:  Print error message when encount error.
 * @param:  message Error message
 * @return: void
 */

void errorMessage(char * message){
    fprintf (stdout, "Error: %s", message);
    exit (1);
}

/**
 * @brief:  Allocate memory for a new nodeInfo structure
 * @param:  num Numbers of nodes
 * @return: Pointer to the new memory
 */

nodeInfo * nodeNew(int num)
{
    nodeInfo * n;
    
    n = (nodeInfo*) calloc(1, sizeof *n);

    if (n){
        n->dstNum   = num;
        n->dstName  = (char**) calloc(num, sizeof *n->dstName);
        n->dstEid   = (char**) calloc(num, sizeof *n->dstEid);
        n->dstIp    = (char**) calloc(num, sizeof *n->dstIp);
    }
    return n;
}

/**
 * @brief:  Read and preprocess node config info from config file
 * @param:  pwd Config file's path
 * @param:  n   Numbers of nodes
 * @return: Pointer to node information after preprocessing
 */

char * textPreOper(char * pwd, int n)
{
    char buf_i[MAXLINELEN];
    char * buf = (char *) calloc(n * LINES, MAXLINELEN);
    FILE * fp;

    if (!buf)
        errorMessage("cannot allocate memory!");

    if ((fp = fopen(pwd, "r")) == NULL)
        errorMessage("cannot open file!");

    while (!feof(fp) && fgets(buf_i, MAXLINELEN, fp) != NULL){
        if (buf_i[0] == '\n' || buf_i[0] == '#')
            continue;
        buf = strcat(buf, buf_i);
    }

    fclose(fp);
    return buf;
}

/**
 * @brief:  Duplicate node information into nodeInfo structure
 * @param:  buf Pointer to node information after preprocessing
 * @param:  n   Pointer to nodeInfo structure that node info 
 *              will be duplicate into
 * @return void
 */

void readConfig(char * buf, nodeInfo * n, int num)
{
    char    buf_o[num*LINES][MAXLINELEN];
    char  * ptr;
    //char  * str;
    //使用指针未分配空间会导致错误
    char    str[MAXLINELEN];
    int i = 0;
    int count = 0;
    int secNum = -1;
    memset(buf_o, 0, sizeof buf_o);


    //将buf逐行分割
    ptr = strtok(buf, "\n");
    while (ptr != NULL){
       //buf_o[i] = (char *)malloc(sizeof(char *));
       // if ((buf_o[i] = (char*) malloc(strlen(ptr)+1)) == NULL)
         //   errorMessage("cannot allocate memory!");
         //   ???SIGSEGV
        strcpy(buf_o[i++], ptr);
        ptr = strtok(NULL, "\n");        
        count++;
    }

    for (int ix = 0; ix < count; ix++){
        switch (buf_o[ix][0]){
            case '[':
                secNum++;
                sscanf (buf_o[ix], "%*1c%[^]]", str);
                //strcpy (n->dstName[secNum], str);
                n->dstName[secNum] = strdup(str);
                break;
            case 'e':
                sscanf (buf_o[ix], "%*[^=]=%s", str);
                //strcpy (n->dstEid[secNum], str);
                n->dstEid[secNum] = strdup(str);
                break;
            case 'i':
                sscanf (buf_o[ix], "%*[^=]=%s", str);
                //strcpy (n->dstIp[secNum], str);
                n->dstIp[secNum] = strdup(str);
                break;
            default:
                break;
        }
    }

    //for (;i > 0;)
    //    free(buf_o[--i]);
}


/**
 * @brief:  Duplicate node config from config file
 * @param:  num Numbers of nodes
 * @param:  pwd Config file's path
 * @return: n   Pointer to the nodeInfo structure stored the node information
 */

nodeInfo * configParser(int num, char * pwd)
{
    char * buffer;
    nodeInfo * n;

    if ((n=nodeNew(num)) == NULL)
        errorMessage("can not allocate memory!");

    buffer = textPreOper(pwd, num);
    readConfig(buffer, n, num);
    
    free(buffer);
    return n;
}








/*
int main(){
    nodeInfo * n;

    n = nodeNew(2);
    fprintf (stdout, "%p, %p, %p\n", n->dstName, n->dstEid, n->dstIp);
    
    n->dstName[0] = "node1";
    n->dstEid[0] = "ipn1:2";
    n->dstIp[0] = "192.168.99.1";
    fprintf (stdout, "%s, %s, %s\n", n->dstName[0], n->dstEid[0], n->dstIp[0]);
    n->dstName[1] = "node2";
    n->dstEid[1] = "ipn2:2";
    n->dstIp[1] = "192.168.99.2";
    fprintf (stdout, "%s, %s, %s\n", n->dstName[1], n->dstEid[1], n->dstIp[1]);
    return 0;
}
*/
