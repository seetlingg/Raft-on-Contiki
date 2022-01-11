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


#include "simple-udp.h"

#include "sys/timer.h"

//#include "ieee-addr.h"



#include "raft.h"



#include <stdio.h>

#include <stdint.h>


#define BROADCAST_CHANNEL          7      // Channel used for broadcast data transfer
#define UNICAST_CHANNEL            146    // Channel used for unicast data transfer


static struct Raft node;

//static struct timer nodeTimer;

static struct ctimer nodeTimeout;

static void timeout_callback(void *ptr);

bool init = false;

static struct Vote voteMsg;

static struct Response responseMsg;





//static struct simple_udp_connection broadcast_connection;
//for leader
static const struct broadcast_callbacks broadcast_call = {receiver}; //go to receiver function to execute, function pointer

//for follower
static const struct unicast_callbacks unicast_callbacks = {receiver};
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

//uip_ipaddr_t addr;



/*---------------------------------------------------------------------------*/

PROCESS(raft_node_process, "UDP broadcast raft node process");

AUTOSTART_PROCESSES(&raft_node_process);

/*---------------------------------------------------------------------------*/

static void

receiver(struct broadcast_conn *c, const linkaddr_t *from) {

  printf("\nGOT MESSAGE\n");

  struct Heartbeat *msg = (struct Msg *)packetbuf_dataptr();

  //msg_print(node.term, sender_addr, msg);
  //memcpy(msg, packetbuf_dataptr(), sizeof(struct Heartbeat)); // buffer has the copied data


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
          struct Election *elect = (struct Election *)packetbuf_dataptr();
          //memcpy(elect, packetbuf_dataptr(), sizeof(struct Election));

           //if Election, change
          //the type and format of message to Election. this casts the type of message (type cast).
          //memcpy(elect, packetbuf_dataptr(), sizeof(struct Election));

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

          //uip_create_linklocal_allnodes_mcast(&addr);
              struct unicast_conn *c = (struct unicast_conn *)broadcast;
              //simple_udp_sendto(&broadcast_connection, &voteMsg, sizeof(voteMsg), &addr);
              packetbuf_copyfrom(&voteMsg, sizeof(voteMsg));
              //broadcast_send(&broadcast);
              unicast_send(&unicast, elect->from);

          }}

        //heartbeat

	else if (msg->type == heartbeat) {

		struct Heartbeat *heart = (struct Heartbeat *)packetbuf_dataptr();
    
    //memcpy(heart, packetbuf_dataptr(), sizeof(struct Heartbeat));

		heartbeat_print(heart);

		//reset timer

		ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);
		

          //try to insert log logic here:


			  

		//to insert check for similarity of last entry. requires heartbeat log, rather than heartbeat value.

		   if (msg->term >= node.prevLogTerm && heart->prevLogIndex >= node.prevLogIndex 
        && heart->leaderCommit < node.prevLogIndex) {

		   //will require a for loop here from node.prevLogIndex to heart->leaderCommit

		   	node.log[heart->nextIndex] = heart->value; // to think about this to include log in each heartbeat
        //node.log[node.prevLogIndex+1] = heart->entries[-1];

		   	}

			  

		  else if (msg->term >= node.prevLogTerm && heart->prevLogIndex 
        >= node.prevLogIndex && heart->leaderCommit == node.prevLogIndex) {
            build_response(&responseMsg, node.commitIndex, node.currentTerm, 
      node.prevLogIndex, node.prevLogTerm, heart->value);


        struct unicast_conn *c = (struct unicast_conn *)broadcast;
        packetbuf_copyfrom(&responseMsg, sizeof(responseMsg));
        unicast_send(&unicast, heart->from);


		  	//PLEASE LOOK AT THIS AGAIN, which &addr is it pointing to?

			//uip_create_linklocal_allnodes_mcast(&addr);

			//simple_udp_sendto(&broadcast_connection, &responseMsg, sizeof(responseMsg), heart->from);

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

          struct Vote *vote = (struct Vote *)packetbuf_dataptr(); // use memcpy for variable, use typecast for pointer (like here)


          vote_print(vote);



          //vote is for this node

          if (mac_compare(vote->voteFor, node.macAddr) && vote->voteGranted) {

            //increment vote count

            ++node.totalVotes;

            if (node.totalVotes > (TOTAL_NODES / 2)) { //if vote count is majority, change to leader & send heartbeat

              raft_set_leader(&node);

             

              static struct Heartbeat heart;

              build_heartbeat(&heart, node.term, node.macAddr, node.prevLogIndex, 
                node.prevLogTerm, node.nextIndex,
                1, node.leaderCommit); 


              packetbuf_copyfrom(&heart, sizeof(heart));
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

      break;

    case leader:

      {//log stuff later on

      	//reset timer

        	ctimer_set(&nodeTimeout, node.timeout * CLOCK_SECOND, &timeout_callback, NULL);  



        	static struct Heartbeat heart;

        	build_heartbeat(&heart, node.term, node.macAddr, node.prevLogIndex, node.prevLogTerm, 
            node.nextIndex,
            1, node.leaderCommit); 
            
            packetbuf_copyfrom(&heart, sizeof(heart));
            broadcast_send(&broadcast);


        	printf("LEADER SENDING HEARTBEAT\n");

        	heartbeat_print(&heart);



        	//uip_create_linklocal_allnodes_mcast(&addr);

        	//simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);

        	if (msg->type == respond){

                  struct Response *response = (struct Response *)data;

                  //vote_print(vote); to include response_print function in raft.c

              if (responseMsg.currentTerm == heart.term && 
                responseMsg.commitIndex == heart.nextIndex &&
                responseMsg.valueCheck == heart.value) {

              	 ++node.totalCommits;

              	      

                	if (node.totalCommits >= (TOTAL_NODES/2)) {   	

                	node.leaderCommit = responseMsg.prevLogIndex; 

                	node.totalCommits = 0;

                  }		
              }

            else if (responseMsg.commitIndex < heart.nextIndex){
              int patch = 0;
              patch = responseMsg.commitIndex;


              for (patch; patch <responseMsg.commitIndex; patch ++){
                build_heartbeat(&heart, node.term, node.macAddr, 
                  node.prevLogIndex, node.prevLogTerm, node.nextIndex,
                   node.log[patch], node.leaderCommit);

                //to substitute w code that allows one to one sending.

                packetbuf_copyfrom(&heart, sizeof(heart));
                //broadcast_send(&broadcast);
                unicast_send(&unicast, msg->from);
                //uip_create_linklocal_allnodes_mcast(&addr);
                //simple_udp_sendto(&broadcast_connection, &heart, sizeof(heart), &addr);
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

    packetbuf_copyfrom(&elect, sizeof(elect));
    broadcast_send(&broadcast);

    //uip_create_linklocal_allnodes_mcast(&addr);

    //simple_udp_sendto(&broadcast_connection, &elect, sizeof(elect), &addr);

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


  broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
  unicast_open(&unicast, UNICAST_CHANNEL, &unicast_callbacks);
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

      build_heartbeat(&heart, node.term, node.macAddr,node.prevLogIndex,  node.prevLogTerm, 
        1, node.leaderCommit); 



      printf("LEADER SENDING HEARTBEAT\n");

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

