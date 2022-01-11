/******************************

* Contiki Raft Node

* Jacob English

* je787413@ohio.edu

*******************************/



#include "contiki.h"

#include "sys/etimer.h"

#include "net/ip/uip.h"

#include "net/ipv6/uip-ds6.h"



#include "simple-udp.h"

#include "sys/timer.h"

//#include "ieee-addr.h"



#include "raft.h"



#include <stdio.h>

#include <stdint.h>



static struct Raft node;

//static struct timer nodeTimer;

static struct ctimer nodeTimeout;

static void timeout_callback(void *ptr);

bool init = false;

static struct Vote voteMsg;

static struct Response responseMsg;





static struct simple_udp_connection broadcast_connection;

uip_ipaddr_t addr;



/*---------------------------------------------------------------------------*/

PROCESS(raft_node_process, "UDP broadcast raft node process");

AUTOSTART_PROCESSES(&raft_node_process);

/*---------------------------------------------------------------------------*/

static void

receiver(struct simple_udp_connection *c,

         const uip_ipaddr_t *sender_addr,

         uint16_t sender_port,

         const uip_ipaddr_t *receiver_addr,

         uint16_t receiver_port,

         const uint8_t *data,

         uint16_t datalen) {

  printf("\nGOT MESSAGE\n");

  struct Msg *msg = (struct Msg *)data;

  msg_print(node.term, sender_addr, msg);

  //int i = 0;

  if (msg->term > node.term) { //got msg with higher term, update term and change to follower state

    node.term = msg->term;

    raft_set_follower(&node);

  }

  else if (msg->term < node.term) { //ignore terms less than node term

    return;

  }



  switch (node.state) {

    case follower:

      {

        //election

        if (msg->type == election) {

          struct Election *elect = (struct Election *)data;

          election_print(elect);



          //reset timer

          ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);



          //build vote msg

          //static struct Vote voteMsg;

          build_vote(&voteMsg, node.term, node.macAddr, 0, false);



	   unsigned short int nullAddr = 0;

          //uint8_t nullAddr[8] = {0,0,0,0,0,0,0,0};

          if (mac_compare(nullAddr, node.votedFor) && (msg->term >= node.term) && (elect->lastLogIndex >= node.prevLogIndex)) { //vote has not been used

          voteMsg.voteFor == elect->from;

            printf("Updating votedFor and granting vote.\n");

            printf("elect->from: ");

            printf("%d", elect->from); //voteFor = elect->from. its correct.

            voteMsg.voteGranted = true;

            //update node.votedFor to sender_addr

            /*

            for (i = 0; i < 8; ++i) {

              printf("%d", elect->from[i]);

              node.votedFor[i] = elect->from[i];

              voteMsg.voteFor[i] = elect->from[i];*/

            

            printf("\n");}

            //voteGranted = true already set

          

          else { //vote was used this term

            printf("Vote has already been used this term.\n");

            //voteGranted = false;

            voteMsg.voteGranted = false;

          

          //send vote

          uip_create_linklocal_allnodes_mcast(&addr);

          simple_udp_sendto(&broadcast_connection, &voteMsg, sizeof(voteMsg), &addr);

          }}

        //heartbeat

	else if (msg->type == heartbeat) {

		struct Heartbeat *heart = (struct Heartbeat *)data;

		heartbeat_print(heart);

		//reset timer

		ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

		

          //try to insert log logic here:

		build_response(&responseMsg, node.prevLogIndex, node.prevLogTerm, heart->value);

			  

		//to insert check for similarity of last entry. requires heartbeat log, rather than heartbeat value.

		   if (msg->term >= node.prevLogTerm && heart->prevLogIndex >= node.prevLogIndex && heart->leaderCommit < node.prevLogIndex) {

		   //will require a for loop here from node.prevLogIndex to heart->leaderCommit

		   	node.log[node.prevLogIndex+1] = heart->value; // to think about this to include log in each heartbeat
        //node.log[node.prevLogIndex+1] = heart->entries[-1];

		   	}

			  

		  else if (msg->term >= node.prevLogTerm && heart->prevLogIndex >= node.prevLogIndex && heart->leaderCommit == node.prevLogIndex) {

		  	//PLEASE LOOK AT THIS AGAIN, which &addr is it pointing to?

			uip_create_linklocal_allnodes_mcast(&addr);

			simple_udp_sendto(&broadcast_connection, &responseMsg, sizeof(responseMsg), &addr);

		  }

		  

		  }

    

        else {

        raft_set_candidate(&node);

        }

      }

      break;

    case candidate:

      {//vote response

      

        if (msg->type == vote) {

         ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

          struct Vote *vote = (struct Vote *)data;

          vote_print(vote);



          //vote is for this node

          if (mac_compare(vote->voteFor, node.macAddr) && vote->voteGranted) {

            //increment vote count

            ++node.totalVotes;

            if (node.totalVotes > (TOTAL_NODES / 2)) { //if vote count is majority, change to leader & send heartbeat

              raft_set_leader(&node);

             

              static struct Heartbeat heart;

              build_heartbeat(&heart, node.term, node.macAddr, node.prevLogIndex,  node.prevLogTerm, 
                1, node.leaderCommit); 

              uip_create_linklocal_allnodes_mcast(&addr);

              simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

            }

            //ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL); //should have time out at the start and intermittently when receiving new messages?

          }

        }

       else if (msg->type == heartbeat) {

           raft_set_follower(&node);

       }

      }

      break;

    case leader:

      {//log stuff later on

      	//reset timer

        	ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);  



        	static struct Heartbeat heart;

        	build_heartbeat(&heart, node.term, node.macAddr, node.prevLogIndex, node.prevLogTerm, 
            1, node.leaderCommit); 



        	printf("LEADER SENDING HEARTBEAT\n");

        	heartbeat_print(&heart);



        	uip_create_linklocal_allnodes_mcast(&addr);

        	simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

        	if (msg->type == respond){

                  struct Response *response = (struct Response *)data;

                  //vote_print(vote); to include response_print function in raft.c

                  if (responseMsg.valueCheck == heart.value) {

                  	++node.totalCommits;

                  	      

                  	if (node.totalCommits == TOTAL_NODES) {   	

                  	node.leaderCommit = responseMsg.prevLogIndex; 

                  	node.totalCommits = 0;

          }		

	}

	

	node.prevLogIndex+=1;

	break;

      	/* note that this is in the PROCESS_THREAD already

 

          build_heartbeat(&heart, node.term, node.macAddr, node.prevLogIndex,  node.prevLogTerm, 0, node.leaderCommit);

      



      static struct Heartbeat heart;

      build_heartbeat(&heart, node.term, node.macAddr, 0, 0, 0, 0); //fill in vars later



      uip_create_linklocal_allnodes_mcast(&addr);

      simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);*/

      }

      break;

  }

}}

