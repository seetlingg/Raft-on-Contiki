/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "raft.h"
#include "dev/leds.h"

#include <stdio.h>

uint8_t get_timeout(void);

// RAFT NODE Functions
void raft_init(struct Raft *node) {
  node->term = 0;
  uip_ipaddr(&node->votedFor, 0,0,0,0);
  node->timeout = get_timeout();
  node->state = follower;
  node->totalVotes = 0;
}

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
  uip_ipaddr(&node->votedFor, 0,0,0,0); //reset voted for
  node->totalVotes = 0;
  leds_on(LEDS_RED);
  leds_off(LEDS_GREEN);
  printf("Set State FOLLOWER\n");
  raft_print(node);
}

void raft_set_candidate(struct Raft *node) {
  node->state = candidate;
  //vote for self
  uip_ipaddr(&node->votedFor, 1,1,1,1); //some junk so it won't be the same as the null addr
  node->totalVotes = 1;
  leds_on(LEDS_RED);
  leds_on(LEDS_GREEN);
  printf("Set State CANDIDATE\n");
  raft_print(node);
}
void raft_set_leader(struct Raft *node) {
  node->state = leader;
  leds_on(LEDS_GREEN);
  leds_off(LEDS_RED);
  printf("Set State LEADER\n");
  raft_print(node);
}

void raft_print(struct Raft *node) {
  printf("NODE: {term: %ld, votedFor: ", node->term);
  uip_debug_ipaddr_print(&node->votedFor);
  printf(", timeout: %d, state: %d, totalVotes: %d}\n", node->timeout, node->state, node->totalVotes);
}

// RAFT MSG Functions
void build_election(struct Election *elect, uint32_t term, uint8_t lastLogIndex, uint8_t lastLogTerm) {
  elect->type = election;
  elect->term = term;
  elect->lastLogIndex = lastLogIndex;
  elect->lastLogTerm = lastLogTerm;
}

void build_heartbeat(struct Heartbeat *heart, uint32_t term, uint8_t prevLogIndex,
               uint8_t prevLogTerm, uint8_t entries, uint8_t leaderCommit) {
  heart->type = heartbeat;
  heart->term = term;
  heart->prevLogIndex = prevLogIndex;
  heart->prevLogTerm = prevLogTerm;
  heart->entries = entries;
  heart->leaderCommit = leaderCommit;
}

void build_vote(struct Vote *voteMsg, uint32_t term, uip_ipaddr_t *voteFor, bool voteGranted) {
  voteMsg->type = vote;
  voteMsg->term = term;
  uip_ipaddr_copy(&voteMsg->voteFor, voteFor);
  voteMsg->voteGranted = voteGranted;
}

void msg_print(struct Msg *msg) {
  printf("MSG: {type: %d, term: %ld}\n", msg->type, msg->term);
}

void heartbeat_print(struct Heartbeat *heart) {
  printf("HEARTBEAT: {type: %d, term: %ld", heart->type, heart->term);
  // uip_debug_ipaddr_print(&heart->leaderId);
  printf(", prevLogIndex: %d, prevLogTerm: %d, entries: %d, leaderCommit: %d}\n",
         heart->prevLogIndex, heart->prevLogTerm, heart->entries, heart->leaderCommit);
}

void election_print(struct Election *elect) {
  printf("ELECTION: {type: %d, term: %ld, lastLogIndex: %d, lastLogTerm: %d}\n",
         elect->type, elect->term, elect->lastLogIndex, elect->lastLogTerm);
}

void vote_print(struct Vote *vote) {
  printf("ELECTION: {type: %d, term: %ld, voteFor: ",
         vote->type, vote->term);
  uip_debug_ipaddr_print(&vote->voteFor);
  printf(", voteGranted: %s}\n", vote->voteGranted ? "true" : "false");
}
