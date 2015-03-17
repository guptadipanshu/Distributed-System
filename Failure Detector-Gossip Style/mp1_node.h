/**********************
*
* Progam Name: MP1. Membership Protocol.
* 
* Code authors: <Dipanshu Gupta>
*
* Current file: mp2_node.h
* About this file: Header file.
* 
***********************/

#ifndef _NODE_H_
#define _NODE_H_

#include "stdincludes.h"
#include "params.h"
#include "queue.h"
#include "requests.h"
#include "emulnet.h"
#include <stdlib.h>
 #include <time.h>
/* Configuration Parameters */
char JOINADDR[30];                    /* address for introduction into the group. */
extern char *DEF_SERVADDR;            /* server address. */
extern short PORTNUM;                /* standard portnum of server to contact. */

/* Miscellaneous Parameters */
extern char *STDSTRING;

typedef struct member{            
        struct address addr;            // my address
        int inited;                     // boolean indicating if this member is up
        int ingroup;                    // boolean indiciating if this member is in the group

        queue inmsgq;                   // queue for incoming messages
        int bfailed;                    // boolean indicating if this member has failed
        
        int neighbor[200][3];			//neighbor[i][j] i is the index of neghibor,j=0 for dead/alive,j=1  heartbeat,j=2 time revieced
		struct address neighbor_addr[200]; 
		int current_hearbeat;	
		int timeout;	// counter to current node hearbeat
} member;

/* Message types */
/* Meaning of different message types
  JOINREQ - request to join the group
  JOINREP - replyto JOINREQ
  NODEMSG - message for node  
*/
enum Msgtypes{
		JOINREQ,			
		JOINREP,
		DUMMYLASTMSGTYPE,
		NODEMSG,
		NODEHEARTBEAT //<----------- To indicate membership list
};

/* Generic message template. */
typedef struct messagehdr{ 	
	enum Msgtypes msgtype;
} messagehdr;


/* Functions in mp2_node.c */

/* Message processing routines. */
STDCLLBKRET Process_joinreq STDCLLBKARGS; // void Process_joinreq (void *env, char *data, int size)
STDCLLBKRET Process_joinrep STDCLLBKARGS; // void Process_joinrep (void *env, char *data, int size)
STDCLLBKRET Process_memberlist STDCLLBKARGS; // void Process_memberlist(void *env,char *data,int size)
STDCLLBKRET Process_memberheartbeat STDCLLBKARGS;
/*
int recv_callback(void *env, char *data, int size);
int init_thisnode(member *thisnode, address *joinaddr);
*/

/*
Other routines.
*/

void nodestart(member *node, char *servaddrstr, short servport);
void nodeloop(member *node);
int recvloop(member *node);
int finishup_thisnode(member *node);
int getNode(char *data,int size);
int getHeartBeat(char *data,int size);
void send_helper(member*node,int i,int j);
void send_helper2(member*node,int i,int j);
int getNodeHeart(char *data,int size);

#endif /* _NODE_H_ */

