#include "../lib/microtcp.h"
#include "../utils/crc32.h"



microtcp_sock_t microtcp_socket (int domain, int type, int protocol) {
	microtcp_sock_t sock;
    	sock.sd = socket(domain, type, protocol);

    	if(sock.sd == -1) {
        	perror("Creation of socket");
        	sock.state = INVALID;
    	}else{
        	sock.state = UNKNOWN;
        	sock.init_win_size = 0 ;
        	sock.curr_win_size = 0;
        	sock.recvbuf = NULL;
        	sock.buf_fill_level = 0;
        	sock.cwnd =0;
        	sock.ssthresh =0;
        	sock.seq_number =0;
        	sock.ack_number =0;
        	sock.packets_send =0;
        	sock.packets_received =0;
        	sock.packets_lost =0;
        	sock.bytes_send =0;
        	sock.bytes_received =0;
        	sock.bytes_lost =0;
		memset(&sock.address, 0 , sizeof(struct  sockaddr));
		sock.address_len = 0;
    	}
    	return sock;
}








int microtcp_bind (microtcp_sock_t *socket, const struct sockaddr *address,
               socklen_t address_len){

    	if(socket->state == INVALID){
        	perror("Bind of socket");
        	return -1;
    	}
    	return bind(socket->sd, address, address_len);
}






/**
*				CONNECT (3way handshake)
*			Step 1: O client stelnei to 1o paketo (SYN)
*			Step 3: O client lamvanei to 2o paketo (SYN,ACK) kai stelnei to 3o paketo (ACK)
*/


int microtcp_connect (microtcp_sock_t *socket, const struct sockaddr *address,
                  socklen_t address_len){

	microtcp_header_t* header = malloc(sizeof(microtcp_header_t));
	uint32_t tmp_seq, tmp_ack;
	uint16_t tmp_win;

	/*Elegxos malloc*/
	if(header == NULL){
		perror("Connect: Header Memory Allocation");
		return -1;
	}// temp_ack 

	srand((uint32_t)time(NULL));

	/*Elegxos socket*/
	if (socket->state != UNKNOWN) {
        	perror("Connect: Invalid socket");
        	return -1;
    	}




        /************ ARXI 1ou STEP *************/

    	/*Dhmiourgia 1ou PAKETO*/
    	header_init(header);
    	header->seq_number = (rand()% (10000 - 1000 + 1)) + 1000;
    	header->control = SYN;
    	header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

	tmp_seq = header->seq_number;		// tmp_seq = N

	printf("\nStelnw to 1o paketo (3way handshake)\n");
    	header_print(header);
    	printf("\n");

    	/*  Metatropi se network byte order  */
    	header_hton(header);

	/*  Apostoli 1ou paketoy */
    	if(sendto(socket->sd, header, sizeof(microtcp_header_t),0,address ,address_len) == -1 ) {		// seq=N, ack=0
        	perror("Connect: Step1 Send");
        	socket->state = INVALID;
        	return -1;
   	}
	/************ TELOS 1ou STEP ************/







        /************ ARXI 3ou STEP *************/

	/*   Lhpsh 2ou paketoy */
    	if(recvfrom(socket->sd,header, sizeof(microtcp_header_t), 0, (struct sockaddr*)address, &address_len) == -1){		// seq=M, ack=N+1
		perror("Connect: Step3 Recieve");
        	socket->state = INVALID;
        	return -1;
	}

	/* Metatropi se host byte order*/
    	header_ntoh(header);

	printf("\nElava to 2o paketo (3way handshake)\n");
    	header_print(header);
    	printf("\n");

    	/* Elegxoi 2ou paketoy*/
    	if(!check_sum(header)){
        	perror("Connect: Step3 Checksum");
        	socket->state = INVALID;
        	return -1;
    	}

    	if(header->control != SYN_ACK){
        	perror("Connect: Step3 Control");
        	socket->state = INVALID;
        	return -1;
    	}

    	if(header->ack_number != (tmp_seq + 1)){
        	perror("Connect: Step3 Seq_number");
        	socket->state = INVALID;
        	return -1;
    	}

	tmp_seq = header->seq_number;		// tmp_seq = M
	tmp_ack = header->ack_number;		// tmp_ack = N+1
	tmp_win = header->window;

	/*  Dimiourgia 3o paketou */
    	header_init(header);
    	header->control =ACK;
    	header->seq_number = tmp_ack;		//
    	header->ack_number = tmp_seq + 1;   //M+1
    	header->window = tmp_win;
    	header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

	printf("\nStelnw to 3o paketo (3way handshake)\n");
    	header_print(header);
    	printf("\n");

	/*Metatropi header se network byte order */
	header_hton(header);

    	/*  Apostolh tritou paketou */
    	if(sendto(socket->sd, header, sizeof(microtcp_header_t),0,address ,address_len) == -1 ){		// seq=N+1, ack=M+1
        	perror("Connect: Step3 Send");
        	socket->state = INVALID;
        	return -1;
 	}
        /************ ARXI 3ou STEP *************/





    	/*Arxikopoihsh socket*/
	socket->caller = CLIENT;
    	socket->state = ESTABLISHED;
	socket->ssthresh = MICROTCP_INIT_SSTHRESH; 
	socket->cwnd = MICROTCP_INIT_CWND;    
	socket->seq_number = tmp_seq+1;		// seq = N+2
    socket->ack_number = tmp_ack;		// ack = M+1 = 79
	socket->init_win_size = tmp_win;
	socket->curr_win_size = tmp_win;
	socket->address = *address;
	socket->address_len = address_len;
	socket->recvbuf = (uint8_t*) calloc(MICROTCP_RECVBUF_LEN,sizeof(uint8_t));

	/*Elegxos calloc*/
	if(socket->recvbuf == NULL){
		perror("Connect: Recvbuf Memory Allocation");
		return -1;
	}

	free(header);
	return 0;
}








