// Steve Comer - sfc15
// Dan Hartman - drh51
// CS 1652
// Project 2
// 15 Nov 2013


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

#include "Minet.h"
#include "tcpstate.h"

#include <typeinfo>

using namespace std;


int main(int argc, char * argv[]) {

	// declare variables
	MinetHandle mux;
	MinetHandle sock;
	MinetEvent event;
	double timeout;
	ConnectionList<TCPState> clist;
	Connection c;

	// declare incoming variables
	IPHeader iph_in;
	TCPHeader tcph_in;
	unsigned short total_length_in;
	unsigned char iph_length_in;
	unsigned int seq_num_in;
	unsigned int ack_num_in;
	unsigned char flags_in;
	bool checksum_correct_in;
	unsigned short win_size_in;
	Buffer payload_in;
	char * data_in;
	unsigned int data_length_in;
	ConnectionToStateMapping<TCPState> ctsm_in;

	// declare outgoing variables
	IPHeader iph_out;
	TCPHeader tcph_out;
	unsigned int seq_num_out;
	unsigned int ack_num_out;
	unsigned char flags_out;
	unsigned short win_size_out;
	Buffer payload_out;
	unsigned int data_length_out;
	ConnectionToStateMapping<TCPState> ctsm_out;

	// declare other variables
	SockRequestResponse resp;
	SockRequestResponse req;

	// init
	data_length_in = 0;
	data_length_out = 0;
	timeout = 1;
	iph_length_in = 20;
	MinetInit(MINET_TCP_MODULE);

	mux = MinetIsModuleInConfig(MINET_IP_MUX) ?  
		MinetConnect(MINET_IP_MUX) : 
		MINET_NOHANDLE;

	sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? 
		MinetAccept(MINET_SOCK_MODULE) : 
		MINET_NOHANDLE;

	if ( (mux == MINET_NOHANDLE) && 
			(MinetIsModuleInConfig(MINET_IP_MUX)) ) {
		MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));
		return -1;
	}

	if ( (sock == MINET_NOHANDLE) && 
			(MinetIsModuleInConfig(MINET_SOCK_MODULE)) ) {
		MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));
		return -1;
	}

	cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

	MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

	while (MinetGetNextEvent(event, timeout) == 0) {

		// reset used memory
		Packet p_in("", 0);
		Packet p_out("", 0);
		data_length_in = 0;
		data_length_out = 0;

		if ((event.eventtype == MinetEvent::Dataflow) && 
				(event.direction == MinetEvent::IN)) {

			if (event.handle == mux) {

				// receive incoming packet
				MinetReceive(mux, p_in);

				// extract packet headers and flags
				p_in.ExtractHeaderFromPayload<TCPHeader>(TCPHeader::EstimateTCPHeaderLength(p_in));
				iph_in = p_in.FindHeader(Headers::IPHeader);
				tcph_in = p_in.FindHeader(Headers::TCPHeader);
				
				checksum_correct_in = tcph_in.IsCorrectChecksum(p_in);

				// TCP connection 4-tuple
				iph_in.GetProtocol(c.protocol);
				iph_in.GetDestIP(c.src);
				iph_in.GetSourceIP(c.dest);
				tcph_in.GetDestPort(c.srcport);
				tcph_in.GetSourcePort(c.destport);

				// get additional TCP information
				tcph_in.GetSeqNum(seq_num_in);
				tcph_in.GetAckNum(ack_num_in);
				tcph_in.GetFlags(flags_in);
				tcph_in.GetWinSize(win_size_in);

				// test checksum
				if(!checksum_correct_in){
					MinetSendToMonitor(MinetMonitoringEvent("forwarding packet to sock even though checksum failed"));
				}

				// get data length
				iph_in.GetTotalLength(total_length_in);
				data_length_in = total_length_in - iph_length_in - TCP_HEADER_BASE_LENGTH; // CHECK OPTIONS IF PROBLEM WITH LENGTHS

				// find matching connection
				ConnectionList<TCPState>::iterator cs = clist.FindMatching(c);

				// handle new connection
				if(cs==clist.end()) {

					ctsm_in.connection = c;
					ctsm_in.state.SetState(LISTEN);

					// get SYN, send SYN-ACK
					if(IS_SYN(flags_in) && !(IS_ACK(flags_in))) {
						cout << "receives SYN" << endl;
						cout << "TCP state: " << ctsm_in.state.GetState() << endl;

						// set outgoing variables
						seq_num_out = rand() % sizeof(int);
						ack_num_out = seq_num_in + 1;
						flags_out = 0x12; // SYN-ACK
						win_size_out = win_size_in;

						// initialize connection state
						ctsm_in.state.SetState(SYN_RCVD);
						ctsm_in.state.last_acked = seq_num_out;
						ctsm_in.state.last_sent = seq_num_out + 1;
						ctsm_in.state.last_recvd = 0;
						
						// construct outgoing ip header
						iph_out.SetProtocol(IP_PROTO_TCP);
						iph_out.SetSourceIP(c.src);
						iph_out.SetDestIP(c.dest);
						iph_out.SetTotalLength(40);
						p_out.PushFrontHeader(iph_out);				

						// construct outgoing tcp header
						tcph_out.SetSourcePort(c.srcport, p_out);
						tcph_out.SetDestPort(c.destport, p_out);
						tcph_out.SetSeqNum(seq_num_out, p_out);
						tcph_out.SetAckNum(ack_num_out, p_out);
						tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
						tcph_out.SetFlags(flags_out, p_out);
						tcph_out.SetWinSize(win_size_out,p_out);
						p_out.PushBackHeader(tcph_out);

						// send SYN-ACK
						MinetSend(mux, p_out);

						// add ctsm to clist
						clist.push_back(ctsm_in);

						cout << "TCP state: " << ctsm_in.state.GetState() << endl;
						cout << "sends SYN-ACK" << endl;
					}
				}

				// handle existing connection
				else  {

					// get SYN-ACK, send ACK
					if(IS_SYN(flags_in) && IS_ACK(flags_in)) {
						cout << "receives SYN-ACK" << endl;
						cout << "TCP state: " << cs->state.GetState() << endl;

						// set outgoing variables
						seq_num_out = ack_num_in;
						ack_num_out = seq_num_in + 1;
						flags_out = 0x10; // ACK
						win_size_out = win_size_in;

						// update connection state
						cs->state.SetState(ESTABLISHED);
						cs->state.last_acked = ack_num_in;
						cs->state.last_sent = seq_num_out + data_length_out;
						cs->state.last_recvd = ack_num_out;

						// construct outgoing ip header
						iph_out.SetProtocol(IP_PROTO_TCP);
						iph_out.SetSourceIP(c.src);
						iph_out.SetDestIP(c.dest);
						iph_out.SetTotalLength(40 + data_length_out);
						p_out.PushFrontHeader(iph_out);				

						// construct outgoing tcp header
						tcph_out.SetSourcePort(c.srcport, p_out);
						tcph_out.SetDestPort(c.destport, p_out);
						tcph_out.SetSeqNum(seq_num_out, p_out);
						tcph_out.SetAckNum(ack_num_out, p_out);
						tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
						tcph_out.SetFlags(flags_out, p_out);
						tcph_out.SetWinSize(win_size_out,p_out);
						p_out.PushBackHeader(tcph_out);

						// send ACK
						MinetSend(mux, p_out);

						// send socket response
						SockRequestResponse write(WRITE, cs->connection, payload_out, 0, EOK);
						MinetSend(sock, write);

						cout << "TCP state: " << cs->state.GetState() << endl;
						cout << "sends ACK" << endl;
					}

					// get data packet, send ACK
					if(IS_ACK(flags_in) && !IS_SYN(flags_in) && !IS_FIN(flags_in)) {
						
						
						cout << "receives potential data packet" << endl;
						cout << "TCP state: " << cs->state.GetState() << endl;

						// get incoming payload
						payload_in = p_in.GetPayload();
						cs->state.last_acked = ack_num_in;
						//cs->state.last_sent = seq_num_out + data_length_out;
						cs->state.last_sent += data_length_out;
						cs->state.last_recvd = seq_num_in + data_length_in;

						//SockRequestResponse write(WRITE, cs->connection, payload_in, payload_in.GetSize(), EOK);
						SockRequestResponse write(WRITE, cs->connection, payload_in, data_length_in, EOK);

						if(cs->state.GetState() == SYN_RCVD){
						
							cs->state.SetState(ESTABLISHED);
							MinetSend(sock, write);

						}else if(cs->state.GetState() == ESTABLISHED){
							
							if(data_length_in > 0) {
								data_in = (char*)malloc(data_length_in);
								payload_in.GetData(data_in, data_length_in, 0);
								// print received data
								cout << "data: " << data_in << endl;
							}
							else {
								continue;
							}
							seq_num_out = cs->state.last_sent;
							ack_num_out = seq_num_in + data_length_in;
							flags_out = 0x10; // ACK
							win_size_out = win_size_in;
							iph_out.SetProtocol(IP_PROTO_TCP);
							iph_out.SetSourceIP(c.src);
							iph_out.SetDestIP(c.dest);
							iph_out.SetTotalLength(40);
							p_out.PushFrontHeader(iph_out);		
	
							// construct outgoing tcp header
							tcph_out.SetSourcePort(c.srcport, p_out);
							tcph_out.SetDestPort(c.destport, p_out);
							tcph_out.SetSeqNum(seq_num_out, p_out);
							tcph_out.SetAckNum(ack_num_out, p_out);
							tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
							tcph_out.SetFlags(flags_out, p_out);
							tcph_out.SetWinSize(win_size_out,p_out);
							p_out.PushBackHeader(tcph_out);

							cout << "TCP state: " << cs->state.GetState() << endl;
							cout << "sends ACK" << endl;
	
							MinetSend(mux, p_out);
							MinetSend(sock, write);
						}else if(cs->state.GetState() == FIN_WAIT1){
							cout << "TCP state: FIN_WAIT2" << endl;
							cs->state.SetState(FIN_WAIT2);
							
						}else if(cs->state.GetState() == FIN_WAIT2){
							cout << "receiving data in FIN_WAIT2" << endl;
							MinetSend(sock, write);

						}else if(cs->state.GetState() == LAST_ACK){
							cout << "Lask ACK received. removing from list." << endl;
							clist.erase(cs);
							// done						

						}				

					}

					// get FIN, send ACK
					if(IS_FIN(flags_in)) {
						cout << "receives FIN" << endl;
						cout << "TCP state: " << cs->state.GetState() << endl;
						
						// set outgoing variables
						seq_num_out = cs->state.last_sent + data_length_out;
						ack_num_out = seq_num_in + data_length_in + 1;		// the +1 is just to stop netcat
						//flags_out = 0x11; // FIN-ACK
						flags_out = 0x10; // ACK							// this is just to stop netcat
						win_size_out = win_size_in;

						// update connection state
						
						if(cs->state.GetState() == ESTABLISHED){
							cout << "TCP state: CLOSE_WAIT" << endl;
							cs->state.SetState(CLOSE_WAIT);
						}else{
							cout << "removing from list" << endl;
							clist.erase(cs);
						}
						cs->state.last_acked = ack_num_in;
						cs->state.last_sent = seq_num_out + data_length_out;
						cs->state.last_recvd = seq_num_in + data_length_in;
						
						// construct outgoing ip header
						iph_out.SetProtocol(IP_PROTO_TCP);
						iph_out.SetSourceIP(c.src);
						iph_out.SetDestIP(c.dest);
						iph_out.SetTotalLength(40);
						p_out.PushFrontHeader(iph_out);				

						// construct outgoing tcp header
						tcph_out.SetSourcePort(c.srcport, p_out);
						tcph_out.SetDestPort(c.destport, p_out);
						tcph_out.SetSeqNum(seq_num_out, p_out);
						tcph_out.SetAckNum(ack_num_out, p_out);
						tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
						tcph_out.SetFlags(flags_out, p_out);
						tcph_out.SetWinSize(win_size_out,p_out);
						p_out.PushBackHeader(tcph_out);

						// send ACK
						MinetSend(mux, p_out);

						cout << "sends ACK" << endl;
					}
				} // END handle existing connection
			} // END handle mux event

			if (event.handle == sock) {
				// socket request or response has arrived
				MinetReceive(sock,req);

				// need to somehow check if this is a new/existing connection
				// because connection state needs to be updated/allocated/modified

				ConnectionList<TCPState>::iterator cs = clist.FindMatching(req.connection);			

				// new connection (all that is needed for handshake)
				if(cs == clist.end()) {

					switch(req.type) {

						case CONNECT:
							// Initial state for active open (client)
							// Fully bound connection
							// Send SYN packet
							// Update state to SYN_SENT
							{
								cout << "active open" << endl;

								// send sock response, STATUS
								SockRequestResponse statusRepl;
								statusRepl.type=STATUS;
								statusRepl.connection=req.connection;
								statusRepl.bytes=0;
								statusRepl.error=EOK;
								MinetSend(sock,statusRepl);		
								
								// add new ctsm
								ctsm_out.connection = req.connection;
								ctsm_out.state.SetState(CLOSED);
	
								cout << "TCP state: " << ctsm_out.state.GetState() << endl;

								// set outgoing variables
								seq_num_out = rand() % sizeof(int);
								//ack_num_out = seq_num_in + 1;
								flags_out = 0x02; // SYN
								win_size_out = win_size_in;

								// initialize connection state
								ctsm_out.state.SetState(SYN_SENT);
								ctsm_out.state.last_acked = 0;
								ctsm_out.state.last_sent = seq_num_out;
								ctsm_out.state.last_recvd = 0;
						
								// construct outgoing ip header
								iph_out.SetProtocol(IP_PROTO_TCP);
								iph_out.SetSourceIP(req.connection.src);
								iph_out.SetDestIP(req.connection.dest);
								iph_out.SetTotalLength(40);
								p_out.PushFrontHeader(iph_out);				

								// construct outgoing tcp header
								tcph_out.SetSourcePort(req.connection.srcport, p_out);
								tcph_out.SetDestPort(req.connection.destport, p_out);
								tcph_out.SetSeqNum(seq_num_out, p_out);
								//tcph_out.SetAckNum(ack_num_out, p_out);
								tcph_out.SetFlags(flags_out, p_out);
								tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
								tcph_out.SetWinSize(win_size_out,p_out);
								p_out.PushBackHeader(tcph_out);

								// send SYN
								MinetSend(mux, p_out);
								sleep(1);
								MinetSend(mux, p_out);

								// add ctsm to clist
								clist.push_back(ctsm_out);

								cout << "TCP state: " << ctsm_out.state.GetState() << endl;
								cout << "sends SYN" << endl;
								break;
							}

						case ACCEPT:
							// Initial state for passive open (server)
							// Half-bound connection (remote unspecified)
							// Allocate connection state and possibly buffer
							// Update state to LISTEN
							// Send STATUS to sock
							{
								// Allocate connection state and possibly buffer
								// this may not need done if connection already exists??

								// Update state to LISTEN
								//cs->state.SetState(LISTEN);
								//req.connection.state.SetState(LISTEN);
								printf("in first accept...\n");
								// Send STATUS to sock
								SockRequestResponse statusRepl;
								statusRepl.type=STATUS;
								statusRepl.connection=req.connection;
								statusRepl.bytes=0;
								statusRepl.error=EOK;
								MinetSend(sock,statusRepl);

								break;
							}

						case STATUS:
							{
								break;
							}

						case WRITE:
							{
								
								break;
							}

						case FORWARD:
							{
								// TCP ignores this case
								break;
							}

						case CLOSE:
							{
								
								
								break;
							}

						default:
							{
								SockRequestResponse repl;
								repl.type = STATUS;
								repl.error = EWHAT;
								MinetSend(sock,repl);
								break;
							}
					} // END switch case
				}else{ // END handle new connection
					switch(req.type) {
						case WRITE:
							{

								//cout << "Start WRITE case" << endl;

								// additional state
								int remaining = req.data.GetSize();
								int sent = 0;
								Buffer seg;
								
								// set packet information
								flags_out = 0x10;
								win_size_out = 5000;

								// common header information
								iph_out.SetProtocol(IP_PROTO_TCP);
								iph_out.SetSourceIP(req.connection.src);
								iph_out.SetDestIP(req.connection.dest);

								//char testString[1024];
								//req.data.GetData(testString, 1024, 0);
								//cout << "length of string: " << strlen(testString) << endl;

								int MAXDATASIZE = 536;
								int i=0;						
								while(remaining > 0){

									//cout << endl << "before extract" << endl;
									//cout << "remaining: " << remaining << endl;
									//cout << "sent: " << sent << endl;
									//cout << "req.data.GetSize(): " << req.data.GetSize() << endl;
									//cout << "seg.GetSize(): " << seg.GetSize() << endl;
									//req.data.GetData(testString, 1024, 0);									
									//cout << "length of string: " << strlen(testString) << endl;
									//cout << endl;
									
									if(remaining <= MAXDATASIZE){
										seg = req.data.Extract(0, remaining);
										iph_out.SetTotalLength(40+remaining);
									}
									else {
										seg = req.data.Extract(0, MAXDATASIZE);
										iph_out.SetTotalLength(MAXDATASIZE + 40);
									}

									// common information
									Packet p_out(seg);
									p_out.PushFrontHeader(iph_out);
									tcph_out.SetSourcePort(req.connection.srcport, p_out);
									tcph_out.SetDestPort(req.connection.destport, p_out);
									tcph_out.SetFlags(flags_out, p_out);
									tcph_out.SetWinSize(win_size_out, p_out);
									tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
									tcph_out.SetSeqNum(cs->state.GetLastSent(), p_out);
									int acknum = cs->state.GetLastRecvd();
									tcph_out.SetAckNum(acknum, p_out);

									if(remaining <= MAXDATASIZE) {
										cs->state.SetLastSent(cs->state.GetLastSent() + remaining);
										sent += remaining;
										remaining = 0;
									}
									else {
										cs->state.SetLastSent(cs->state.GetLastSent() + MAXDATASIZE);
										sent += MAXDATASIZE;										
										remaining -= MAXDATASIZE;
									}
									
									tcph_out.RecomputeChecksum(p_out);
									p_out.PushBackHeader(tcph_out);

									MinetSend(mux, p_out);
									cout << "sends packet " << i << endl;
									i++;									
								}
								
								//send STATUS to sock
								SockRequestResponse statusRepl;
								statusRepl.type = STATUS;
								statusRepl.connection=req.connection;
								statusRepl.bytes = req.data.GetSize();
								statusRepl.error=EOK;
								MinetSend(sock, statusRepl);
								break;
							}
						case CLOSE:
							{
								
								Packet p_out("", 0);
								
								iph_out.SetProtocol(IP_PROTO_TCP);
								iph_out.SetSourceIP(req.connection.src);
								iph_out.SetDestIP(req.connection.dest);
								iph_out.SetTotalLength(40);
								p_out.PushFrontHeader(iph_out);				

								// construct outgoing tcp header
								tcph_out.SetSourcePort(req.connection.srcport, p_out);
								tcph_out.SetDestPort(req.connection.destport, p_out);
								tcph_out.SetSeqNum(cs->state.GetLastSent(), p_out);
								tcph_out.SetAckNum(cs->state.GetLastAcked(), p_out);
								tcph_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p_out);
								flags_out = 1;
								tcph_out.SetFlags(flags_out, p_out);
								tcph_out.SetWinSize(5000, p_out);
								tcph_out.SetChecksum(tcph_out.ComputeChecksum(p_out));

								cs->state.SetLastSent(cs->state.GetLastSent());

								p_out.PushBackHeader(tcph_out);
								// send SYN
								MinetSend(mux, p_out);

								if(cs->state.GetState() == ESTABLISHED){
									cs->state.SetState(FIN_WAIT1);
								}else if(cs->state.GetState() == CLOSE_WAIT){
									cs->state.SetState(LAST_ACK);
								}
								
								break;
							}
						case ACCEPT:
							{
								break;
							}
						case STATUS:
							{
								break;
							}
						case CONNECT:
							{
								break;
							}
						case FORWARD:
							{
								break;
							}
					}
				} // END new connection else
			} // END handle sock event
		}

		if (event.eventtype == MinetEvent::Timeout) {
			// timeout ! probably need to resend some packets
		}

	}

	MinetDeinit();

	return 0;
}
