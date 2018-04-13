/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/
#include "contiki.h"

#define UDP_PORT 1234 //UDP Broadcast Port for messaging
#define MIN_TIMEOUT 3 //minimum value for timeout
#define MAX_TIMEOUT 7 //maximum value for timeout
#define TOTAL_NODES 4 //total number of nodes in network

enum states {follower, candidate, leader};
enum msg_types {heartbeat, election, vote};

struct Raft {
  uint32_t term;
  uip_ipaddr_t voted_for;
  uint8_t timeout;
  enum states state;
};

// default message struct to determine type and term on incoming messages
struct Msg {
  enum msg_types type;
  uint32_t term;
};

// heartbeat message
struct Heartbeat {
  enum msg_types type;
  uint32_t term;
};

// start election message
struct Election {
  enum msg_types type;
  uint32_t term;
};

// vote response for election
struct Vote {
  enum msg_types type;
  uint32_t term;
};

void raft_init(struct Raft *node);

void call_election(struct Raft *node);

void send_vote(struct Raft *node);

void send_heartbeat(struct Raft *node);
