/**********************
*
* Progam Name: MP1. Membership Protocol
* 
* Code authors: Dipanshu Gupta
*
* Current file: mp1_node.c
* About this file: Member Node Implementation
* 
***********************/

#include "mp1_node.h"
#include "emulnet.h"
#include "MPtemplate.h"
#include "log.h"


/*
 *
 * Routines for introducer and current time.
 *
 */

char NULLADDR[] = {0,0,0,0,0,0};
int isnulladdr( address *addr){
    return (memcmp(addr, NULLADDR, 6)==0?1:0);
}

/* 
Return the address of the introducer member. 
*/
address getjoinaddr(void){

    address joinaddr;

    memset(&joinaddr, 0, sizeof(address));
    *(int *)(&joinaddr.addr)=1;
    *(short *)(&joinaddr.addr[4])=0;

    return joinaddr;
}

/*
 *
 * Message Processing routines.
 *
 */

/* 
Received a JOINREQ (joinrequest) message.
*/
void Process_joinreq(void *env, char *data, int size)
{
	
	//fprintf(stderr,"JOINREQ pktdata %s size%d init %d ingroup %d mem fail %d \n",
   	//	data,size,node->inited,node->ingroup,node->bfailed);
   	
	member *node = (member *) env;
	
	/*Get the address of node that will join introducer node */
	address *addr=malloc(sizeof(address));
	memset(addr, 0, sizeof(address));
	memcpy((address *)&(addr), &data, size);
	
   	/* 
   	*	update yor data structure neghibor[i][j] with new member i.
   	*/
  	
  	int request_process=addr->addr[0];
  	if(node->neighbor[request_process][0]==0 && node->neighbor[request_process][0]!=-1)
  	{
  		printf("added node %d by introducer\n",request_process);
  		node->neighbor[request_process][0]=1;// mark process as alive
  		node->neighbor[request_process][1]=1;// set heartbeat as zero
  		node->neighbor[request_process][2]=getcurrtime();
  		node->neighbor_addr[request_process]=*addr;
  		if(node->neighbor[1][0]==0)
  		{
  			node->neighbor[1][0]=1; //assume introducer never fails
  			node->neighbor[1][1]=1;
  			node->neighbor[1][2]=getcurrtime(); 
  			node->neighbor_addr[1]=node->addr;
  			node->timeout=20;
  			logNodeAdd(&node->addr,&node->addr);
  		}
  		logNodeAdd(&node->addr,addr);
  	}
  	int i;
  	/*
	  * Node env is the destination that will get Join Reply message
	  *	call MpP2psend from here for that node with message as data
	  *	create data packet with msgtype=NODEMSG,process address.heartbeat
	  */
  	for(i=0;i<200;i++) // Assume that max 200 nodes in cluster
    {
    	if(node->neighbor[i][0]==1)
    	{
    		char clusternode[20];
    		int heartbeat=node->neighbor[i][1];
			memset(clusternode,'\0',sizeof(clusternode));
			sprintf(clusternode,"%d.%d",i,heartbeat);
			
			messagehdr *msg;
			size_t msgsize = sizeof(messagehdr) + sizeof(address);
        	//printf("msg_size %d msgheader %d addr %d \n",msgsize,sizeof(messagehdr),sizeof(address));
        	msg=malloc(msgsize);
      
        	msg->msgtype=NODEMSG;
        	memcpy((char *)(msg+1),&node->neighbor_addr[i],sizeof(address));
			MPp2psend(&node->addr,addr,(char *)msg, msgsize);
			//printf("message send from joinreq from node %d to %d about %d \n",node->addr.addr[0],addr->addr[0],i);
    		
    		
    		// send heartbeat 
    		messagehdr *msg2;
    		size_t msgsize2 = sizeof(messagehdr)+20;
    		msg2=malloc(msgsize2);
    		msg2->msgtype=NODEHEARTBEAT;
    		memcpy((char *)(msg2+1),clusternode,20);
    		MPp2psend(&node->addr,addr,(char *)msg2, msgsize2);
    	}
    }
    
    return;
}