/**
*                               ACCEPT (3way handshake)
*                       Step 2: O server lamvanei to 1o paketo (SYN) kai stelnei to 2o paketo (SYN,ACK)
*                       Step 4: O server lamvanei to 3o paketo (ACK)
*/
int microtcp_accept (microtcp_sock_t *socket, struct sockaddr *address,
                 socklen_t address_len){

	microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
	uint32_t tmp_seq, tmp_ack;

	/*Elegxos malloc*/
	if(header == NULL){
		perror("Accept: Header Memory allocation");
		return -1;
	}

	srand((uint32_t)time(NULL));

        /************ ARXI 2ou STEP *************/

    	/* Lhpsh 1ou paketou*/
	if(recvfrom(socket->sd, header, sizeof(microtcp_header_t), 0, address, &address_len) == -1){		// seq=N, ack=0
        	perror("Accept: Step2 Recieve");
        	socket->state = INVALID;
        	return -1;
    	}

	/*Metatropi se host byte order*/
    	header_ntoh(header);

   	printf("\nElava to 1o paketo (3way handshake)\n");
   	header_print(header);
    	printf("\n");

    	/*  Elegxoi prwtou paketou*/
    	if(!check_sum(header)){
        	socket->state = INVALID;
        	return -1;
    	}
	if(header->control != SYN){
        	socket->state = INVALID;
        	return -1;
    	}

	tmp_seq = header->seq_number;		// tmp_seq = N

    	/* Dimiourgia 2ou paketou*/
	header_init(header);
   	header->seq_number = (rand()% (20000 - 11000 + 1)) + 1000;
	header->ack_number = tmp_seq + 1;
    	header->control = SYN_ACK;
    	header->window = MICROTCP_WIN_SIZE;
    	header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

    	tmp_seq = header->seq_number;		// tmp_seq = M
    	tmp_ack = header->ack_number;		// tmp_ack = N+1

    	printf("\nStelnw to 2o paketo (3way handshake)\n");
    	header_print(header);
    	printf("\n");

    	/*Metatropi se network byte order */
	header_hton(header);

    	/*  Apostoli 2ou paketou*/
    	if(sendto(socket->sd, header, sizeof(microtcp_header_t), 0, address, address_len) == -1){			// seq=M, ack=N+1
        	socket->state = INVALID;
        	return -1;
    	}
	/************ TELOS 2ou STEP ************/

	/************ ARXI 4ou STEP *************/

	/*  Lhpsh 3ou paketou */
    	if(recvfrom(socket->sd, header, sizeof(microtcp_header_t), 0, address, &address_len) == -1){		// seq = N+1, ack = M+1
        	socket->state = INVALID;
        	return -1;
    	}

        /* Metatropi se host byte order */
    	header_ntoh(header);

    	printf("\nElava to 3o paketo (3way handshake)\n");
    	header_print(header);
    	printf("\n");

    	/* Elegxos 3ou paketou*/
    	if(!check_sum(header)){
        	perror("checksum");
        	socket->state = INVALID;
        	return -1;
    	}
    	if(header->control != ACK) {
        	perror("ACK");
        	socket->state = INVALID;
        	return -1;
    	}
    	if(header->seq_number != tmp_ack) {
        	perror("seq");
        	socket->state = INVALID;
        	return -1;
    	}

    	if(header->ack_number != (tmp_seq + 1)) {
        	socket->state =INVALID;
        	return -1;
    	}

    	/*  Arxikopoihsh tou socket**/
	socket->caller = SERVER;
    socket->state = ESTABLISHED;
	socket->ssthresh = MICROTCP_INIT_SSTHRESH; 
	socket->cwnd = MICROTCP_INIT_CWND;	
	socket->init_win_size = MICROTCP_WIN_SIZE;
    socket->curr_win_size = MICROTCP_WIN_SIZE;
    socket->seq_number = header->ack_number;
    socket->ack_number = header->seq_number + 1;
	socket->address = *address;
	socket->address_len = address_len;
	socket->recvbuf = (uint8_t*) calloc(MICROTCP_RECVBUF_LEN,sizeof(uint8_t));

        /*Elegxos calloc*/
        if(socket->recvbuf == NULL){
                perror("Connect: Recvbuf Memory Allocation");
                return -1;
        }

	free(header);
    	return socket->sd;
}














