/******************************
* Raft
* Jacob English
* je787413@ohio.edu
*******************************/
#include "contiki.h"
#include "net/ip/uip.h"
#include "lib/random.h"
#include "raft.h"


uint8_t get_timeout(void);

void raft_init(struct Raft *node) {
  node->term = 0;
  node->timeout = get_timeout();
  node->state = follower;
}

uint8_t get_timeout() {
  //contiki random function (0 - 65,535)
  uint16_t r = random_rand();
  //scale value to min and max timeout range
  return (uint8_t)(r / RANDOM_RAND_MAX) * (MAX_TIMEOUT - MIN_TIMEOUT) + MIN_TIMEOUT;
}
