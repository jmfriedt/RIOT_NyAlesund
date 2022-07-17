/* merge between tests/driver_sx127x and tests/xtimer_periodic_wakeup/ and tests/bench_mutex_pingpong/main.c
   must be DIFFERENT stack for each thread!
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "thread.h"
#include "shell.h"
#include "shell_commands.h"

#include "net/netdev.h"
#include "net/netdev/lora.h"
#include "net/lora.h"

#include "board.h"

#include "sx127x_internal.h"
#include "sx127x_params.h"
#include "sx127x_netdev.h"

#include "fmt.h"

#include "mutex.h"

#define SX127X_LORA_MSG_QUEUE   (16U)
#define SX127X_STACKSIZE        (THREAD_STACKSIZE_DEFAULT*2) // 1024*2

static char stack[SX127X_STACKSIZE];
static mutex_t _mutex = MUTEX_INIT;

#define with_rs 1

#ifdef with_rs
static kernel_pid_t _rs_pid;
#endif

#define BUFSIZE (1024)

static char messagein[SX127X_LORA_MSG_QUEUE*2];
static char messageout[BUFSIZE];
static sx127x_t sx127x;

#include "periph/timer.h"
#include "xtimer.h"
volatile unsigned int compteurin;

#include "stdio_uart.h"

int lora_setup_cmd(int bw, uint8_t lora_sf, int cr)
{

    /* Check bandwidth value */
    uint8_t lora_bw;
    switch (bw) {
    case 125:
        puts("setup: setting 125KHz bandwidth");
        lora_bw = LORA_BW_125_KHZ;
        break;

    case 250:
        puts("setup: setting 250KHz bandwidth");
        lora_bw = LORA_BW_250_KHZ;
        break;

    case 500:
        puts("setup: setting 500KHz bandwidth");
        lora_bw = LORA_BW_500_KHZ;
        break;

    default:
        puts("[Error] setup: invalid bandwidth value given, "
             "only 125, 250 or 500 allowed.");
        return -1;
    }

    /* Check spreading factor value */
    if (lora_sf < 7 || lora_sf > 12) {
        puts("[Error] setup: invalid spreading factor value given");
        return -1;
    }

    /* Check coding rate value */
    if (cr < 5 || cr > 8) {
        puts("[Error ]setup: invalid coding rate value given");
        return -1;
    }
    uint8_t lora_cr = (uint8_t)(cr - 4);

    /* Configure radio device */
    netdev_t *netdev = &sx127x.netdev;

    netdev->driver->set(netdev, NETOPT_BANDWIDTH,
                        &lora_bw, sizeof(lora_bw));
    netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR,
                        &lora_sf, sizeof(lora_sf));
    netdev->driver->set(netdev, NETOPT_CODING_RATE,
                        &lora_cr, sizeof(lora_cr));

    puts("[Info] setup: configuration set with success");
    return 0;
}

int send_cmd(char *data, int len)
{ int res;
  iolist_t iolist = {
        .iol_base = data,
        .iol_len = len
    };

 netdev_t *netdev = &sx127x.netdev;
 res=(netdev->driver->send(netdev, &iolist));
// if (res == -ENOTSUP) {puts("Cannot send: radio is still transmitting");}
// else res=0;
 return(res);
}

int listen_cmd(void)
{
    netdev_t *netdev = &sx127x.netdev;
    /* Switch to continuous listen mode */
    const netopt_enable_t single = false;
    netdev->driver->set(netdev, NETOPT_SINGLE_RECEIVE, &single, sizeof(single));
    const uint32_t timeout = 0;
    netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(timeout));

    /* Switch to RX state */
    netopt_state_t state = NETOPT_STATE_RX;
    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));

    printf("Listen mode set\n");

    return 0;
}

int channel_cmd(uint32_t chan)
{
    netdev_t *netdev = &sx127x.netdev;
    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan,
                            sizeof(chan));
    printf("New channel set\n");
    return 0;
}