/*---------------------------------------------------------------------------*/

static void timeout_callback(void *ptr) {

  printf("\nTIMEOUT CALLBACK\n");

  if ((node.state == follower) || (node.state == candidate)) {

    printf("msg timeout, starting election process\n");

    ++node.term;

    raft_set_candidate(&node);



    //send election

    static struct Election elect;

    build_election(&elect, node.term, node.macAddr, node.lastLogTerm, node.lastLogIndex); 



    uip_create_linklocal_allnodes_mcast(&addr);

    simple_udp_sendto(&broadcast_connection, &elect, sizeof(elect), &addr);

  }

  else if (node.state == leader) {} //log stuff later on



  ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);

}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(raft_node_process, ev, data) {

  static struct etimer leaderTimer;



  PROCESS_BEGIN();



  printf("--BROADCAST RAFT NODE PROCESS BEGIN--\n");



  if (!init) {

    raft_init(&node);

    init = true;

  }



  raft_print(&node);



  ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);



  simple_udp_register(&broadcast_connection, UDP_PORT,

                      NULL, UDP_PORT,

                      receiver);



  while(1) {

    etimer_set(&leaderTimer, LEADER_SEND_INTERVAL * CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&leaderTimer));

    /*if (node.state == leader) {

      //send heartbeat 

      static struct Heartbeat heart;

      build_heartbeat(&heart, node.term, node.macAddr,node.prevLogIndex,  node.prevLogTerm, 1, node.leaderCommit); //add rest of params later



      printf("LEADER SENDING HEARTBEAT\n");

      heartbeat_print(&heart);



      uip_create_linklocal_allnodes_mcast(&addr);

      simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

    }*/

  }



  PROCESS_END();

}

/*---------------------------------------------------------------------------*/

