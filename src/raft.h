/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/

#include "contiki.h"



#define UDP_PORT 1234 //UDP Broadcast Port for messaging

#define MIN_TIMEOUT 5 //minimum value for timeout

#define MAX_TIMEOUT 10 //maximum value for timeout

#define LEADER_SEND_INTERVAL (MIN_TIMEOUT / 2)

#define TOTAL_NODES 3 //total number of nodes in network

typedef enum {false = 0, true = !false} bool;



enum states {follower, candidate, leader};

enum msg_types {heartbeat, election, vote, respond};
enum broadcast_types {unicast_msg, broadcast_msg};



struct Raft {

  uint32_t term;

  uint32_t currentTerm;

  unsigned short int id;

  unsigned short int votedFor;

  uint8_t timeout;

  enum states state;

  uint8_t totalVotes;

  uint8_t totalCommits;  

  uint8_t log[15];

  

  uint8_t commitIndex;

  uint8_t lastApplied;

  

  unsigned short int nextIndex;

  unsigned short int matchIndex;

  

  uint8_t lastLogIndex;

  uint8_t lastLogTerm;

  

  uint8_t prevLogIndex;

  uint8_t prevLogTerm;





  uint8_t entries[10]; 

  uint8_t leaderCommit;
  struct Set *voterSet;


  

};

struct Set
{
    /* data */
    int *members;
    int length;
} ;


// default message struct to determine type and term on incoming messages

struct Msg {

  enum msg_types type;
  enum broadcast_types bType;

  uint32_t term;

  unsigned short int from;

};



// heartbeat message

struct Heartbeat {

  enum msg_types type;

  enum broadcast_types bType;

  uint32_t term;

  unsigned short int from;

  uint8_t prevLogIndex; // Not sure what to do with these quite yet

  uint8_t prevLogTerm;  //

  uint8_t value;      //

  uint8_t nextIndex;


  uint8_t leaderCommit; //

};

void build_heartbeat(struct Heartbeat *heart, uint32_t term, unsigned short int from, uint8_t prevLogIndex,

               uint8_t prevLogTerm, uint8_t nextIndex, uint8_t value, uint8_t leaderCommit);

 
               

struct Response {

  enum msg_types type;
  enum broadcast_types bType;
  uint8_t commitIndex; 
  uint8_t currentTerm; 
  unsigned short int from;
  uint8_t prevLogIndex; // Not sure what to do with these quite yet
  uint8_t prevLogTerm;  //

  bool success;   
  


};              

void build_response(struct Response *response, uint8_t commitIndex, uint8_t currentTerm,
  unsigned short int from,
  uint8_t prevLogIndex, 
  uint8_t prevLogTerm, bool success); 






// start election message

struct Election {

  enum msg_types type;
  enum broadcast_types bType;

  uint32_t term;

  unsigned short int from;

  uint8_t lastLogIndex; // Same as above

  uint8_t lastLogTerm;  //

};

void build_election(struct Election *elect, uint32_t term, unsigned short int from, uint8_t lastLogIndex, uint8_t lastLogTerm);



// vote response for election

struct Vote {

  enum msg_types type;
  enum broadcast_types bType;

  uint32_t term;

  unsigned short int from;

  unsigned short int voteFor;

  bool voteGranted;

};

void build_vote(struct Vote *vote, uint32_t term, unsigned short int from, unsigned short int voteFor, bool voteGranted);

//SET DECLARATIONS



void init_set(struct Raft *node);
bool check_empty_set(struct Raft *node);
void insert_set_member(struct Raft *node, int member);
bool is_set_member(struct Raft *node, int value);
void print_set(struct Raft *node);

/*--------*/

void raft_init(struct Raft *node);

void raft_print(struct Raft *node);

void raft_set_follower(struct Raft *node);

void raft_set_candidate(struct Raft *node);

void raft_set_leader(struct Raft *node);



bool id_compare(unsigned short int a, unsigned short int b);



void msg_print(uint32_t currTerm, uint8_t node_id, struct Msg *msg);

void heartbeat_print(struct Heartbeat *heart);

void election_print(struct Election *elect);

void vote_print(struct Vote *vote);
void response_print(struct Response *response);
void broadcast_print(struct Msg *msg, struct Raft *node);


void call_election(struct Raft *node);

void send_vote(struct Raft *node);

void send_heartbeat(struct Raft *node);
