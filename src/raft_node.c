/******************************
* Contiki Raft Node
* Jacob English
* je787413@ohio.edu
*******************************/

#include "contiki.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "dev/leds.h"
#include "simple-udp.h"
#include "board-peripherals.h"
#include "net/ip/uip-debug.h"
#include "sys/timer.h"

#include "lib/raft.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define UDP_PORT 1234
#define nth_bit(arg,n) (((arg)>>(n))&1)

static struct simple_udp_connection broadcast_connection;
int recv = 0; //count on received messages
bool buzzer_flag = false;

/*---------------------------------------------------------------------------*/
PROCESS(client_process, "UDP broadcast client process");
AUTOSTART_PROCESSES(&client_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen) {}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(client_process, ev, data) {
  static struct etimer timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  printf("--BROADCAST CLIENT PROCESS BEGIN--\n");

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  leds_toggle(LEDS_GREEN);

  static struct Msg data;
  data.a = 32;
  data.b = 49;
  data.c = 1;

  while(1) {
    etimer_set(&timer, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

      printf("sending data\n");
      printf("DATA: {a: %d, b: %d, c: %d}\n", data.a, data.b, data.c);

      uip_create_linklocal_allnodes_mcast(&addr);
      simple_udp_sendto(&broadcast_connection, &data, sizeof(data), &addr);

      ++data.a;
      data.c *= 2;
      if (data.c == 0)
        data.c = 1;
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