/* 
Received a JOINREP (joinreply) message. 
*/
void Process_joinrep(void *env,char *data,int size)
{
	  printf("JOINREPLY\n");
    return;
}
/**********Helper Function to update Cluster List*********/
/*

 	* for the received member check if they are not in table

	* if not present add to log  

	* we donot remove any member here wait for nodelooppop to that 

*/
void Process_memberlist(void *env,char *data,int size)
{
 	//fprintf(stderr,"MEMBERLIST pktdata %s size%d init %d ingroup %d mem fail %d node address%d\n",
   	//	data,size,node->inited,node->ingroup,node->bfailed,node->addr.addr[0]);
 	
	member *node = (member *) env;
	int neighbor_node=getNode(data,size);
	//printf("negihbor received %d with heartbeat %d\n",neighbor_node,neighbor_heartbeat);
   	if(node->neighbor[neighbor_node][0]==0 && node->neighbor[neighbor_node][0]!=-1)  // received a new neighbor
    {	
  		node->neighbor[neighbor_node][0]=1;// mark process as alive
  		node->neighbor[neighbor_node][1]+=1;//hearbeat
  		node->neighbor[neighbor_node][2]=getcurrtime();
  		
  		address *addr=malloc(sizeof(address));
		memset(addr, 0, sizeof(address));
		memcpy((address *)&(addr), &data, 6);
		node->neighbor_addr[neighbor_node]=*addr; //store in address for future
  		logNodeAdd(&node->addr,addr);
    	node->ingroup=1;
    }
}
/**********Helper Function to update Cluster List*********/
/*

 	* for the received member check if their hearbeat counter

	* if received hearbeat is greater than heartbeat in our cluster update

	* we donot remove any member here wait for nodelooppop to that 

*/
void Process_memberheartbeat(void *env,char *data,int size)
{
	member *node = (member *) env;
	int neighbor_node=getNodeHeart(data,size);
	int neighbor_heartbeat=getHeartBeat(data,size);
	int current_heartbeat=node->neighbor[neighbor_node][1];
    if(neighbor_heartbeat>current_heartbeat)
    {
    	node->neighbor[neighbor_node][2]=getcurrtime(); // this is the latest time we got heartbeat
    	node->neighbor[neighbor_node][1]=neighbor_heartbeat;
    	//printf("updated hearbeat with curr time %d for %d with newbeat %d\n",getcurrtime(),node->addr.addr[0],neighbor_heartbeat);
    }
}
int getNodeHeart(char *data,int size)
{
	char * node= malloc(sizeof(char)*20);
	memset(node, '\0', 20);
	int i=0;
	for(i=0;i<size;i++)
	{
		if(data[i]=='.')
			break;
		node[i]=data[i];	
	}
	return atoi(node);
}
int getNode(char *data,int size)
{
	address *addr=malloc(sizeof(address));
	memset(addr, 0, sizeof(address));
	memcpy((address *)&(addr), &data, 6);
	int val=addr->addr[0];
	//free(addr);
	//printf("size %d %d.%d.%d.%d\n",size,addr->addr[0],addr->addr[1],addr->addr[2],addr->addr[3]);
	return val;
}
int getHeartBeat(char *data,int size)
{
	char * heartbeat= malloc(sizeof(char)*20);
	memset(heartbeat, '\0', 20);
	int i=0,count=0;
	while(data[i]!='.')
		i++;
	for(i=i+1;data[i]!='\0';i++)
			heartbeat[count++]=data[i];	
	return atoi(heartbeat);
}
/* 
Array of Message handlers. 
*/
void ( ( * MsgHandler [20] ) STDCLLBKARGS )={
/* Message processing operations at the P2P layer. */
    Process_joinreq, 
    Process_joinrep,
    Process_memberlist,	//<--------------- to update cluster list
	Process_memberheartbeat
};

