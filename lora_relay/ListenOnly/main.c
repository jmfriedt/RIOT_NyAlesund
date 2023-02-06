/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the sx126x/llcc68 radio driver
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */
#define 	LORA_SYNCWORD_PRIVATE 0x32//(0x12)
#define 	LORA_SYNCWORD_PUBLIC (0x34)

#define SPREADING_FACTOR 12

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "msg.h"
#include "thread.h"
#include "shell.h"

#include "net/lora.h"
#include "net/netdev.h"
#include "net/netdev/lora.h"

#include "sx126x.h"
#include "sx126x_params.h"
#include "sx126x_netdev.h"

#define SX126X_MSG_QUEUE        (8U)
#define SX126X_STACKSIZE        (THREAD_STACKSIZE_DEFAULT)
#define SX126X_MSG_TYPE_ISR     (0x3456)
#define SX126X_MAX_PAYLOAD_LEN  (256U)

static char stack[SX126X_STACKSIZE];
static kernel_pid_t _recv_pid;

static char message[SX126X_MAX_PAYLOAD_LEN];
size_t len;
int flag=0;

static sx126x_t sx126x;


static void _event_cb(netdev_t *dev, netdev_event_t event){
    if (event == NETDEV_EVENT_ISR) {
        msg_t msg;
        msg.type = SX126X_MSG_TYPE_ISR;
        if (msg_send(&msg, _recv_pid) <= 0) {
            puts("sx126x_netdev: possibly lost interrupt.");
        }
    }
    else {
        switch (event) {
        case NETDEV_EVENT_RX_STARTED:
            puts("Data reception started");
            break;

        case NETDEV_EVENT_RX_COMPLETE:
        {
            len = dev->driver->recv(dev, NULL, 0, 0);
            netdev_lora_rx_info_t packet_info;
            dev->driver->recv(dev, message, len, &packet_info);
            /*printf(
                "Received: \"%s\" (%d bytes) - [RSSI: %i, SNR: %i, TOA: %" PRIu32 "ms]\n",
                message, (int)len,
                packet_info.rssi, (int)packet_info.snr,
                sx126x_get_lora_time_on_air_in_ms(&sx126x.pkt_params, &sx126x.mod_params)
                );*/
            //char buffer[256];
            //char temp[256]="";
            //printf("Received : ");
            flag=1;
            //for (int k = 0; k < (int)len; ++k){
				//if ((int)message[k]!=0){
					//sprintf(buffer,"%x",(int)message[k]);
					//for (int i=0; i<4; i++){
						//printf("%c%c|", buffer[6-i*2], buffer[7-i*2]);
					//}
				//	printf("%x|",(int)message[k]);
				//}
			//}
			//printf("\n");
			/*printf(" (%d bytes)\n",(int)len);
			for (int k = 0; k < (int)len; ++k){
				//if ((int)message[k]!=0){
					//printf("%x|",(int)message[k]);
					sprintf(buffer,"%x",(int)message[k]);
					//strcat(temp,buffer);
				//}
			}*/
			//printf("\n");
			//printf("%s\n",temp);
            //printf(
                //"Received: \"%x\" (%d bytes) - [RSSI: %i, SNR: %i, TOA: %" PRIu32 "ms]\n",
                //(int)message[4], (int)len,
                //packet_info.rssi, (int)packet_info.snr,
                //sx126x_get_lora_time_on_air_in_ms(&sx126x.pkt_params, &sx126x.mod_params)
                //);
            netopt_state_t state = NETOPT_STATE_RX;
            dev->driver->set(dev, NETOPT_STATE, &state, sizeof(state));
        }
        break;

        case NETDEV_EVENT_TX_COMPLETE:
            puts("Transmission completed");
            netopt_state_t state = NETOPT_STATE_RX;
            dev->driver->set(dev, NETOPT_STATE, &state, sizeof(state));
            break;

        case NETDEV_EVENT_TX_TIMEOUT:
            puts("Transmission timeout");
            break;

        default:
            printf("Unexpected netdev event received: %d\n", event);
            break;
        }
    }
}

void *_recv_thread(void *arg){
    netdev_t *netdev = arg;

    static msg_t _msg_queue[SX126X_MSG_QUEUE];

    msg_init_queue(_msg_queue, SX126X_MSG_QUEUE);

    while (1) {
        msg_t msg;
        msg_receive(&msg);
        if (msg.type == SX126X_MSG_TYPE_ISR) {
            netdev->driver->isr(netdev);
        }
        else {
            puts("Unexpected msg type");
        }
    }
}





void print_param(sx126x_t sx126x){
	//uint32_t freq;
    //netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &freq, sizeof(uint32_t));
    //printf("Frequency: %" PRIu32 "Hz\n", freq);
    printf("Channel frequency : %li\n",sx126x_get_channel(&sx126x));
    int bw=sx126x_get_bandwidth(&sx126x);
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
    printf("Bandwidth: %ikHz\n", bw_val);
    printf("Spreading factor: %d\n", sx126x_get_spreading_factor(&sx126x));
    printf("Coding rate: %d\n", sx126x_get_coding_rate(&sx126x));
}

