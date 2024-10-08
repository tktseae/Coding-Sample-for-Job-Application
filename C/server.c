#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER "127.0.0.1"
#define SERVER_PORT 5000

#define CLIENT_BASE_PORT 6000
#define MAX_NUM 100

#define MAXMSG 1400
#define OP_SIZE 20

#define SEQ0 0
#define SEQ1 1

#define REGISTER "REGISTER"
#define UPDATE "UPDATE"
#define QUERY "QUERY"
#define RESPONSE "RESPONSE"
#define FINISH "FINISH"
#define ACK "ACK"

#define TIMEOUT 500000		/* 1000 ms */


/* This structure can be used to parse packet */
struct ip_port {
	unsigned int ip;
	unsigned short port;
};


struct node{
	unsigned int ip;        /* ip and port are used as index */
	unsigned short port;    /* the port number of TCP listening */
	unsigned int file_map;  /* a bitmap for files */
	struct node *next;
};


struct rdt3_sender_ctx{
	unsigned int ip;          /* ip and port are used as index */
	unsigned short port;      /* the port number of current socket */
	char waiting_ack;         /* if waiting ack */
	char noack_num;           /* waiting ack num */
	char file_idx;            /* file index for QUERY & RESPONSE */
	long long clock;          /* the clock time of last packet sending, for timeout */
	struct node *noack_node;  /* waiting ack node, maybe retransmitted */
	/*struct node *node_info;   [> related node info <]*/
	struct rdt3_sender_ctx *next;
};


long long get_current_time() {
  struct timeval current_time;

  // Get the current time with microsecond precision
  if (gettimeofday(&current_time, NULL) == -1) {
      perror("gettimeofday");
      return 1;
  }

  // Calculate the total milliseconds
  long long total_microseconds = current_time.tv_sec * 1000000LL + current_time.tv_usec;
	return total_microseconds;
}


int set_timeout(int sockfd, int usec) {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usec;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			(struct timeval *)&tv, sizeof(struct timeval));
	if (ret == SO_ERROR) {
		return -1;
	}
	return 0;
}


int unset_timeout(int sockfd) {
	return set_timeout(sockfd, 0);
}


// Initialize an empty linked list
struct node* init_node_list() {
    return NULL; // Return NULL indicating an empty list
}