/* 
Called from nodeloop() on each received packet dequeue()-ed from node->inmsgq. 
Parse the packet, extract information and process. 
env is member *node, data is 'messagehdr'. 
*/
int recv_callback(void *env, char *data, int size){

    member *node = (member *) env;
    messagehdr *msghdr = (messagehdr *)data;
    char *pktdata = (char *)(msghdr+1);
	
    if(size < sizeof(messagehdr)){
#ifdef DEBUGLOG
        LOG(&((member *)env)->addr, "Faulty packet received - ignoring");
#endif
        return -1;
    }

#ifdef DEBUGLOG
    LOG(&((member *)env)->addr, "Received msg type %d with %d B payload", msghdr->msgtype, size - sizeof(messagehdr));
#endif

    if((node->ingroup && msghdr->msgtype >= 0 && msghdr->msgtype <= DUMMYLASTMSGTYPE)
        || (!node->ingroup && msghdr->msgtype==JOINREP))    
	   {        
            /* if not yet in group, accept only JOINREPs */
        	printf("calling function msgtype %d from recv_callback\n",msghdr->msgtype);
        	MsgHandler[msghdr->msgtype](env, pktdata, size-sizeof(messagehdr)); // 10.0.0.x pktdata size
        }
   	else if(msghdr->msgtype==NODEMSG) // update address of neighbors in clusterlist
    {
    	//If the node is not in group add it by making ingroup=1;
    	// CALL process_MEMBERLIST FROM HERE. MEMBER LIST UPDATES CLUSTER TABLE
    	MsgHandler[2](env, pktdata, size-sizeof(messagehdr)); 
    }
    else if(msghdr->msgtype==NODEHEARTBEAT) // update heartbeat in cluster list
    {
    	MsgHandler[3](env, pktdata, size-sizeof(messagehdr)); 
    }
    else 
    	printf("recvloop junk data %d \n",msghdr->msgtype);
    	free(data);

    return 0;

}

/*
 *
 * Initialization and cleanup routines.
 *
 */

/* 
Find out who I am, and start up. 
*/
int init_thisnode(member *thisnode, address *joinaddr){
    
    if(MPinit(&thisnode->addr, PORTNUM, (char *)joinaddr)== NULL){ /* Calls ENInit */
#ifdef DEBUGLOG
        LOG(&thisnode->addr, "MPInit failed");
#endif
        exit(1);
    }
#ifdef DEBUGLOG
    else LOG(&thisnode->addr, "MPInit succeeded. Hello.");
#endif

    thisnode->bfailed=0;
    thisnode->inited=1;
    thisnode->ingroup=0;
    /* node is up! */

    return 0;
}


/* 
Clean up this node. 
*/
int finishup_thisnode(member *node){

	/* <your code goes in here> */
    return 0;
}


/* 
 *
 * Main code for a node 
 *
 */

/* 
Introduce self to group. 
*/
int introduceselftogroup(member *node, address *joinaddr){
    
    messagehdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if(memcmp(&node->addr, joinaddr, 4*sizeof(char)) == 0){
        /* I am the group booter (first process to join the group). Boot up the group. */
#ifdef DEBUGLOG
        LOG(&node->addr, "Starting up group...");
#endif

        node->ingroup = 1;
        //printf("booter\n");
    }
    else{
        size_t msgsize = sizeof(messagehdr) + sizeof(address);
        msg=malloc(msgsize);
    /* create JOINREQ message: format of data is {struct address myaddr} */
        msg->msgtype=JOINREQ;
        memcpy((char *)(msg+1), &node->addr, sizeof(address));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        LOG(&node->addr, s);
#endif

    /* send JOINREQ message to introducer member. */
		printf("send join req to introducer\n");       
        MPp2psend(&node->addr, joinaddr, (char *)msg, msgsize); /* SEND message to initiator for join*/
        
        free(msg);
    }

    return 1;

}

/* 
Called from nodeloop(). 
*/
void checkmsgs(member *node){
    void *data;
    int size;

    /* Dequeue waiting messages from node->inmsgq and process them. */
	
    while((data = dequeue(&node->inmsgq, &size)) != NULL) {
       // printf("checkmessage dequeue\n");
        recv_callback((void *)node, data, size); 
    }
    return;
}


/* 
Executed periodically for each member. 
Performs necessary periodic operations. 
Called by nodeloop(). 
*/
/* We have updated Cluster List, Updated the ClusterList before this function call checkmsgs->recvloop->process_memberlist.
	We have updated Cluster List, Updated the ClusterList before this function call checkmsgs->recvloop->process_memberheartbeat.
	*   Increment your (node) heartbeat 
	
	*   For every member in the list check if heartbeat < timeout then logremove and delete from member list.
	
	*	For some random members in the cluster list

	*		send our neighbors to them with heartbeat.

	*/
