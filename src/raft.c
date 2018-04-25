/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "ieee-addr.h"
#include "raft.h"
#include "dev/leds.h"

#include <stdio.h>

uint8_t get_timeout(void);

// RAFT NODE Functions
void raft_init(struct Raft *node) {
  node->term = 0;
  ieee_addr_cpy_to(node->macAddr, 8);
  for (int i = 0; i < 8; ++i)
    node->votedFor[i] = 0;
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
  for (int i = 0; i < 8; ++i) //reset voted for
    node->votedFor[i] = 0;
  node->totalVotes = 0;
  leds_on(LEDS_RED);
  leds_off(LEDS_GREEN);
  printf("Set State FOLLOWER\n");
  raft_print(node);
}

void raft_set_candidate(struct Raft *node) {
  node->state = candidate;
  //vote for self
  ieee_addr_cpy_to(node->votedFor, 8);
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
  printf("NODE: {term: %ld, macAddr: ", node->term);
  for (int i = 0; i < 8; ++i)
    printf("%d", node->macAddr[i]);
  printf(",\n\t votedFor: ");
  for (int i = 0; i < 8; ++i)
    printf("%d", node->votedFor[i]);
  printf(",\n\t timeout: %d, state: %d, totalVotes: %d}\n", node->timeout, node->state, node->totalVotes);
}

bool mac_compare(uint8_t a[], uint8_t b[]) {
  for (int i = 0; i < 8; ++i) {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

// RAFT MSG Functions
void build_election(struct Election *elect, uint32_t term, uint8_t from[], uint8_t lastLogIndex, uint8_t lastLogTerm) {
  elect->type = election;
  elect->term = term;
  for (int i = 0; i < 8; ++i)
    elect->from[i] = from[i];
  elect->lastLogIndex = lastLogIndex;
  elect->lastLogTerm = lastLogTerm;
}

void build_heartbeat(struct Heartbeat *heart, uint32_t term, uint8_t from[], uint8_t prevLogIndex,
               uint8_t prevLogTerm, uint8_t entries, uint8_t leaderCommit) {
  heart->type = heartbeat;
  heart->term = term;
  for (int i = 0; i < 8; ++i)
    heart->from[i] = from[i];
  heart->prevLogIndex = prevLogIndex;
  heart->prevLogTerm = prevLogTerm;
  heart->entries = entries;
  heart->leaderCommit = leaderCommit;
}

void build_vote(struct Vote *voteMsg, uint32_t term, uint8_t from[], uint8_t voteFor[], bool voteGranted) {
  voteMsg->type = vote;
  voteMsg->term = term;
  for (int i = 0; i < 8; ++i)
    voteMsg->from[i] = from[i];
  for (int i = 0; i < 8; ++i)
    voteMsg->voteFor[i] = voteFor[i];
  voteMsg->voteGranted = voteGranted;
}

void msg_print(uint32_t currTerm, const uip_ipaddr_t *from, struct Msg *msg) {
  printf("MSG from ");
  uip_debug_ipaddr_print(from);
  printf(" in term %ld: {type: %d, term: %ld}\n", currTerm, msg->type, msg->term);
  printf("Sender MAC: ");
  for (int i = 0; i < 8; ++i)
    printf("%d", msg->from[i]);
  printf("\n");
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
  printf("VOTE: {type: %d, term: %ld, voteFor: ",
         vote->type, vote->term);
  for (int i = 0; i < 8; ++i)
    printf("%d", vote->voteFor[i]);
  printf(",\n\t voteGranted: %s}\n", vote->voteGranted ? "true" : "false");
}