int microtcp_shutdown (microtcp_sock_t *socket, int how){
	srand((uint32_t)time(NULL));

	microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
	uint32_t tmp_seq, tmp_ack;

	/*Elegxos malloc*/
	if(header == NULL){
		perror("Memory allocation error in accept h1");
		return -1;
	}

	/*********** CLIENT ***********/
	if(socket->caller == CLIENT){		//socket->state == ESTABLISHED	(2i fasi)

			/*Dimiourgia 1ou paketou*/
		header_init(header);
		header->control = FIN_ACK;
		header->seq_number = (uint32_t)socket->seq_number+1;
		header->window = socket->curr_win_size;
		header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

		tmp_seq = header->seq_number;		// tmp_seq = X

		printf("\nStelnw to 1o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Metatropi se network byte order*/
		header_hton(header);

		/*Apostoli 1ou paketou*/
		if(sendto(socket->sd, header, sizeof(microtcp_header_t), 0, &socket->address, socket->address_len) == -1 ){	// seq=X, ack=0 
			socket->state=INVALID;
			perror("Shutdown Packet1 Send");
			return -1;
		}

			/*Lipsi 2ou paketou*/
		if(recvfrom(socket->sd, header,sizeof(microtcp_header_t), 0, &socket->address, &socket->address_len) == -1){	// seq=0, ack=X+1
			socket->state=INVALID;
			perror("Shutdown Packet2 Recieve");
			return -1;
		}

		/*Metatropi se host byte order*/
		header_ntoh(header);

		printf("\nElava to 2o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Elegxos 2ou paketou*/
		if(!check_sum(header)) {
        		perror("Shutdown Packet2 Recieve CHECKSUM");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->control != ACK) {
        		perror("Shutdown Packet2 Recieve CONTROL");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->ack_number != (tmp_seq +1)) {
        		perror("Shutdown Packet2 Recieve ACK NUMBER");
        		socket->state = INVALID;
        		return -1;
    		}

		/*Allagi state sto socket tou client*/
		socket->state = CLOSING_BY_HOST;

		/*Lipsi 3ou paketou*/
		if(recvfrom(socket->sd, header ,sizeof(microtcp_header_t), 0, &socket->address, &socket->address_len) == -1){	// seq=Y, ack=0
			socket->state=INVALID;
			perror("Shutdown Packet3 Recieve");
			return -1;
		}

		/*Metatropi se host byte order*/
		header_ntoh(header);

		printf("\nElava to 3o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		tmp_ack = header->seq_number;		// temp_ack = Y

		/*Elegxos 3ou paketou*/
		if(!check_sum(header)) {
        		perror("Shutdown Packet3 Recieve CHECKSUM");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->control != FIN_ACK) {
        		perror("Shutdown Packet3 Recieve CONTROL");
        		socket->state = INVALID;
        		return -1;
    		}

		/*Dimiourgia 4ou paketou*/
		header_init(header);
		header->control = ACK;
		header->ack_number = tmp_ack + 1;
		header->seq_number = tmp_seq + 1;
		header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

		printf("\nStelnw to 4o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Metatropi se network byte order*/
		header_hton(header);

		/*Apostoli 4ou paketou*/
		if(sendto(socket->sd, header, sizeof(microtcp_header_t), 0, &socket->address, socket->address_len) == -1 ){   	// seq=X+1, ack=Y+1
			socket->state=INVALID;
			perror("Shutdown Packet4 Send");
			return -1;
		}


	}
	/*********** SERVER ***********/
	else if (socket->caller == SERVER){		//socket->state == CLOSING_BY_PEER  (2i fasi)
		header_init(header);

		/*Lipsi 1ou paketou**/
		if(recvfrom(socket->sd, header,sizeof(microtcp_header_t), 0, &socket->address, &socket->address_len) == -1){	// seq=X, ack=0
			socket->state=INVALID;
			perror("Shutdown Packet1 Recieve");
			return -1;
		}

		/*Metatropi se host byte order*/
		header_ntoh(header);

		printf("\nElava to 1o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		tmp_seq = header->seq_number;		// tmp_seq = X

		/*Elegxos 1ou paketou*/
		if(!check_sum(header)) {
        		perror("Shutdown Packet1 Recieve CHECKSUM");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->control != FIN_ACK) {
        		perror("Shutdown Packet1 Recieve CONTROL");
        		socket->state = INVALID;
        		return -1;
    		}

		/*Dimiourgia 2ou paketou*/
		header_init(header);
		header->control = ACK;
		header->ack_number = tmp_seq + 1;
		header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

		printf("\nStelnw to 2o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Metatropi se network byte order*/
		header_hton(header);

		/*Apostoli 2ou paketou*/
		if(sendto(socket->sd, header, sizeof(microtcp_header_t), 0, &socket->address, socket->address_len) == -1 ){	// seq=0, ack=X+1
			socket->state=INVALID;
			perror("Shutdown Packet2 Send");
			return -1;
		}

		/*Allagi sto state tou socket tou server*/
		socket->state = CLOSING_BY_PEER;

		/*Dimiourgia 3ou paketou*/
		header_init(header);
		header->control = FIN_ACK;
		header->seq_number = (rand()% (11000 - 1000 + 1)) + 1000;
		header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));

		tmp_ack = header->seq_number;		// temp_ack = Y

		printf("\nStelnw to 3o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Metatropi se network byte order*/
		header_hton(header);

		/*Apostoli 3ou paketou*/
		if(sendto(socket->sd, header, sizeof(microtcp_header_t), 0, &socket->address, socket->address_len) == -1 ){	// seq=Y, ack=0
			socket->state=INVALID;
			perror("Shutdown Packet3 Send");
			return -1;
		}

		/*Lipsi 4ou paketou **/
		if(recvfrom(socket->sd, header,sizeof(microtcp_header_t), 0, &socket->address, &socket->address_len) == -1){	// seq=X+1, ack=Y+1
			socket->state=INVALID;
			perror("Shutdown Packet4 Recieve");
			return -1;
		}

		/*Metatropi se host byte order*/
		header_ntoh(header);

		printf("\nElava to 4o paketo (shutdown)\n");
    		header_print(header);
    		printf("\n");

		/*Elegxoi 4ou paketou*/
		if(!check_sum(header)) {
        		perror("Shutdown Packet4 Recieve CHECKSUM");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->control != ACK) {
        		perror("Shutdown Packet4 Recieve CONTROL");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->seq_number != (tmp_seq +1)) {
        		perror("Shutdown Packet4 Recieve SEQ NUMBER");
        		socket->state = INVALID;
        		return -1;
    		}
		if(header->ack_number != (tmp_ack +1)) {
        		perror("Shutdown Packet4 Recieve ACK NUMBER");
        		socket->state = INVALID;
        		return -1;
    		}
	}else{
		return -1;
	}

	socket->state = CLOSED;
	//shutdown(socket->sd, how);

	free(header);
	free(socket->recvbuf);
	return 0;
}











ssize_t microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length,
               int flags)
{	
	size_t remaining, data_sent, bytes_to_send, chunks;
	microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
	void *segment = malloc(MICROTCP_MSS);
	int ack_count = 0;



	data_sent = 0;
	remaining = length;	


	while(data_sent < length ){		


	

		bytes_to_send = min(socket->curr_win_size , socket->cwnd , remaining ); 
		chunks = bytes_to_send / MICROTCP_MSS ;  



		for(int i = 0; i < chunks; i++){	
			header_init(header);

			header->seq_number = (uint8_t) socket->seq_number + data_sent + i*(MICROTCP_MSS - sizeof(microtcp_header_t));	
			header->ack_number = (uint8_t) socket->ack_number + ack_count;
			
			header->data_len = MICROTCP_MSS - sizeof(microtcp_header_t);	


			memcpy(segment, header, sizeof(microtcp_header_t));
			

			memcpy(segment + sizeof(microtcp_header_t), buffer+data_sent , MICROTCP_MSS - sizeof(microtcp_header_t) );

			header->checksum = crc32(segment, sizeof(microtcp_header_t));
			header_hton(header);
			memcpy(segment, header, sizeof(microtcp_header_t));

			

			if( sendto(socket->sd, segment, MICROTCP_MSS, 0, &socket->address, socket->address_len) == -1 ) {
				socket->state=INVALID;
				perror("Shutdown Packet3 Send");
				return -1;
			}


		}
		data_sent =+ bytes_to_send;
		free(segment);
		segment = malloc(bytes_to_send % MICROTCP_MSS);
	
		if(bytes_to_send % MICROTCP_MSS){ 
			
			header_init(header);
			header->seq_number = (uint8_t) socket->seq_number + data_sent + chunks*(MICROTCP_MSS - sizeof(microtcp_header_t));
			chunks ++;

			header->data_len = bytes_to_send % MICROTCP_MSS;

			memcpy(segment, header, sizeof(microtcp_header_t));
			

			memcpy(segment + sizeof(microtcp_header_t), buffer+data_sent , MICROTCP_MSS - sizeof(microtcp_header_t) );

			header->checksum = crc32(segment, sizeof(microtcp_header_t));
			header_hton(header);
			memcpy(segment, header, sizeof(microtcp_header_t));

			

			if( sendto(socket->sd, segment, MICROTCP_MSS, 0, &socket->address, socket->address_len) == -1 ) {
				socket->state=INVALID;
				perror("Shutdown Packet3 Send");
				return -1;
			}

		}

		
		microtcp_header_t * recv_header = malloc(sizeof(microtcp_header_t));
        int retrasmit = 0;
        uint32_t temp_seq = 0, temp_ack = 0;
        int counter = 0;
    
    
        for (int i = 0; i < chunks; i++){

            if(recvfrom(socket->sd, recv_header, sizeof(microtcp_header_t), 0, &socket->address, &socket->address_len) == -1){
                perror("Time out");
                retrasmit = 1;
                break;
            }
            header_ntoh(recv_header);
            if(recv_header->control != ACK) {
                perror("NOT ACK");
            }
            else  {               
                if (header->ack_number == temp_ack || header->seq_number == temp_seq) {
                    perror("Wrong ack or seq");
                    i -= 2;
                    counter++;
                }
                else {

                    temp_seq = header->seq_number;  
                    temp_ack = header->ack_number;    
                    counter = 0;
                }
            }
        }


		/* Retransmissions */
		/* Update window */
		/* Update congestion control */
		socket->curr_win_size = header->window;
		remaining -= (bytes_to_send - chunks*sizeof(microtcp_header_t));
		data_sent += (bytes_to_send - chunks*sizeof(microtcp_header_t));
		ack_count++;

		}	
    return 0;
}





ssize_t microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags)
{
    
    void* recv_segment = malloc(MICROTCP_MSS);
    microtcp_header_t* header = malloc(sizeof(microtcp_header_t));
    
    ssize_t total_data = 0;
    
    //while(1) {
        
        if (recvfrom(socket->sd, recv_segment, MICROTCP_MSS, 0, &socket->address, &socket->address_len) == -1){
            perror("Time out");
            return  0;
        }
        
        memcpy(header, recv_segment, sizeof(microtcp_header_t));
        
        header_ntoh(header);
        header_print(header);
        if (header->control == FIN_ACK) {
        
            return total_data;
        }
        memcpy(buffer, recv_segment + 32, MICROTCP_MSS - 32);
        total_data+= header->data_len;
        
   // }
    
    return total_data;
}











void header_init(microtcp_header_t *header){
    	header->seq_number =0;
    	header->ack_number = 0;
    	header->control = 0;
    	header->window = 0;
    	header->data_len = 0;
    	header->future_use0 = 0;
    	header->future_use1 = 0;
    	header->future_use2 = 0;
    	header->checksum = 0;
}






void header_print(microtcp_header_t* header) {
    	printf("seq_number = %u\n",header->seq_number);
    	printf("ack_number = %u\n",header->ack_number);
    	printf("control = %d\t window = %d\n",header->control, header->window);
    	printf("data_let = %u\n",header->data_len);
    	printf("future_use0 = %u\n",header->future_use0);
    	printf("future_use1 = %u\n",header->future_use1);
    	printf("future_use2 = %u\n",header->future_use2);
    	printf("checksum = %u\n",header->checksum);
}







void header_ntoh(microtcp_header_t* header){
    	header->seq_number = ntohl(header->seq_number);
    	header->ack_number = ntohl(header->ack_number);
    	header->control = ntohs(header->control);
    	header->window = ntohs(header->window);
    	header->data_len = ntohl(header->data_len);
    	header->future_use0 = ntohl(header->future_use0);
    	header->future_use1 = ntohl(header->future_use1);
    	header->future_use2 = ntohl(header->future_use2);
    	header->checksum = ntohl(header->checksum);
}





void header_hton(microtcp_header_t* header){
    	header->seq_number = htonl(header->seq_number);
    	header->ack_number = htonl(header->ack_number);
    	header->control = htons(header->control);
    	header->window = htons(header->window);
    	header->data_len = htonl(header->data_len);
    	header->future_use0 = htonl(header->future_use0);
    	header->future_use1 = htonl(header->future_use1);
    	header->future_use2 = htonl(header->future_use2);
    	header->checksum = htonl(header->checksum);
}







int check_sum(microtcp_header_t* header){
	uint32_t check = header->checksum;

	header->checksum = 0;
	header->checksum = crc32((uint8_t*)header, sizeof(microtcp_header_t));
	if(header->checksum != check) {
        	return 0;
    	}
    	return 1;
}

size_t min(size_t flow_cntrl_win, size_t cwnd, size_t remaining){
	size_t a = flow_cntrl_win;

	if(a > cwnd){
		a = cwnd;
	}
	if(a > remaining){
		a = remaining;
	}

	return a;
}