void nodeloopops(member *node)
{
	if(node->ingroup!=1)
		return;
		
	node->timeout=20; // timeout of 20sec
	int i=0,j;
	int node_index=node->addr.addr[0];
	node->neighbor[node_index][1]++; //increment heartbeat
	for(i=0;i<200;i++) // check heartbeats and delete non responsive neighbors
	{
		if(node->neighbor[i][0]==1)
		{
			if(getcurrtime()-node->neighbor[i][2] > node->timeout && i!=node->addr.addr[0])
			{
				//node has time out
				printf("curr time %d at %d neighbor %d time %d dif %d timeout %d\n",
				getcurrtime(),node->addr.addr[0],i,node->neighbor[i][2],
				getcurrtime()-node->neighbor[i][2],node->timeout);
				node->neighbor[i][0]=-1;
				logNodeRemove(&node->addr,&node->neighbor_addr[i]);
			}
		}
	}
	
	// select a random porcess and gossip them your cluster list
	int send_index=rand()%10+1;
	while(node->neighbor[send_index][0]!=1 && send_index!=node->addr.addr[0])
		send_index=rand()%10+1;
	//printf("send index %d\n",send_index);	
	for(j=0;j<200;j++)			
	{			
		if(node->neighbor[j][0]==1 && j!=send_index)
		{	
			send_helper(node,send_index,j);
			send_helper2(node,send_index,j);
		}
	}
    return;
}
/**
Helper function to send neighbor node address
**/
void send_helper(member*node,int i,int j)
{
	messagehdr *msg;
	size_t msgsize = sizeof(messagehdr) + 20;
    //printf("msg_size %d msgheader %d addr %d \n",msgsize,sizeof(messagehdr),sizeof(address));
    msg=malloc(msgsize);
    msg->msgtype=NODEMSG;
    memcpy((char *)(msg+1),&node->neighbor_addr[j],sizeof(address));
	MPp2psend(&node->addr,&node->neighbor_addr[i],(char *)msg, msgsize);
}
/**
Helper function to send heartbeat
**/
void send_helper2(member*node,int i,int j)
{
	char clusternode[20];
    int heartbeat=node->neighbor[j][1];
	memset(clusternode,'\0',sizeof(clusternode));
	sprintf(clusternode,"%d.%d",j,heartbeat);
	
   	messagehdr *msg2;
   	size_t msgsize2 = sizeof(messagehdr)+20;
   	msg2=malloc(msgsize2);
   	msg2->msgtype=NODEHEARTBEAT;
   	memcpy((char *)(msg2+1),clusternode,20);
   	MPp2psend(&node->addr,&node->neighbor_addr[i],(char *)msg2, msgsize2);
}
/* 
Executed periodically at each member. Called from app.c.
*/
void nodeloop(member *node){
    if (node->bfailed) return;

    checkmsgs(node); // node of the member who called nodeloop

    /* Wait until you're in the group... */
    if(!node->ingroup) return ;

    /* ...then jump in and share your responsibilites! */
    nodeloopops(node);
    
    return;
}

/* 
All initialization routines for a member. Called by app.c. 
*/
void nodestart(member *node, char *servaddrstr, short servport){

    address joinaddr=getjoinaddr();

    /* Self booting routines */
    if(init_thisnode(node, &joinaddr) == -1){

#ifdef DEBUGLOG
        LOG(&node->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if(!introduceselftogroup(node, &joinaddr)){
        finishup_thisnode(node);
#ifdef DEBUGLOG
        LOG(&node->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/* 
Enqueue a message (buff) onto the queue env. 
*/
int enqueue_wrppr(void *env, char *buff, int size){    return enqueue((queue *)env, buff, size);}

/* 
Called by a member to receive messages currently waiting for it. 
*/
int recvloop(member *node){
    if (node->bfailed) return -1;
    else return MPrecv(&(node->addr), enqueue_wrppr, NULL, 1, &node->inmsgq); 
    /* Fourth parameter specifies number of times to 'loop'. */
}

