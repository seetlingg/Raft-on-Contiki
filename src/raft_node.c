/******************************
* Contiki Raft Node
* Jacob English
* je787413@ohio.edu
*******************************/



#include "contiki.h"

#include "sys/etimer.h"

//#include "net/ip/uip.h"

//#include "net/ipv6/uip-ds6.h"
#include "net/rime/rime.h"
#include "dev/cc2420/cc2420.h"


//#include "simple-udp.h"

#include "sys/timer.h"

//#include "ieee-addr.h"



#include "raft.h"



#include <stdio.h>

#include <stdint.h>
//#include <stdlib.h>


#define BROADCAST_CHANNEL          7      // Channel used for broadcast data transfer
#define UNICAST_CHANNEL            146    // Channel used for unicast data transfer


static struct Raft node;

//static struct timer nodeTimer;

static struct ctimer nodeTimeout;

static void timeout_callback(void *ptr);

bool init = false;

static struct Vote voteMsg;

static struct Response responseMsg;


static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
//static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from);


//static struct simple_udp_connection broadcast_connection;
//for leader & candidate
static const struct broadcast_callbacks broadcast_call = {broadcast_recv}; //go to receiver function to execute, function pointer

//for follower
//static const struct unicast_callbacks unicast_callbacks = {broadcast_recv};
static struct broadcast_conn broadcast;
//static struct unicast_conn unicast;

//uip_ipaddr_t addr;



/*---------------------------------------------------------------------------*/

PROCESS(raft_node_process, "Broadcast and unicast raft node process");
//PROCESS(simple_comm_process, "Simple communication process");
//PROCESS(unicast_process, "unicast process");
//PROCESS(unicast_process, "unicast process");

AUTOSTART_PROCESSES(&raft_node_process);

/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {

  struct Msg *msg = (struct Msg *)packetbuf_dataptr();
  broadcast_print(msg,&node);

  switch (node.state) {

    case follower:

      {
        //election

        if (msg->type == election) {
          struct Election *elect = (struct Election *)packetbuf_dataptr();
          printf("ELECTION BROADCAST MESSAGE RECEIVED BY FOLLLOWER \n");
          election_print(elect);

          ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

         unsigned short int nullAddr = 0;

         if (msg->term >= node.term){
            node.term = msg->term;
            build_vote(&voteMsg, node.term, node.id, 0, false);



            if (id_compare(nullAddr, node.votedFor) && ((elect->lastLogTerm > node.prevLogTerm) || ((elect->lastLogIndex >= node.prevLogIndex) && \
            (elect->lastLogTerm == node.prevLogTerm)))) { //vote has not been used

                voteMsg.voteFor = elect->from;
                voteMsg.voteGranted = true;

                printf("VOTE GRANTED! \t");
                printf("voteFor: %d \n", voteMsg.voteFor);
                
              }
          }

          

          else { //vote was used this term
              build_vote(&voteMsg, node.term, node.id, 0, false);
              printf("VOTE NOT GRANTED \n");

          }
        
        linkaddr_t bufferId = {{elect->from}};

        packetbuf_copyfrom(&voteMsg, sizeof(voteMsg));
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(bufferId));
    
        //broadcast_send(&broadcast);
        broadcast_send(&broadcast); //check what is const union linkaddr_t, seems like only unicast has this issue
        printf("VOTE UNICAST MESSAGE SENT TO CANDIDATE\n");
        vote_print(&voteMsg);
        }


        //heartbeat

  else if (msg->type == heartbeat) {

    struct Heartbeat *heart = (struct Heartbeat *)packetbuf_dataptr();
    printf("HEARTBEAT BROADCAST RECEIVED BY FOLLOWER \n");
    heartbeat_print(heart);


    ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);
    node.votedFor = 0;

    
    if (msg->term >= node.term){
        node.term = msg->term;
        node.currentTerm = msg->term;
      }

    bool logOK = ((heart->prevLogIndex >= node.prevLogIndex) && \
      (msg->term >= node.prevLogTerm));

    if ((msg->term >= node.term) && logOK) {
        printf("HEARTBEAT VALUE ACCEPTED BY FOLLOWER \n");

        node.log[heart->nextIndex] = heart->value;

        node.prevLogTerm = msg->term;
        node.prevLogIndex = heart->nextIndex;
        node.leaderCommit = heart->leaderCommit;

        
        build_response(&responseMsg, node.commitIndex, node.currentTerm, node.id, \
          node.prevLogIndex, node.prevLogTerm, true);

        linkaddr_t bufferId = {{heart->from}};
        packetbuf_copyfrom(&responseMsg, sizeof(responseMsg));
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(bufferId));
        printf("ACK UNICAST SENT BY FOLLOWER TO LEADER\n");
        response_print(&responseMsg);
        broadcast_send(&broadcast); 
    }
    
    else {
        build_response(&responseMsg, node.commitIndex, node.currentTerm, node.id, \
          node.prevLogIndex, node.prevLogTerm, false);

        linkaddr_t bufferId = {{heart->from}};
        packetbuf_copyfrom(&responseMsg, sizeof(responseMsg));
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(bufferId));
        printf("NACK UNICAST SENT BY FOLLOWER TO LEADER\n");
        response_print(&responseMsg);
        broadcast_send(&broadcast); 

        }

      }
    }

      break;

    case candidate:

      {//vote response

        if (msg->type == vote) {

         ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

          struct Vote *vote = (struct Vote *)packetbuf_dataptr(); // use memcpy for variable, use typecast for pointer (like here)


          printf("VOTE UNICAST MESSAGE RECEIVED BY CANDIDATE \n");
          vote_print(vote);



          //vote is for this node

          if (id_compare(vote->voteFor, node.id) && vote->voteGranted) {

            //increment vote count
            printf("+1 VOTE \n");

            ++node.totalVotes;

            if (node.totalVotes > (TOTAL_NODES / 2)) { //if vote count is majority, change to leader & send heartbeat
              printf("QUORUM MET, SET NODE AS LEADER \n");

              raft_set_leader(&node);

             

              static struct Heartbeat heart;

              build_heartbeat(&heart, node.term, node.id, node.prevLogIndex, 
                node.prevLogTerm, node.nextIndex,
                1, node.leaderCommit); 


              packetbuf_copyfrom(&heart, sizeof(heart));
              printf("HEARTBEAT BROADCAST SENT AFTER BEING ELECTED LEADER \\n");
              heartbeat_print(&heart);

              broadcast_send(&broadcast);



              //uip_create_linklocal_allnodes_mcast(&addr);

              //simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

            }

            //ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL); //should have time out at the start and intermittently when receiving new messages?

          }

        }


       else if (msg->type == heartbeat) {

           raft_set_follower(&node);

       }

      }
    
  case leader:
    {
      if (msg->type == respond){

              struct Response *response = (struct Response *)packetbuf_dataptr();

              printf("RESPONSE UNICAST MESSAGE RECEIVED BY LEADER\n");
              response_print(&responseMsg);

          if (msg->term == node.currentTerm){
            if (response->success){
              ++node.totalCommits;

              if (node.totalCommits >= (TOTAL_NODES/2)) {     

                node.leaderCommit = responseMsg.prevLogIndex; 
                node.totalCommits = 0;
                printf("Commited to index: %d \n", node.leaderCommit);

              }   
            }
          }
  
      } 
    }
}}

