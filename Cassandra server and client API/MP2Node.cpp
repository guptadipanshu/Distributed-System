/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 * author -dipanshu gupta
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	for(int i=0;i<3000;i++)
	{
		trans_status[i]=-1;
		trans_type[i]=-1;
	}
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());
	
	//GET Ring DS , if ring DS !=currMemList, ring changed 
	if(ring.size()==0)
		ring=curMemList;
	
	if(curMemList.size()!=ring.size() && curMemList.size()>0)
	{
		//cout<<curMemList.size() <<" "<<ring.size();
 		//ring=curMemList;
		change=true;
	}
	else
	{
		//cout<<"THE LISTS HAVE SAME SIZE ,CHECK IF THE CODES MATCH\n";
		int size=curMemList.size();
		for(int i=0;i<size;i++)
		{
			if(curMemList[i].getHashCode()!=ring[i].getHashCode())
			{	
				//ring=curMemList;
				change=true;
				cout<<"Hash code differed\n";
			}
		}
	}
	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	if(change)
	{
		stabilizationProtocol(curMemList);
	}		
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		//cout<<id<<":"<<port<<"\n";
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	//cout<<"hashFunction\n";
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */
	 //cout<<"clientCreate key val"<< key<<" "<<value<<" "<<hashFunction(key)<<"\n";
	 // find the index of neighbors in ring
	 size_t hash_key=hashFunction(key);
	 int id=g_transID+1;
	 int size=ring.size();
	 int prim_ring_index=0,sec_ring_index,ter_ring_index;
	 for(int i=0;i<size;i++)
	 {
	 	if(ring[i].getHashCode()>=hash_key)
	 	{
	 		prim_ring_index=i;
	 		break;
	 	}
	 }
	 //cout<<hash_key<<" "<<ring[ring_index].getHashCode()<<"\n";
	 if(size-prim_ring_index>2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=prim_ring_index+2;
	 }
	 else if(size-prim_ring_index==2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=0;
	 }
	 else
	 {
	 	sec_ring_index=0;
	 	ter_ring_index=1;
	 }
	
	 // create message for the neighbor node
	 Message msg1(id,this->memberNode->addr,CREATE,key,value,PRIMARY);
	 Message msg2(id,this->memberNode->addr,CREATE,key,value,SECONDARY);
	 Message msg3(id,this->memberNode->addr,CREATE,key,value,TERTIARY);
	 //printRing();
	 //cout<<msg1.toString()<<"..."<<msg2.toString()<<"..."<<msg3.toString()<<"\n";
	 //set the vector for reply 
	 vector<int> reply(3);
	 reply[0]=-1;
	 reply[1]=-1;
	 reply[2]=-1;
	 quroum_status.insert((make_pair( id, reply)));
	 quroum_time.insert((make_pair( id, par->getcurrtime())));
	 trans_status[id]=5;
	 trans_id[key]=id;
	 trans_type[id]=CREATE;
	 trans_key.insert((make_pair( id, key)));
	 trans_val.insert((make_pair( id, value)));
	 dispatchMessage(msg1,prim_ring_index);
	 dispatchMessage(msg2,sec_ring_index);
	 dispatchMessage(msg3,ter_ring_index);
	 
	 g_transID++;
	 
}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */
	 //cout<<"clientRead\n";
	 int id=g_transID+1;
	 size_t hash_key=hashFunction(key);
	 int size=ring.size();
	 int prim_ring_index=0,sec_ring_index,ter_ring_index;
	 for(int i=0;i<size;i++)
	 {
	 	if(ring[i].getHashCode()>=hash_key)
	 	{
	 		prim_ring_index=i;
	 		break;
	 	}
	 }
	 //cout<<hash_key<<" "<<ring[ring_index].getHashCode()<<"\n";
	 if(size-prim_ring_index>2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=prim_ring_index+2;
	 }
	 else if(size-prim_ring_index==2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=0;
	 }
	 else
	 {
	 	sec_ring_index=0;
	 	ter_ring_index=1;
	 }
	 
	 vector<int> reply(3);
	 reply[0]=-1;
	 reply[1]=-1;
	 reply[2]=-1;
	 quroum_status.insert((make_pair( id, reply)));
	 quroum_time.insert((make_pair( id, par->getcurrtime())));
	 trans_status[id]=5;
	 trans_type[id]=READ;
	 trans_id[key]=id;
	 trans_key.insert((make_pair( id, key)));
	 
	 
	 Message msg1(id,this->memberNode->addr,READ,key);
	 Message msg2(id,this->memberNode->addr,READ,key);
	 Message msg3(id,this->memberNode->addr,READ,key);
	 dispatchMessage(msg1,prim_ring_index);
	 dispatchMessage(msg2,sec_ring_index);
	 dispatchMessage(msg3,ter_ring_index);
	 g_transID++;
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
	size_t hash_key=hashFunction(key);
	 int id=g_transID+1;
	 int size=ring.size();
	 int prim_ring_index=0,sec_ring_index,ter_ring_index;
	 for(int i=0;i<size;i++)
	 {
	 	if(ring[i].getHashCode()>=hash_key)
	 	{
	 		prim_ring_index=i;
	 		break;
	 	}
	 }
	 //cout<<hash_key<<" "<<ring[ring_index].getHashCode()<<"\n";
	 if(size-prim_ring_index>2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=prim_ring_index+2;
	 }
	 else if(size-prim_ring_index==2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=0;
	 }
	 else
	 {
	 	sec_ring_index=0;
	 	ter_ring_index=1;
	 }
	 Message msg1(id,this->memberNode->addr,UPDATE,key,value,PRIMARY);
	 Message msg2(id,this->memberNode->addr,UPDATE,key,value,SECONDARY);
	 Message msg3(id,this->memberNode->addr,UPDATE,key,value,TERTIARY);
	 //printRing();
	 //cout<<msg1.toString()<<"..."<<msg2.toString()<<"..."<<msg3.toString()<<"\n";
	 vector<int> reply(3);
	 reply[0]=-1;
	 reply[1]=-1;
	 reply[2]=-1;
	 quroum_status.insert((make_pair( id, reply)));
	 quroum_time.insert((make_pair( id, par->getcurrtime())));
	 trans_status[id]=5;
	 trans_type[id]=UPDATE;
	 trans_id[key]=id;
	 trans_val.insert((make_pair( id, value)));
	 trans_key.insert((make_pair( id, key)));
	 
	
	 
	 dispatchMessage(msg1,prim_ring_index);
	 dispatchMessage(msg2,sec_ring_index);
	 dispatchMessage(msg3,ter_ring_index);
	 
	 g_transID++;
	 
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */
	// cout<<"clientDelete\n";
	 size_t hash_key=hashFunction(key);
	 int id=g_transID+1;
	 int size=ring.size();
	 int prim_ring_index=0,sec_ring_index,ter_ring_index;
	 for(int i=0;i<size;i++)
	 {
	 	if(ring[i].getHashCode()>=hash_key)
	 	{
	 		prim_ring_index=i;
	 		break;
	 	}
	 }
	 //cout<<hash_key<<" "<<ring[ring_index].getHashCode()<<"\n";
	 if(size-prim_ring_index>2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=prim_ring_index+2;
	 }
	 else if(size-prim_ring_index==2)
	 {
	 	sec_ring_index=prim_ring_index+1;
	 	ter_ring_index=0;
	 }
	 else
	 {
	 	sec_ring_index=0;
	 	ter_ring_index=1;
	 }
	 Message msg1(id,this->memberNode->addr,DELETE,key);
	 Message msg2(id,this->memberNode->addr,DELETE,key);
	 Message msg3(id,this->memberNode->addr,DELETE,key);
	 //printRing();
	 //cout<<msg1.toString()<<"..."<<msg2.toString()<<"..."<<msg3.toString()<<"\n";
	 vector<int> reply(3);
	 reply[0]=-1;
	 reply[1]=-1;
	 reply[2]=-1;
	 quroum_status.insert((make_pair( id, reply)));
	 quroum_time.insert((make_pair( id, par->getcurrtime())));
	 trans_status[id]=5;
	 trans_type[id]=DELETE;
	 trans_id[key]=id;
	 trans_key.insert((make_pair( id, key)));
	 dispatchMessage(msg1,prim_ring_index);
	 dispatchMessage(msg2,sec_ring_index);
	 dispatchMessage(msg3,ter_ring_index);
	 
	 g_transID++;
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table<--ht using entry form
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaTyape into the hash table
	//cout<<"CreateKeYVal\n";
	int time_stamp=par->getcurrtime();
	Entry entry(value,time_stamp,replica);
	string val=entry.convertToString();
	bool return_val= ht->create(key,val);
	updateMyreplicas(key,value,replica);
	return return_val;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
	//cout<<"readKey\n";
	string val=ht->read(key);
	if(val=="")
		return "";
	Entry entry(val);
	return entry.value;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
	int time_stamp=par->getcurrtime();
	Entry entry(value,time_stamp,replica);
	string val=entry.convertToString();
	bool return_val= ht->update(key,val);
	return return_val;
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
	//cout<<"deleteKey\n";
	return ht->deleteKey(key);
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;
	//cout<<"checkMessgaes\n";

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	for(int i=0;i<3000;i++)
	{
		if(trans_status[i]==5 && par->getcurrtime()-quroum_time[i]>8 )//timeout for transaction
		{
			trans_status[i]=0;
			//cout<<"timeout ";
			//cout<<quroum_time[i]<<endl;
			switch(trans_type[i])
			{
				case READ : 	log->logReadFail(&this->memberNode->addr,true, i, trans_key[i]);
								break;
				case CREATE :	log->logCreateFail(&this->memberNode->addr,true, i, trans_key[i], trans_val[i]);
								break;
				
				case UPDATE :	log->logUpdateFail(&this->memberNode->addr,true, i, trans_key[i], trans_val[i]);
								break;
				case DELETE :	log->logDeleteFail(&this->memberNode->addr,true, i, trans_key[i]);
								break;
			}
			
		}
	}
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		// cout<<"checkMessgaesWHILE\n";
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);

		/*
		 * Handle the message types here
		 */
		 //transID::fromAddr::CREATE::key::value::ReplicaType
		 
		//cout<<message<<"\n";
		Message msg_pkt(message);
		MessageType type=msg_pkt.type;
		ReplicaType replica=msg_pkt.replica;
		string key=msg_pkt.key;
		string value=msg_pkt.value;
		Address fromAddr=msg_pkt.fromAddr;
		int transID=msg_pkt.transID;
		bool coord=false;
		bool result=msg_pkt.success;
		int ret_val;
		string client_val;
		switch(type){
			case CREATE :	//cout<<type<<"."<<replica<<"."<<key<<"."<<value<<"."<<transID<<"\n";
							result=createKeyValue(key,value,replica);
							if(result)
								log->logCreateSuccess(&this->memberNode->addr,coord, transID, key, value);
							else
								log->logCreateFail(&this->memberNode->addr,coord, transID, key, value);
							sendReply(transID,result,fromAddr);

							break; 
			
			case UPDATE :	//cout<<type<<"."<<replica<<"."<<key<<"."<<value<<"."<<transID<<"\n";
							result=updateKeyValue(key,value,replica);
							if(result)
								log->logUpdateSuccess(&this->memberNode->addr,coord, transID, key, value);
							else
								log->logUpdateFail(&this->memberNode->addr,coord, transID, key, value);
							sendReply(transID,result,fromAddr);

							break; 
							
			case READ  :	client_val=readKey(key);
							if(client_val!="")
								result=true;
							else
								result=false;
							if(result)
								log->logReadSuccess(&this->memberNode->addr,coord, transID, key, client_val);
							else
								log->logReadFail(&this->memberNode->addr,coord, transID, key);
							sendReadReply(transID,client_val,fromAddr);

							break; 
			case DELETE :	result=deletekey(key);
							if(result)
								log->logDeleteSuccess(&this->memberNode->addr,coord, transID, key);
							else
								log->logDeleteFail(&this->memberNode->addr,coord, transID, key);
							sendReply(transID,result,fromAddr);

							break; 
			
			case REPLY :	if(trans_status[transID]==5)
							{
								ret_val= get_quorum(result,transID);
								//cout<<ret_val<<endl;
								if(ret_val==1)
								{
									if(trans_type[transID]==CREATE  && transID!=0)
										log->logCreateSuccess(&this->memberNode->addr,true, transID, trans_key[transID],trans_val[transID]);
									else if(trans_type[transID]==UPDATE)
										log->logUpdateSuccess(&this->memberNode->addr,true, transID, trans_key[transID],trans_val[transID]);
									else if (trans_type[transID]==DELETE  && transID!=0)
										log->logDeleteSuccess(&this->memberNode->addr,true, transID, trans_key[transID]);
									trans_status[transID]=1;
									//cout<<"coordinator replica";
								}
								else if(ret_val==0)
								{
									if(trans_type[transID]==CREATE && transID!=0) 
										log->logCreateFail(&this->memberNode->addr,true, transID, trans_key[transID],trans_val[transID]);
									else if(trans_type[transID]==UPDATE)
										log->logUpdateFail(&this->memberNode->addr,true, transID, trans_key[transID],trans_val[transID]);
									else if (trans_type[transID]==DELETE  && transID!=0)
										log->logDeleteFail(&this->memberNode->addr,true, transID, trans_key[transID]);
									trans_status[transID]=0;
								}		
							}
							break;
			case READREPLY:	if(trans_status[transID]==5)
							{				
								ret_val=get_readReplyquorum(value,transID);
							 	if(ret_val==1)
							 	{
							 		log->logReadSuccess(&this->memberNode->addr,true, transID, trans_key[transID],value);
							 		trans_status[transID]=1;
							 	}
							 	else if(ret_val==0)
							 	{
							 		log->logReadFail(&this->memberNode->addr,true, transID, trans_key[transID]);
							 		trans_status[transID]=0;
							 	}
							}
							 break;
							
		}
	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	//cout<<"fINDnODES\n";
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			int size=ring.size();
			for (int i=1; i<size; i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
   
    if ( memberNode->bFailed ) {
   // cout<<"rwcvLoop failed\n";
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol(vector<Node> curMemList) {
	/*
	 * Implement this
	 */
	// scan all keys . for every key find its hash, 
	//find all neighbors it is hashed to,for every neighbor hashed delete the key,then call servercreate
	ring=curMemList;
	int size =node_keys.size();
	for(int i=0;i<size;i++)
	{
		string entry_value=ht->read(node_keys[i]);
		Entry entry(entry_value);
		string value=entry.value;
		vector<Node> neighbors=findNodes(node_keys[i]);
		int nsize=neighbors.size();
		//cout<<nsize<<" \n";
		for(int j=0;j<nsize;j++)
		{
			if(&this->memberNode->addr!=neighbors.at(j).getAddress())
			{
				//deleteKeyValHelper(node_keys[i],neighbors.at(j));
				reinsertKeyVal(node_keys[i],value,neighbors.at(j),j);
			}
		}
		
	}
	 
}


/***************************************************************Helper Functions*********************/

void MP2Node::deleteKeyValHelper(string key,Node node)
{
	Message msg(0,this->memberNode->addr,DELETE,key);
	emulNet->ENsend(&this->memberNode->addr,node.getAddress(),msg.toString());
}

void MP2Node::reinsertKeyVal(string key,string value , Node node,int replica)
{
	ReplicaType type;
	if(replica==0)
		type=PRIMARY;
	else if(replica==1)
		type=SECONDARY;
	else
		type=TERTIARY;
	Message msg(0,this->memberNode->addr,CREATE,key,value,type);
	emulNet->ENsend(&this->memberNode->addr,node.getAddress(),msg.toString());
}

void MP2Node::updateMyreplicas(string key,string value,ReplicaType replica)
{
	vector<Node> neighbors=findNodes(key);
	int size=neighbors.size();
	for(int i=0;i<size;i++)
	{
		if(replica==PRIMARY && &this->memberNode->addr!=neighbors.at(i).getAddress())
			hasMyReplicas.push_back(neighbors.at(i));
		else if(&this->memberNode->addr!=neighbors.at(i).getAddress())
		{
			haveReplicasOf.push_back(neighbors.at(i));
		}
	}
	node_keys.push_back(key);
	
}
void findNeighbors(){}
/**
*Helper function to send message to recv_index node in the ring
**/
void MP2Node::dispatchMessage(Message message,int recv_index)
{
	//send 
	string msg=message.toString();
	emulNet->ENsend(&this->memberNode->addr,ring[recv_index].getAddress(),msg);
}

void MP2Node::sendReadReply(int transID,string result,Address fromAddr)
{
	Message msg(transID,this->memberNode->addr,result);
	emulNet->ENsend(&this->memberNode->addr,&fromAddr,msg.toString());
}
 

void MP2Node::sendReply(int transID,bool result,Address fromAddr)
{
	Message msg(transID,this->memberNode->addr,REPLY,result);
	emulNet->ENsend(&this->memberNode->addr,&fromAddr,msg.toString());
}



int MP2Node::get_quorum(bool result,int transID)
{
	vector<int> reply(3);
	//if(trans_status[transID]!=5 && trans_status[transID]!=0 && trans_status[transID]!=1)
	//	cout<<"unexpected "<<transID<<" ";
	//else
	//	cout<<"expected "<<transID<<" ";
	reply=quroum_status[transID];
	int flag=-1;
	if(result==true)
	{	
		//cout<<"server had created\n";
		for(int i=0;i<3;i++)
		{
			if(reply[i]==1)
				flag=1;
			//cout<<reply[i]<<" ";	
		}
		//cout<<"\n";	
	}
	else
	{
		//cout<<"failed at server\n";
		for(int i=0;i<3;i++)
		{
			if(reply[i]==0)
				flag=0;
		}	
	}
	int val=0; //0 for false
	if(result==true)
		val=1;
	for(int i=0;i<3;i++)
	{
		if(reply[i]==-1)
		{
			reply[i]=val;
			break;
		}
	}
	quroum_status[transID]=reply;
	return flag;
}
int MP2Node::get_readReplyquorum(string value,int transID)
{
	vector<int> reply(3);
	reply=quroum_status[transID];
	int flag=-1;
	if(value!="")
	{	
		for(int i=0;i<3;i++)
		{
			if(reply[i]==1)
				flag=1;
		}
	}
	else
	{
		//cout<<"failed at server\n";
		for(int i=0;i<3;i++)
		{
			if(reply[i]==0)
				flag=0;
		}	
	}
	int val=0; //0 for false
	if(value!="")
		val=1;
	for(int i=0;i<3;i++)
	{
		if(reply[i]==-1)
		{
			reply[i]=val;
			break;
		}
	}
	quroum_status[transID]=reply;
	return flag;
}



void MP2Node::printRing()
{
 int size=ring.size();
 for(int i=0;i<size;i++)
 {
 	Address addr=*ring[i].getAddress();
 	string message = " " +addr.getAddress();
 	cout<<message<<" :";
 }
 cout<<"\n";
}


vector<Node> MP2Node::getMyAddress()
{
	vector<Node> senderNode;
	Address addressOfThisMember;
	vector<MemberListEntry>::iterator pos;
	int i=0;
	for(pos=this->memberNode->memberList.begin();pos<this->memberNode->myPos;pos++,i++);
	int id = this->memberNode->memberList.at(i).getid();
	short port = this->memberNode->memberList.at(i).getport();
	memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
	memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
	senderNode.emplace_back(Node(addressOfThisMember));
	return senderNode;
}


