/**
 ******************************************************************************
 * > File:      test_main.c
 * > Author:    chengzhycn
 * > Mail:      chengzhycn@gmail.com 
 * > Date:      Wed 06 Apr 2016 02:28:44 PM CST
 * > Brief:     
 * > Copyright(C) 2016, by chengzhycn<chengzhycn@gmail.com>
 * > All rights reserved.
 ******************************************************************************
 * Change Logs:     
 * Date       Author      Notes
 * 
 * 
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configParser.h"

typedef struct {
    int tun_fd;
    int num;
    char *DTNEth;
    char *ownEid;
    char **dstEid;
}BPArg;

BPArg * bpArgNew(int num)
{
    BPArg * n;

    n = (BPArg*) calloc(1, sizeof *n);

    if (n){
        n->num = num;
        n->DTNEth = (char*) calloc(1, sizeof *n->DTNEth);
        n->ownEid = (char*) calloc(1, sizeof *n->ownEid);
        n->dstEid = (char**) calloc(num, sizeof *n->dstEid);
    }
    return n;
}

int main(int argc, char **argv)
{
    nodeInfo * n;
    char * pwd ="example.config";
    int num = 2;
    BPArg * sendArg = NULL;

    n = configParser(num, pwd);
    if (n == NULL){
        fprintf(stdout, "error!!!!");
        exit(1);
    }

    for (int i = 0; i < num; i++){
        fprintf (stdout, "%s, %s, %s\n", n->dstName[i], n->dstEid[i], n->dstIp[i]);
    }

    sendArg = bpArgNew(num);

    for (int ix =0; ix < num; ix++){
        sendArg->dstEid[ix] = n->dstEid[ix];
        printf("%s\n", sendArg->dstEid[ix]);
    }
    return 0;
}
