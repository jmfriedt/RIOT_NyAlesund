#define LM75_PARAMS 

#include <stdio.h>
#include <string.h>

#define ENABLE_DEBUG (1)
#include "debug.h"


#include "xtimer.h"
#include <time.h>

#include "fmt.h"

//#include "net/loramac.h"     /* core loramac definitions */
#include "semtech_loramac.h" /* package API */

#include "loramac_utils.h"

#include "thread.h"


extern semtech_loramac_t loramac;  /* The loramac stack device descriptor */
/* define the required keys for OTAA, e.g over-the-air activation (the
   null arrays need to be updated with valid LoRa values) */

static uint8_t secret[LORAMAC_APPKEY_LEN];
static uint8_t deveui[LORAMAC_DEVEUI_LEN] ;
static uint8_t appeui[LORAMAC_APPEUI_LEN] ;
static uint8_t appkey[LORAMAC_APPKEY_LEN] ;

#define SEND_PERIOD  30 // in sec
 
#define RECV_MSG_QUEUE                   (4U)

static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

static void *_recv(void *arg)
{
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
 
    (void)arg;
    while (1) {
        /* blocks until some data is received */
        semtech_loramac_recv(&loramac);
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("Data received: %s, port: %d\n",
               (char *)loramac.rx_data.payload, loramac.rx_data.port);
    }
    return NULL;
}

int main(void)
{
    /* 1. initialize the LoRaMAC MAC layer */
    semtech_loramac_init(&loramac);
    thread_create(_recv_stack, sizeof(_recv_stack), THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");
    /* Convert identifiers and application key */
	fmt_hex_bytes(secret, SECRET);
    //* forge the deveui, appeui and appkey of the endpoint */
    loramac_utils_forge_euis_and_key(deveui,appeui,appkey,secret);
    DEBUG("[otaa] Secret:"); printf_ba(secret,LORAMAC_APPKEY_LEN); DEBUG("\n");
	DEBUG("[otaa] DevEUI:"); printf_ba(deveui,LORAMAC_DEVEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppEUI:"); printf_ba(appeui,LORAMAC_APPEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppKey:"); printf_ba(appkey,LORAMAC_APPKEY_LEN); DEBUG("\n");
 
    /* 2. set the keys identifying the device */
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);
	printf("joining ...\n");
    /* 3. join the network */
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
		printf("Join procedure failed\n");
		printf("Process stopped\n");
		return 1;
    }
    printf("Join procedure succeeded\n");
 
	xtimer_usleep( 10* 1000000);
		/* 4. send some data */
	while(1){
		semtech_loramac_set_tx_mode(&loramac, LORAMAC_TX_UNCNF);
		printf("sending message...\n");
		char *message = "this is a message";
		if (semtech_loramac_send(&loramac, (uint8_t *)message, strlen(message)) != SEMTECH_LORAMAC_TX_DONE) {
			printf("Cannot send message '%s'\n", message);
		}
		else{
		printf("message sent : %s\n",message);
		}
		
		//printf("uplink counter : %ld\n",semtech_loramac_get_uplink_counter(&loramac));
		
		xtimer_usleep( SEND_PERIOD* 1000000);
	}
}
