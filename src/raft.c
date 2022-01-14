/******************************

* Raft

* Jacob English

* je787413@ohio.edu

*******************************/

#include "contiki.h"

//#include "net/ip/uip.h"

//#include "net/ip/uip-debug.h"

#include "lib/random.h"

#include "node-id.h"

//#include "ieee-addr.h"

#include "raft.h"

#include "dev/leds.h"



#include <stdio.h>



uint8_t get_timeout(void);
/*
static unsigned short int entries[10] = {0,0,0,0,0,0,0,0,0};

static unsigned short int log[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};*/



// RAFT NODE Functions

void raft_init(struct Raft *node) {

  node->term = 0;

  node->currentTerm=0;

  //int i = 0;

  //ieee_addr_cpy_to(node->macAddr, 8);

  node->id = node_id;

  node->votedFor = 0;

  /*for (i = 0; i < 8; ++i)

    node->votedFor[i] = 0;*/

  node->timeout = get_timeout();

  node->state = follower;

  node->totalVotes = 0;

  node->totalCommits = 0;



  int i = 0;
  for (i = 0; i < 15; ++i)

    node->log[i] = 0;

  //note: i havent applied the stable storage on all servers bit

  

  node->commitIndex=0;
  

  //volatile state on leaders

  node->nextIndex=0;

  node->matchIndex=0;

  

  //additional terms required for RequestVote RPC

  //voteGranted is in buildVote

  node->lastLogIndex = 0;

  node->lastLogTerm = 0;

  

  //additional terms required for AppendEntries RPC

  //success is within the if-else statements

  node->prevLogIndex = 0;

  node->prevLogTerm = 0;


  int j = 0;
  for (j; j < 10; ++j) {

    node->entries[j] = 0; }

  node->leaderCommit = 0;

  

};



uint8_t get_timeout() {

  //contiki random function (0 - 65,535)

  uint16_t r = random_rand();

  printf("RAND: %d\n", r);



  //scale value to min and max timeout range

  float _r = (float)r;

  float _randMax = (float)RANDOM_RAND_MAX;

  float _maxTimeout = (float)MAX_TIMEOUT;

  float _minTimeout = (float)MIN_TIMEOUT;

  float res = (_r / _randMax) * (_maxTimeout - _minTimeout) + _minTimeout;



  return (const uint8_t)res;

}



void raft_set_follower(struct Raft *node) {

  node->state = follower;

  /*int i = 0;

  for (i = 0; i < 8; ++i) //reset voted for

    node->votedFor[i] = 0;*/

  node->votedFor = 0;

  node->totalVotes = 0;

  leds_on(LEDS_RED);

  leds_off(LEDS_GREEN);

  printf("Set State: Follower \n");

  raft_print(node);

}



void raft_set_candidate(struct Raft *node) {

  node->state = candidate;
  //node->id = id;

  //vote for self

  //ieee_addr_cpy_to(node->votedFor, 8);

  node->votedFor = node_id;

  node->totalVotes = 1;

  leds_on(LEDS_RED);

  leds_on(LEDS_GREEN);

  printf("Set State: Candidate \n");

  raft_print(node);

}



void raft_set_leader(struct Raft *node) {

  node->state = leader;

  leds_on(LEDS_GREEN);

  leds_off(LEDS_RED);

  printf("Set State: Leader \n");

  raft_print(node);

}





void raft_print(struct Raft *node) {

  printf("NODE: {term: %ld, ", node->term);

  //int i = 0;

  printf("id: %d, ", node->id);

  printf("votedFor: %d, ", node->votedFor);

  /*

  for (i = 0; i < 8; ++i)

    printf("%d", node->macAddr[i]);

  printf(",\n\t votedFor: ");

  for (i = 0; i < 8; ++i)

    printf("%d", node->votedFor[i]); */

  printf("timeout: %d, state: %d, totalVotes: %d}\n",\
   node->timeout, node->state, node->totalVotes);
 /* printf("totalCommits: %d, commitIndex: %d, lastApplied: %d, nextIndex, %d", node->totalCommits, \
    node->commitIndex, node->lastApplied, node->nextIndex);
  printf("matchIndex: %d, lastLogIndex: %d, lastLogTerm: %d, prevLogIndex: %d, ", \
    node->matchIndex, node->lastLogIndex, node->lastLogTerm, node->prevLogIndex);
  printf("prevLogTerm: %d, leaderCommit: %d \n", node->prevLogTerm, node->leaderCommit); */

}



bool id_compare(unsigned short int a, unsigned short int b) {

    if (a != b)

      return false;

    else  return true;

}



/*

bool mac_compare(uint8_t a[], uint8_t b[]) {

  int i = 0;

  for (i = 0; i < 8; ++i) {

    if (a[i] != b[i])

      return false;

  }

  return true;

}

*/

// RAFT MSG Functions

void build_msg(struct Msg *msg);

void build_election(struct Election *elect, uint32_t term, unsigned short int from, \
  uint8_t lastLogIndex, uint8_t lastLogTerm) {

  elect->type = election;
  elect->bType = broadcast_msg;

  elect->term = term;

  elect->from = node_id;

  /*

  int i = 0;

  for (i = 0; i < 8; ++i)

    elect->from[i] = from[i];*/

  elect->lastLogIndex = lastLogIndex;\

  elect->lastLogTerm = lastLogTerm; 

}





