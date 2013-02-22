#include "lsp_client.h"
#include "lspmessage.pb-c.h"

double epoch_lth = _EPOCH_LTH;
int epoch_cnt = _EPOCH_CNT;
double drop_rate = _DROP_RATE;

/*
 *
 *
 *				LSP RELATED FUNCTIONS
 *
 *
 */  

void lsp_set_epoch_lth(double lth){epoch_lth = lth;}
void lsp_set_epoch_cnt(int cnt){epoch_cnt = cnt;}
void lsp_set_drop_rate(double rate){drop_rate = rate;}

/*
 *
 *
 *				CLIENT RELATED FUNCTIONS
 *
 *
 */  

lsp_client* lsp_client_create(const char* src, int portNum)
{
	char *port = (char*)malloc(4*sizeof(char));
	sprintf(port, "%d", portNum);
	
	lsp_client *client = (lsp_client*)malloc(sizeof(lsp_client));
	client->info = sock_n_conn(src, port);
	
	/* create the connection request packet */
	LSPMessage msg = LSPMESSAGE__INIT;
	uint8_t* buffer;
	unsigned response_size = 0, request_size;
	msg.connid = 0;
	msg.seqnum = 0;
	msg.payload.data = NULL;
	msg.payload.len = 0;
	request_size = lspmessage__get_packed_size(&msg);
	buffer = (uint8_t*)malloc(request_size);
	lspmessage__pack(&msg, buffer);
	
	/* send the connection request */
	int i = 0;
	uint8_t response[MAX_PACKET_SIZE];	//server response should be pretty small
	memset(response, 0, MAX_PACKET_SIZE);
	bool sent;
	while(i < epoch_cnt && response_size <= 0){	//keep trying to send connection request up to epoch times
		sent = cli_send(client->info, buffer, request_size);
		sleep(epoch_lth);		//wait a moment
		response_size = cli_recv(client->info, response);	
		if(response_size > 0){
			LSPMessage *message;
			message = lspmessage__unpack(NULL, response_size, response);
			client->conn_id = message->connid;
			client->message_seq_num = 0;
			printf("Received connection id %d\n", client->conn_id);
			break;
		}
		i++;
	}
	if(i == epoch_cnt){
		printf("Failed to connect\n");
		return NULL;
	}
	
	pthread_t epoch_counter;				//initialize the epoch counter and run it
	int thread_id = pthread_create(&epoch_counter, NULL, epoch_trigger, (void*)client);
	
	free(buffer);
	//TODO: free packed messages
	free(port);
	return client;
}

void* epoch_trigger(void* client){
	
	void* buffer;
	unsigned size;	
	
	LSPMessage msg = LSPMESSAGE__INIT;
	msg.connid = ((lsp_client*)client)->conn_id;
	msg.seqnum = ((lsp_client*)client)->message_seq_num;
	msg.payload.data = NULL;
	msg.payload.len = 0;
	
	size = lspmessage__get_packed_size(&msg);
	buffer = malloc(size);
	lspmessage__pack(&msg, (uint8_t*)buffer);
	
	int i;
	while(true){
		//send an empty packet just often enough to maintain the connection
		sleep(epoch_lth*epoch_cnt - 2);
		
		bool sent = cli_send(((lsp_client*)client)->info, buffer, size);
		printf("ICMP packet sent\n");
	}
}

int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{
	memset(pld, 0, MAX_PACKET_SIZE);
	LSPMessage *message;
	uint8_t buf[MAX_PACKET_SIZE];
	
	int length = cli_recv(a_client->info, buf);
	
	if(length <= 0) return -1;	//message timed out
	
	message = lspmessage__unpack(NULL, length, buf);
	if(message == NULL){
		printf("Error unpacking message\n");
		return -1;
	}
	
	length = strlen((char*) message->payload.data);
	int i;
	char* temp = (char*)pld;
	for(i = 0; i < length; i++){
		*(char*)temp++ = message->payload.data[i];
	}
	
	lspmessage__free_unpacked(message, NULL);
	return length;
}



bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int length)
{
	void* buffer;
	unsigned size;	
	
	LSPMessage msg = LSPMESSAGE__INIT;
	msg.connid = a_client->conn_id;
	msg.seqnum = ++a_client->message_seq_num;
	msg.payload.data = (uint8_t*)malloc(sizeof(uint8_t) * length);
	msg.payload.len = length;
	memcpy(msg.payload.data, pld, length * sizeof(uint8_t));
	
	size = lspmessage__get_packed_size(&msg);
	buffer = malloc(size);
	lspmessage__pack(&msg, (uint8_t*)buffer);
	
	bool sent = cli_send(a_client->info, buffer, size);
	
	free(buffer);
	free(msg.payload.data);
	if(sent){
		printf("message of length %d bytes sent\n", size);
		return true;
	}else{
		printf("Host may not be available\n");
		return false;
	}
}



bool lsp_client_close(lsp_client* a_client)
{	
	if(cli_close(a_client->info))
		printf("Connection closed\n");
	free(a_client);
	return true;
}
