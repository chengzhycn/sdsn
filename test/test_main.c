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

int main(int argc, char **argv)
{
    nodeInfo * n;
    char * pwd ="example.config";
    int num = 2;

    n = configParser(num, pwd);
    if (n == NULL){
        fprintf(stdout, "error!!!!");
        exit(1);
    }

    for (int i = 0; i < num; i++){
        fprintf (stdout, "%s, %s, %s\n", n->dstName[i], n->dstEid[i], n->dstIp[i]);
    }
    return 0;
}