void build_vote(struct Vote *voteMsg, uint32_t term, unsigned short int from,\
  unsigned short int voteFor, bool voteGranted) {

  voteMsg->type = vote;
  voteMsg->bType = unicast_msg;

  voteMsg->term = term;

  voteMsg->from = node_id;

  voteMsg->voteFor = voteFor;

  /*

  int i = 0;

  for (i = 0; i < 8; ++i)

    voteMsg->from[i] = from[i];

  for (i = 0; i < 8; ++i)

    voteMsg->voteFor[i] = voteFor[i]; */

  voteMsg->voteGranted = voteGranted;

}



void build_heartbeat(struct Heartbeat *heart, uint32_t term, 
  unsigned short int from, uint8_t prevLogIndex,

               uint8_t prevLogTerm, uint8_t nextIndex,/*uint8_t prevValue,*/ uint8_t value, uint8_t leaderCommit) {

  heart->type = heartbeat;
  heart->bType = broadcast_msg;

  heart->term = term;

  heart->from = node_id;

  /*int i = 0;

  for (i = 0; i < 8; ++i)

    heart->from[i] = from[i];*/

  heart->prevLogIndex = prevLogIndex;

  heart->prevLogTerm = prevLogTerm;
  //heart->entries[10] = array1[10];

  //heart->prevValue = prevValue;
  heart->value = value;
  heart->nextIndex = nextIndex;
  

  heart->leaderCommit = leaderCommit;

}



void build_response(struct Response *response, uint8_t commitIndex, 
  uint8_t currentTerm, unsigned short int from,
 uint8_t prevLogIndex, 
  uint8_t prevLogTerm, uint8_t valueCheck) {

response->type = respond;
response->bType = unicast_msg;
response->commitIndex=commitIndex;
response->currentTerm=currentTerm;
response->from = node_id;
response->prevLogIndex=prevLogIndex;
response->prevLogTerm=prevLogTerm;

response->valueCheck=0;

}





//RAFT PRINT FUNCTIONS



void msg_print(uint32_t currTerm, uint8_t node_id, struct Msg *msg) {

  printf("MSG from \n");

  printf("Sender ID: ");

  printf("%d", msg->from);  
  //uip_debug_ipaddr_print(from);

  printf(" in term %ld: {type: %d, term: %ld}\n", currTerm, msg->type, msg->term);


  /*

  int i = 0;

  for (i = 0; i < 8; ++i)

    printf("%d", msg->from[i]);*/

  printf("\n");

}



void heartbeat_print(struct Heartbeat *heart) {


  printf("HEARTBEAT: {type: %d, term: %ld, ", heart->type, heart->term);

  // uip_debug_ipaddr_print(&heart->leaderId);

  printf("nextIndex: %d, prevLogIndex: %d, prevLogTerm: %d, leaderCommit: %d} \n ",

         heart->nextIndex, heart->prevLogIndex, heart->prevLogTerm, heart->leaderCommit);
    /*

  int i = 0;

  for (i = 0; i < 10; ++i)

    printf("%d", heart->entries[i]);*/



}



void election_print(struct Election *elect) {
  //printf("BROADCAST MESSAGE SENT \n");

  printf("ELECTION CALLED: {type: %d, term: %ld, lastLogIndex: %d, lastLogTerm: %d}\n",

         elect->type, elect->term, elect->lastLogIndex, elect->lastLogTerm);

}



void vote_print(struct Vote *vote) {\
  //printf("UNICAST MESSAGE SENT \n");

  printf("VOTED FOR ELECTION: {type: %d, term: %ld, voteFor: %d,",

         vote->type, vote->term, vote->voteFor);
  /*

  int i = 0;

  for (i = 0; i < 8; ++i)

    printf("%d", vote->voteFor[i]);*/

  printf("voteGranted: %s}\n", vote->voteGranted ? "true" : "false");

}

void response_print(struct Response *response){
  //printf("UNICAST MESSAGE SENT \n");
  printf("RESPONSE: {commitIndex: %d, currentTerm: %d, from: %d, prevLogIndex: %d, \
    prevLogTerm: %d, valueCheck: %d} \n", response->commitIndex, response->currentTerm, \
    response->from, response->prevLogIndex,\
    response->prevLogTerm, response->valueCheck);
}


void broadcast_print(struct Msg *msg, struct Raft *node){
  if (msg->bType == broadcast_msg && msg->from == node->id) {
    printf("BROADCAST MESSAGE SENT \n");
  }
  else if (msg->bType == broadcast_msg && msg->from != node->id) {
    printf("BROADCAST MESSAGE RECEIVED \n");
  }
  else if (msg->bType == unicast_msg && msg->from == node->id) {
    printf("UNICAST MESSAGE SENT \n");
  }
  else if (msg->bType == unicast_msg && msg->from != node->id) {
    printf("UNICAST MESSAGE RECEIVED \n");
  }
  else{
    printf("Oh dear, unknown communication \n");
  }
}


