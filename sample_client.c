#include "lsp_client.h"
	
int main(int argc, char** argv) 
{
int counter = 0;
	lsp_client* myclient;
	
	if(argc < 2){
		printf("Usage: %s <address> <port>\n", argv[0]);
		exit(0);
	}
	else if(argc ==2){
		if((myclient = lsp_client_create("127.0.0.1", argv[1])) == NULL) exit(1);
	}
	else if((myclient = lsp_client_create(argv[1], argv[2])) == NULL) exit(1);

	char message[] = "what is this i don't even";
	
	while(true){
		lsp_client_write(myclient, message, strlen(message));
		uint8_t buffer[1024];

		int bytes = lsp_client_read(myclient, buffer);
		
		if(bytes > 0){
			printf("%d: %s\n", counter, buffer);
			counter++;
		}
		
		sleep(1);
}
	lsp_client_close(myclient);
	
	return 0;
}