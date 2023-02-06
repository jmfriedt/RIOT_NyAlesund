#define 	LORA_SYNCWORD_PRIVATE (0x12)
//#define 	LORA_SYNCWORD_PRIVATE (0x1424)
#define 	LORA_SYNCWORD_PUBLIC (0x34)
//#define 	LORA_SYNCWORD_PUBLIC (0x3444)
#define		MY_SYNCWORD (0x32)

#define SPREADING_FACTOR 12

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
 
/*#define RECV_MSG_QUEUE                   (4U)

static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

static void *_recv(void *arg)
{
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
 
    (void)arg;
    while (1) {
        //blocks until some data is received
        semtech_loramac_recv(&loramac);
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("Data received: %s, port: %d\n",
               (char *)loramac.rx_data.payload, loramac.rx_data.port);
    }
    return NULL;
}*/


int test_send(netdev_t *netdev, char* payload){
	printf("sending \"%s\" payload (%u bytes)\n",
           payload, (unsigned)strlen(payload) + 1);
    iolist_t iolist = {
        .iol_base = payload,
        .iol_len = (strlen(payload) + 1)
    };

    if (netdev->driver->send(netdev, &iolist) == -ENOTSUP) {
        puts("Cannot send: radio is still transmitting");
        return -1;
    }
    return 1;
}

void print_param(netdev_t *netdev){
	uint32_t freq;
	netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &freq, sizeof(uint32_t));
	printf("Frequency: %" PRIu32 "Hz\n", freq);
	uint8_t bw;
	netdev->driver->get(netdev, NETOPT_BANDWIDTH, &bw, sizeof(uint8_t));
	uint16_t bw_val = 0;
	switch (bw) {
	case 0:
		bw_val = 125;
		break;
	case 1:
		bw_val = 250;
		break;
	case 2:
		bw_val = 500;
		break;
	default:
		break;
	}
	printf("Bandwidth: %ukHz\n", bw_val);
	uint8_t sf;
	netdev->driver->get(netdev, NETOPT_SPREADING_FACTOR, &sf, sizeof(uint8_t));
	printf("Spreading factor: %d\n", sf);
	uint8_t cr;
	netdev->driver->get(netdev, NETOPT_CODING_RATE, &cr, sizeof(uint8_t));
	printf("Coding rate: %d\n", cr);
	sx126x_pkt_type_t packet_type;
    sx126x_get_pkt_type(netdev, &packet_type);
    printf("packet type : %d\n", packet_type);
}

void set_keys(semtech_loramac_t *loramac){
	/* Convert identifiers and application key */
	fmt_hex_bytes(secret, SECRET);
    //* forge the deveui, appeui and appkey of the endpoint */
    loramac_utils_forge_euis_and_key(deveui,appeui,appkey,secret);
    DEBUG("[otaa] Secret:"); printf_ba(secret,LORAMAC_APPKEY_LEN); DEBUG("\n");
	DEBUG("[otaa] DevEUI:"); printf_ba(deveui,LORAMAC_DEVEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppEUI:"); printf_ba(appeui,LORAMAC_APPEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppKey:"); printf_ba(appkey,LORAMAC_APPKEY_LEN); DEBUG("\n");
 
    /* 2. set the keys identifying the device */
    semtech_loramac_set_deveui(loramac, deveui);
    semtech_loramac_set_appeui(loramac, appeui);
    semtech_loramac_set_appkey(loramac, appkey);
}

void set_param(netdev_t *netdev, int chan_freq, int bandwidth, int spreading_factor, int code_rate){
	uint8_t bw;
	switch(bandwidth){
	case 125:
		bw=0; // 0:125kHz 1:250kHz 2:500kHz
		break;
	case 250:
		bw=1; // 0:125kHz 1:250kHz 2:500kHz
		break;
	case 500:
		bw=2; // 0:125kHz 1:250kHz 2:500kHz
		break;
	default:
		printf("invalid bandwidth, use 125, 250 or 500\n");
        break;
	}
	netdev->driver->set(netdev, NETOPT_BANDWIDTH, &bw, sizeof(uint8_t));
	printf("set_param : bandwidth = %i\n",bw);
	uint8_t sf = spreading_factor;
    netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &sf, sizeof(uint8_t));
    printf("set_param : spreading factor = %i\n",sf);
	uint8_t cr = code_rate;
    netdev->driver->set(netdev, NETOPT_CODING_RATE, &cr, sizeof(uint8_t));
    printf("set_param : coding rate = %i\n",cr);
    uint32_t freq = chan_freq;
    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &freq, sizeof(uint32_t));
    printf("set_param : channel frequency = %li\n",freq);
}

int main(void)
{
	netdev_t *netdev = loramac.netdev;
	//netdev->driver = &sx126x_driver;
	//if (netdev->driver->init(netdev) < 0) {
        //puts("Failed to initialize SX126X device, exiting");
        //return 1;
    //}
    
    /* 1. initialize the LoRaMAC MAC layer */
    semtech_loramac_init(&loramac);
    uint16_t mask = 0xF0;
    semtech_loramac_set_channels_mask(&loramac,&mask);
    //thread_create(_recv_stack, sizeof(_recv_stack), THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");
	uint8_t syncword = LORA_SYNCWORD_PUBLIC;
    //netdev->driver->set(netdev, NETOPT_SYNCWORD, &syncword, sizeof(uint8_t));
    //uint8_t syncword;
	//netdev->driver->get(netdev, NETOPT_SYNCWORD, &syncword, sizeof(uint8_t));
	//printf("syncword : %i\n",syncword);
	set_keys(&loramac);
	//set_param(netdev,868100000,125,SPREADING_FACTOR,1);
	//printf("set_param\n");
    //print_param(netdev);
    
    /* 3. join the network */
    //netdev->driver->set(netdev, NETOPT_SYNCWORD, &syncword, sizeof(uint8_t));
    printf("joining ...\n");
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
		printf("Join procedure failed\n");
		printf("Process stopped\n");
		return 1;
    }
    printf("Join procedure succeeded\n");
    
    //netdev->driver->get(netdev, NETOPT_SPREADING_FACTOR, &sf, sizeof(uint8_t));
    //printf("Spreading factor: %d\n", sf);
    //sx126x_pkt_type_t packet_type;
    //sx126x_get_pkt_type(netdev, &packet_type);
    //printf("packet type : %d\n", packet_type);
    
		/* 4. send some data */
	while(1){
		//semtech_loramac_set_tx_mode(&loramac, LORAMAC_TX_UNCNF);
		set_param(netdev,868100000,125,SPREADING_FACTOR,1);
		netdev->driver->set(netdev, NETOPT_SYNCWORD, &syncword, sizeof(uint8_t));
		
		printf("sending message...\n");
		char *message = "my message";
		if (semtech_loramac_send(&loramac, (uint8_t *)message, strlen(message)) != SEMTECH_LORAMAC_TX_DONE) {
			printf("Cannot send message '%s'\n", message);
		}
		else{
		printf("Message sent : '%s'\n",message);
		}
		//printf("before send\n");
		//if (test_send(netdev,"test")<=0){
			//printf("fail\n");
		//}
		//else{
			//printf("ok\n");
		//}		
		//printf("uplink counter : %ld\n",semtech_loramac_get_uplink_counter(&loramac));
		printf("before sleep\n");
		xtimer_usleep( SEND_PERIOD* 1000000);
		printf("after sleep\n");
	}
}
