/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/
#include "contiki.h"
#include "net/ip/uip.h"
#include "lib/random.h"
#include "raft.h"

#include <stdio.h>

uint8_t get_timeout(void);

void raft_init(struct Raft *node) {
  node->term = 0;
  node->timeout = (uint8_t)get_timeout();
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