// Insert a new node at the end of the linked list
void insert_node(struct node **head, unsigned ip, unsigned short port, unsigned file_map) {
    // Create a new node
    struct node* newnode = (struct node*)malloc(sizeof(struct node));
    if (newnode == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    newnode->ip = ip;
    newnode->port = port;
    newnode->file_map = file_map;
    newnode->next = NULL;

    // If the list is empty, set the new node as the head
    if (*head == NULL) {
        *head = newnode;
        return;
    }

    // Traverse the list to find the last node
    struct node* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = newnode;
}


// Query a node in the linked list by index
struct node* query_node(struct node *head, unsigned ip, unsigned short port) {
    // Check if the list is empty
    if (head == NULL) {
        return NULL;
    }

    struct node* current = head;

    while (current != NULL && (current->ip != ip || current->port != port) ) {
        current = current->next;
    }

    // If the index is out of bounds, return NULL
    if (current == NULL) {
        return NULL;
    }

    return current;
}


// Initialize an empty linked list
struct rdt3_sender_ctx* init_rdt3_sender_ctx_list() {
    return NULL; // Return NULL indicating an empty list
}


// Insert a new rdt3_sender_ctx at the end of the linked list
void insert_ctx(struct rdt3_sender_ctx **head, unsigned ip, unsigned short port) {
    // Create a new rdt3_sender_ctx
    struct rdt3_sender_ctx* newrdt3_sender_ctx = (struct rdt3_sender_ctx*)malloc(sizeof(struct rdt3_sender_ctx));
    if (newrdt3_sender_ctx == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    newrdt3_sender_ctx->ip = ip;
    newrdt3_sender_ctx->port = port;
    newrdt3_sender_ctx->next = NULL;

    // If the list is empty, set the new rdt3_sender_ctx as the head
    if (*head == NULL) {
        *head = newrdt3_sender_ctx;
        return;
    }

    // Traverse the list to find the last rdt3_sender_ctx
    struct rdt3_sender_ctx* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    // Insert the new rdt3_sender_ctx at the end
    current->next = newrdt3_sender_ctx;
}



// Query a rdt3_sender_ctx in the linked list by index
struct rdt3_sender_ctx* query_ctx(struct rdt3_sender_ctx *head, unsigned ip, unsigned short port) {
    // Check if the list is empty
    if (head == NULL) {
        return NULL;
    }

    struct rdt3_sender_ctx* current = head;

    while (current != NULL && (current->ip != ip || current->port != port) ) {
        current = current->next;
    }

    // If the index is out of bounds, return NULL
    if (current == NULL) {
        return NULL;
    }

    return current;
}


/* Query the ip and port list */
struct node* send_return(int sockfd, struct sockaddr_in cltaddr, char file_idx, struct node* current, char seq) {
	unsigned int ip;
	unsigned short port;
	unsigned int bitmap = (1U << 31) >> file_idx;

	/***********************************************
	 * Start from current node to search for the linked
	 * node list. Until you find a node that has the
	 * file.
	 *
	 * START YOUR CODE HERE
	 **********************************************/
  // printf("=== send_return bitmap %d\n", bitmap);

  if(current == NULL) {
    return NULL;
  }

  while (current != NULL && (current->file_map & bitmap) == 0) {
    current = current->next;
  }

  // If the index is out of bounds, return NULL
  if (current == NULL) {
    return NULL;
  }

  // printf("=== send_return sendto\n");

	/***********************************************
	 * END OF YOUR CODE
	 **********************************************/

	char buffer[MAXMSG];
	
	bzero(buffer, MAXMSG);

	/* Compose send buffer: REGISTER IP Port */
	int total_len = 0;

	memcpy(buffer, &seq, sizeof(seq));
	total_len ++; /* add a seq */

	buffer[total_len] = ' ';
	total_len ++; /* add a blank */

	memcpy(buffer + total_len, RESPONSE, strlen(RESPONSE));
	total_len += strlen(RESPONSE);

	buffer[total_len] = ' ';
	total_len ++; /* add a blank */

	memcpy(buffer + total_len, &current->ip, sizeof(current->ip));
	total_len += sizeof(current->ip);

	memcpy(buffer + total_len, &current->port, sizeof(current->port));
	total_len += sizeof(current->port);

	buffer[total_len] = '\0';

	sendto(sockfd, (const char *)buffer, total_len,
		0, (const struct sockaddr *) &cltaddr, sizeof(cltaddr));
	

	return current;
}


/* Send finish to the client */
int send_finish(int sockfd, struct sockaddr_in servaddr, char seq) {
	char buffer[MAXMSG];
	
	bzero(buffer, MAXMSG);

	/* Compose send buffer: REGISTER IP Port */
	int total_len = 0;

	memcpy(buffer, &seq, sizeof(seq));
	total_len ++; /* add a seq */

	buffer[total_len] = ' ';
	total_len ++; /* add a blank */

	memcpy(buffer + total_len, FINISH, strlen(FINISH));
	total_len += strlen(FINISH);

	buffer[total_len] = ' ';
	total_len ++; /* add a blank */

	buffer[total_len] = '\0';

	sendto(sockfd, (const char *)buffer, total_len,
		0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	
	return 0;
}


int check_timeout(long long now, struct rdt3_sender_ctx *ctx_head, int sockfd) {
	struct rdt3_sender_ctx *current = ctx_head;

  if (current == NULL) {
      return 0;
  }

  while (current != NULL) {

    /***********************************************
    * Traverse the context list and find all the
    * waiting for ACK and timeout packets. Resend
    * them.
    * Hints: the context has the packet sent time,
    * waiting status. You also have the current time
    * and TIMEOUT value.
    *
    * START YOUR CODE HERE
    **********************************************/
    if(current->waiting_ack && now - current->clock >= TIMEOUT) {
      char buffer[MAXMSG];
      
      bzero(buffer, MAXMSG);

      /* Compose send buffer: REGISTER IP Port */
      int total_len = 0;

      memcpy(buffer, &current->noack_num, sizeof(current->noack_num));
      total_len ++; /* add a seq */

      buffer[total_len] = ' ';
      total_len ++; /* add a blank */

      memcpy(buffer + total_len, RESPONSE, strlen(RESPONSE));
      total_len += strlen(RESPONSE);

      buffer[total_len] = ' ';
      total_len ++; /* add a blank */

      memcpy(buffer + total_len, &current->noack_node->ip, sizeof(current->noack_node->ip));
      total_len += sizeof(current->noack_node->ip);

      memcpy(buffer + total_len, &current->noack_node->port, sizeof(current->noack_node->port));
      total_len += sizeof(current->noack_node->port);

      // printf("=== check_timeout noack_node->ip %d, noack_node->port %d\n", 
      //   current->noack_node->ip, current->noack_node->port);

      buffer[total_len] = '\0';

	    struct sockaddr_in cltaddr;
      // Filling server information
      cltaddr.sin_family = AF_INET; // IPv4
      cltaddr.sin_addr.s_addr = current->ip;
      cltaddr.sin_port = htons(current->port);

      current->clock = get_current_time();
      sendto(sockfd, (const char *)buffer, total_len,
        0, (const struct sockaddr *) &cltaddr, sizeof(cltaddr));
    }
    /***********************************************
    * END OF YOUR CODE
    **********************************************/

    current = current->next;
  }

	return 0;
}


int main() {
	int sockfd;
	char buffer[MAXMSG];

	struct sockaddr_in servaddr, clientaddr;
	
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&clientaddr, 0, sizeof(clientaddr));
	
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(SERVER_PORT);
	
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
				sizeof(servaddr)) < 0 )
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}
	
	unsigned int len;
	int n;

	struct node * node_head = init_node_list();                     // NULL
	struct rdt3_sender_ctx *ctx_head = init_rdt3_sender_ctx_list(); // NULL

	struct rdt3_sender_ctx *current_ctx;

	unsigned parse_idx = 0;
	char seq;

	char send_buf[MAXMSG];
	unsigned send_idx = 0;

	set_timeout(sockfd, TIMEOUT);

	printf("The server is listening ...\n");

	while (1) {
		len = sizeof(clientaddr); // len is value/result

		n = recvfrom(sockfd, (char *)buffer, MAXMSG,
					0, (struct sockaddr *) &clientaddr, &len);

		if (n < 0) {
			// Checking timeout
			long long now = get_current_time();
			check_timeout(now, ctx_head, sockfd);

			continue;
		}

		buffer[n] = '\0';

		// First, try to recover context based on ip port
		unsigned ip = clientaddr.sin_addr.s_addr;
		unsigned short port = ntohs(clientaddr.sin_port);

		current_ctx = query_ctx(ctx_head, ip, port);

		if (current_ctx == NULL) {
			insert_ctx(&ctx_head, ip, port);
		}

		seq = buffer[0];
		parse_idx += 2; /* skip seq and blank */

		// Second, parse incoming packets
		// REGISTER
		if (strncmp(buffer + parse_idx, REGISTER, strlen(REGISTER)) == 0) {
			unsigned dst_ip;
			unsigned short dst_port;

			parse_idx += strlen(REGISTER);
			parse_idx ++; /*skip blank */

			memcpy(&dst_ip, buffer + parse_idx, sizeof(dst_ip));
			parse_idx += sizeof(dst_ip);

			memcpy(&dst_port, buffer + parse_idx, sizeof(dst_port));

      // printf("=== det_ip %d, det_port %d\n", dst_ip, dst_port);

			if (query_node(node_head, dst_ip, dst_port) == NULL) {
        // printf("=== REGISTER Success!\n");
				insert_node(&node_head, dst_ip, dst_port, 0);
			} else {
				printf("REGISTER Failed: node already exists!\n");
			}

			/* send ACK packet */
			memcpy(send_buf, &seq, sizeof(seq));
			send_idx += 2; /* seq and blank */

			memcpy(send_buf + send_idx, ACK, strlen(ACK));
			send_idx += strlen(ACK);

			sendto(sockfd, (const char *)send_buf, send_idx,
				0, (const struct sockaddr *) &clientaddr, sizeof(clientaddr));
		}


		// UPDATE
		if (strncmp(buffer + parse_idx, UPDATE, strlen(UPDATE)) == 0) {
			unsigned dst_ip;
			unsigned short dst_port;
			unsigned new_map;

			/***********************************************
			 * Refer to REGISTER implementation above and
			 * the UPDATE protocol description. Dealing with
			 * UPDATE packet
			 *
			 * START YOUR CODE HERE
			 **********************************************/
			parse_idx += strlen(UPDATE);
			parse_idx ++; /*skip blank */

			memcpy(&dst_ip, buffer + parse_idx, sizeof(dst_ip));
			parse_idx += sizeof(dst_ip);

			memcpy(&dst_port, buffer + parse_idx, sizeof(dst_port));
      parse_idx += sizeof(dst_port);

			memcpy(&new_map, buffer + parse_idx, sizeof(new_map));

      // printf("=== det_ip %d, det_port %d, new_map %x\n", dst_ip, dst_port, new_map);

			/***********************************************
			 * END OF YOUR CODE
			 **********************************************/

			struct node *update_node = query_node(node_head, dst_ip, dst_port);

			if (update_node == NULL) {
				printf("UPDATE Failed: node does not exists!");
			} else {
				/***********************************************
				 * Update the file_map of the corresponding node
				 * and send an ACK message back
				 *
				 * START YOUR CODE HERE
				 **********************************************/
        update_node->file_map = new_map;

        /* send ACK packet */
        memcpy(send_buf, &seq, sizeof(seq));
        send_idx += 2; /* seq and blank */

        memcpy(send_buf + send_idx, ACK, strlen(ACK));
        send_idx += strlen(ACK);

        sendto(sockfd, (const char *)send_buf, send_idx,
          0, (const struct sockaddr *) &clientaddr, sizeof(clientaddr));

				/***********************************************
				 * END OF YOUR CODE
				 **********************************************/
			}
		}


		// QUERY
		if (strncmp(buffer + 2, QUERY, strlen(QUERY)) == 0) {
      // printf("=== QUERY packet\n");
			char file_idx;
			char file_name[20];

			parse_idx += strlen(QUERY);
			parse_idx ++; /*skip blank */

			memcpy(&file_name, buffer + parse_idx, 10);
			file_idx = (file_name[0] - '0') * 10 + (file_name[1] - '0');
			if (file_idx < 0 || file_idx > 31) {
				printf("Invalid file name: %s", file_name);
				continue;
			}

			/* send ACK packet */
			memcpy(send_buf, &seq, sizeof(seq));
			send_idx += 2; /* seq and blank */

			memcpy(send_buf + send_idx, ACK, strlen(ACK));
			send_idx += strlen(ACK);

			sendto(sockfd, (const char *)send_buf, send_idx,
					0, (const struct sockaddr *) &clientaddr, sizeof(clientaddr));

			// Send the first RESPONSE message
			// Whenever receive QUERY, initiate the context
			struct node *current_node = send_return(sockfd, clientaddr, file_idx, node_head, SEQ0);

			if (current_node == NULL) {
				char new_seq = 1 - seq; /* 1 -> 0, 0 -> 1 */
				/* send FINISH packet */
				send_finish(sockfd, clientaddr, new_seq);
				current_ctx->waiting_ack = 0;
			} else {
				// Update context
				current_ctx->waiting_ack = 1;
				current_ctx->clock = get_current_time();
				current_ctx->noack_num = SEQ0;
				current_ctx->noack_node = current_node;
				current_ctx->file_idx = file_idx;
			}

		}


		// ACK
		if (strncmp(buffer + 2, ACK, strlen(ACK)) == 0) {
      // printf("=== ACK packet\n");
			char new_seq = 1 - seq; /* 1 -> 0, 0 -> 1 */

			if (current_ctx->waiting_ack && current_ctx->noack_num == seq) {
				current_ctx->clock = get_current_time();
				current_ctx->noack_num = new_seq;
				current_ctx->noack_node = send_return(sockfd, clientaddr,
						current_ctx->file_idx, current_ctx->noack_node->next, new_seq);

				if (current_ctx->noack_node == NULL) {
					/* send FINISH packet */
					send_finish(sockfd, clientaddr, new_seq);
					current_ctx->waiting_ack = 0;
				}

			}
		}

		bzero(buffer, MAXMSG);
		bzero(send_buf, MAXMSG);
		parse_idx = 0;
		send_idx = 0;
	}
	return 0;
}