/*---------------------------------------------------------------------------*/

static void timeout_callback(void *ptr) {

  printf("\nTIMEOUT CALLBACK\n");

  if ((node.state == follower) || (node.state == candidate)) {

    printf("MSG TIMEOUT, STARTING ELECTION\n");

    node.term+=1;


    printf("+1 NODE TERM\n");
    raft_set_candidate(&node);
    //init_set(&node);


    //send election

    static struct Election elect;

    build_election(&elect, node.term, node.id, node.lastLogTerm, node.lastLogIndex); 

    packetbuf_copyfrom(&elect, sizeof(elect));
    broadcast_send(&broadcast);

    printf("IN TIMEOUT CALLBACK, LEADER SENDING ELECTION BROADCAST REQUEST TO ALL\n");

    election_print(&elect);


  }

  else if (node.state == leader) {} //log stuff later on



  ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

}



/*---------------------------------------------------------------------------*/

PROCESS_THREAD(raft_node_process, ev, data) {

  static struct etimer leaderTimer;


  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();



  printf("--BROADCAST RAFT NODE PROCESS BEGIN--\n");



  if (!init) {

    raft_init(&node);

    init = true;

  }


  broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
  //unicast_open(&unicast, UNICAST_CHANNEL, &unicast_callbacks);
  raft_print(&node);



  ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);



  /*simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);*/



  while(1) {

    etimer_set(&leaderTimer, LEADER_SEND_INTERVAL * CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&leaderTimer));

    if (node.state == leader) {

      //send heartbeat 

      static struct Heartbeat heart;
      node.prevLogTerm = node.term;
      node.prevLogIndex = node.nextIndex;
      node.nextIndex++;

      build_heartbeat(&heart, node.term, node.id, node.prevLogIndex,  node.prevLogTerm, 
        node.nextIndex,
        1, node.leaderCommit); 



      printf("LEADER SENDING BROADCAST HEARTBEAT (WHILE LOOP)\n");

      heartbeat_print(&heart);

      packetbuf_copyfrom(&heart, sizeof(heart));
      broadcast_send(&broadcast);



      //uip_create_linklocal_allnodes_mcast(&addr);

      //simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

    }

  }



  PROCESS_END();

}

/*---------------------------------------------------------------------------*/