void set_param(sx126x_t *sx126x, int chan_freq, int bandwidth, int spreading_factor, int code_rate){
	switch(bandwidth){
	case 125:
		sx126x_set_bandwidth(sx126x,0); // 0:125kHz 1:250kHz 2:500kHz
		break;
	case 250:
		sx126x_set_bandwidth(sx126x,1); // 0:125kHz 1:250kHz 2:500kHz
		break;
	case 500:
		sx126x_set_bandwidth(sx126x,2); // 0:125kHz 1:250kHz 2:500kHz
		break;
	default:
		printf("invalid bandwidth, use 125, 250 or 500\n");
        break;
	}
	sx126x_set_spreading_factor(sx126x, spreading_factor);
	if ((code_rate<5)||(code_rate>8)){
		printf("invalid code rate, value between 5 and 8\n");
	}
	else{
		sx126x_set_coding_rate(sx126x,code_rate-4);
	}
    sx126x_set_channel(sx126x, chan_freq);
}

void debug_cmd(netdev_t *netdev){
	uint8_t bw=0;
	netdev->driver->set(netdev, NETOPT_BANDWIDTH, &bw, sizeof(uint8_t));
	uint32_t freq = 868100000;
    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &freq, sizeof(uint32_t));
    uint8_t sf = 12;
    netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &sf, sizeof(uint8_t));
    uint8_t cr = 1;
    netdev->driver->set(netdev, NETOPT_CODING_RATE, &cr, sizeof(uint8_t));
}

void debug_print(netdev_t *netdev){
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
}

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


void wait(int timeToSleep){
	// use wait(22);
	for(int i=1; i<timeToSleep;i++){
		wait(timeToSleep-i);
	}
}


int main(void){
    sx126x_setup(&sx126x, &sx126x_params[0], 0);
    netdev_t *netdev = &sx126x.netdev;
    
    sx126x.netdev.driver = &sx126x_driver;
    sx126x.netdev.event_callback = _event_cb;

    if (netdev->driver->init(netdev) < 0) {
        puts("Failed to initialize SX126X device, exiting");
        return 1;
    }

    _recv_pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _recv_thread, &sx126x.netdev,
                              "recv_thread");

    if (_recv_pid <= KERNEL_PID_UNDEF) {
        puts("Creation of receiver thread failed");
        return 1;
    }


    //sx126x_set_lora_iq_invert(&sx126x,NETOPT_DISABLE);
    netopt_enable_t iq_inverted = NETOPT_ENABLE;//ENABLE;
    netdev->driver->set(netdev, NETOPT_IQ_INVERT, &iq_inverted, sizeof(netopt_enable_t));
    set_param(&sx126x,868100000,125,SPREADING_FACTOR,5);
	//debug_cmd(&sx126x.netdev);
	sx126x_set_lora_crc(&sx126x,0);
    printf("get crc : %i\n",sx126x_get_lora_crc(&sx126x));
    printf("get preamble length : %i\n",sx126x_get_lora_preamble_length(&sx126x));
    printf("get implicit header : %i\n",sx126x_get_lora_implicit_header(&sx126x));
    sx126x_set_lora_payload_length(&sx126x,5);
    printf("get payload length : %i\n",sx126x_get_lora_payload_length(&sx126x));
	netopt_state_t state = NETOPT_STATE_IDLE;
    sx126x.netdev.driver->set(&sx126x.netdev, NETOPT_STATE, &state, sizeof(state));
    print_param(sx126x);
    //debug_print(&sx126x.netdev);
    uint8_t mytemp;
	netdev->driver->get(netdev, NETOPT_IQ_INVERT, &mytemp, sizeof(uint8_t));
    printf("iq invert : %i\n",mytemp);
    uint8_t syncword = LORA_SYNCWORD_PUBLIC;
    netdev->driver->set(netdev, NETOPT_SYNCWORD, &syncword, sizeof(uint8_t));
    puts("Initialization successful");
    while(1){
		char buffer[256]="";
		char temp[256]="";
		if (flag!=0){
			for (int k = 0; k < (int)len; ++k){
				//if ((int)message[k]!=0){
					//printf("%x|",(int)message[k]);
					sprintf(buffer,"%x",(int)message[k]);
					strcat(temp,buffer);
				//}
			}
			printf("%s (%d bytes)\n",temp,(int)len);
			/*set_param(&sx126x,868300000,125,SPREADING_FACTOR,5);
			if (test_send(netdev,temp)<=0){
				printf("fail\n");
			}
			else{
				printf("ok\n");
			}
			set_param(&sx126x,868100000,125,SPREADING_FACTOR,5);*/
			
			
			//temp[0]='\0';
			flag=0;
		}
		fflush(stdout);
		
		//printf(",,%i\n",flag);
		//size_t out_len = b64_decoded_size(*message)+1;
			//char* out = malloc(out_len);
	
			//if (!b64_decode(*message, (unsigned char *)out, out_len)) {
				//printf("Decode Failure\n");
			//}
			//out[out_len] = '\0';

			//printf("dec:     '%s'\n", out);

	//printf("get crc : %i\n",sx126x_get_lora_crc(&sx126x));
	//netdev_lora_rx_info_t packet_info;
	//size_t len = netdev->driver->recv(netdev, NULL, 0, 0);
	//netdev->driver->recv(netdev, message, len, &packet_info);
	//printf("message : %s\n",message);
	}
    return 0;
}