int reset_cmd(void)
{
    netdev_t *netdev = &sx127x.netdev;

    puts("resetting sx127x...");
    netopt_state_t state = NETOPT_STATE_RESET;

    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(netopt_state_t));
    return 0;
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
        dev->driver->isr(dev);
    }
    else {
        size_t len;
        netdev_lora_rx_info_t packet_info;
        switch (event) {
        case NETDEV_EVENT_RX_STARTED:
//            puts("Data reception started");
            break;

        case NETDEV_EVENT_RX_COMPLETE:
            len = dev->driver->recv(dev, NULL, 0, 0);
            dev->driver->recv(dev, messagein, len, &packet_info);
if (len<SX127X_LORA_MSG_QUEUE) {messagein[len]=0; printf("%d\n",(int)len);}
            else {printf("%d ",(int)len);}
/*                "{Payload: \"%s\" (%d bytes), RSSI: %i, SNR: %i, TOA: %" PRIu32 "}\n",
                messagein, (int)len,
                packet_info.rssi, (int)packet_info.snr,
                sx127x_get_time_on_air((const sx127x_t *)dev, len));
*/
// stdio_write(messagein, (int)len);
            break;

        case NETDEV_EVENT_TX_COMPLETE:
            sx127x_set_sleep(&sx127x);
//            puts("Transmission completed");
            break;

        case NETDEV_EVENT_CAD_DONE:
            break;

        case NETDEV_EVENT_TX_TIMEOUT:
            sx127x_set_sleep(&sx127x);
            break;

        default:
            printf("Unexpected netdev event received: %d\n", event);
            break;
        }
    }
}

#ifdef with_rs
void *_rs_thread(void *arg)
{
    (void)arg;
    char val;
    static msg_t _msg_q[SX127X_LORA_MSG_QUEUE];
    msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE);
    puts("RS thread");fflush(stdout);

    while (1) 
       {stdio_read (&val,1);  // requires USEMODULE += shell in Makefile
        mutex_lock(&_mutex);
        if (compteurin<BUFSIZE)
          {messageout[compteurin]=val;
           compteurin++;
           mutex_unlock(&_mutex);
          } 
       }
}
#endif

gpio_t jmf_gpio_in=GPIO_PIN(0,15); // PA15 = P4

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void)
{
    int res;
    unsigned int oldcompteurin;
    uint32_t interval = 25600;
    int indice=0;
    xtimer_ticks32_t last_wakeup;
    gpio_init(jmf_gpio_in, GPIO_IN_PU);
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    sx127x.params = sx127x_params[0];
    netdev_t *netdev = &sx127x.netdev;
    netdev->driver = &sx127x_driver;
    if (netdev->driver->init(netdev) < 0) {
        puts("Failed to initialize SX127x device, exiting");
        return 1;
    }
    netdev->event_callback = _event_cb;

//reset_cmd();
    lora_setup_cmd(500,7,5);
    channel_cmd(868000000);

    if (gpio_read(jmf_gpio_in))
       {puts("\nTX\n");
        printf("THREAD_STACKSIZE_DEFAULT: %d\n",THREAD_STACKSIZE_DEFAULT);
#ifdef with_rs
        _rs_pid =   thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _rs_thread, NULL,
                             "rs_thread");
#endif
        oldcompteurin=0;
        while (1)
          { last_wakeup = xtimer_now();
            xtimer_periodic_wakeup(&last_wakeup, interval);
//            puts("T");
            mutex_lock(&_mutex);
            if (compteurin>0)                               // RS232 received
              {
               if (compteurin==oldcompteurin)              // but long silence since
                 {
                  indice=0;
                  do{
                     if (compteurin>=SX127X_LORA_MSG_QUEUE)
                       {//puts(":");
                        do {res=send_cmd(&messageout[indice],SX127X_LORA_MSG_QUEUE);
                           } while (res!=0);
                        indice+=SX127X_LORA_MSG_QUEUE;
                        compteurin-=SX127X_LORA_MSG_QUEUE;
                        // last_wakeup = xtimer_now();
                        // xtimer_periodic_wakeup(&last_wakeup, 300000);
                       }
                     else
                      {//puts(".");
                       do {res=send_cmd(&messageout[indice],compteurin);
                          } while (res!=0);
                       compteurin=0;oldcompteurin=0;
                      }
                    if (res == -ENOTSUP) { puts("*");}
                   } while (compteurin>0);
                 }
               else
                 {oldcompteurin=compteurin;}
              }
            mutex_unlock(&_mutex);
          }
       }
    else
       {puts("\nRX\n"); listen_cmd();}
    return 0;
}
