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

lsp_client* lsp_client_create(const char* src, const char* port)
{
	lsp_client *client = malloc(sizeof(lsp_client));
	client->info = sock_n_conn(src, port);
	
	client->message_seq_num = 0;
	return client;
}



int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{
	memset(pld, 0, 1024);
	LSPMessage *message;
	uint8_t buf[1024];
	
	int length = cli_recv(a_client->info, buf);
	
	if(length <= 0) return -1;	//message timed out
	
	message = lspmessage__unpack(NULL, length, buf);
	if(message == NULL){
		printf("Error unpacking message\n");
		return -1;
	}
	
	length = strlen(message->payload.data);
	int i;
	char* temp = pld;
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
	msg.payload.data = malloc(sizeof(uint8_t) * length);
	msg.payload.len = length;
	memcpy(msg.payload.data, pld, length * sizeof(uint8_t));
	
	size = lspmessage__get_packed_size(&msg);
	buffer = malloc(size);
	lspmessage__pack(&msg, buffer);
	
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