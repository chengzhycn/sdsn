/**
 * ****************************************************************************
 *  > File Name: configParser.h
 *  > Author: chengzhycn
 *  > Mail: chengzhycn@gmail.com 
 *  > Created Time: Tue 05 Apr 2016 12:33:33 PM CST
 ******************************************************************************
 */

#ifndef _CONFIGPARSER_H_
#define _CONFIGPARSER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* New types -----------------------------------------------------------------*/

/**
 * @brief:  Structure that store node information
 *
 */

typedef struct _nodeInfo_ {
    int         dstNum;
    char    **  dstName;
    char    **  dstEid;
    char    **  dstIp;
    unsigned *  hash;
}nodeInfo;

typedef enum{
    true    = 1,
    false   = 0
}bool;


/* Function prototypes -------------------------------------------------------*/

/**
 * @brief:  Duplicate node config from config file
 * @param:  num Numbers of nodes
 * @param:  pwd Config file's path
 * @return: Pointer to the nodeInfo structure stored the node information
 */

nodeInfo * configParser(int num, char * pwd);


#endif
