/* merge between tests/driver_sx127x and tests/xtimer_periodic_wakeup/ and tests/bench_mutex_pingpong/main.c
   must be DIFFERENT stack for each thread!
 */

//#define RXROVER    // STATION if commented
#define with_event
#define with_wdt     // we verify that no message TX => reset after 5 seconds

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "thread.h"
#include "shell.h"

#include "net/netdev.h"
#include "net/netdev/lora.h"
#include "net/lora.h"

#include "board.h"

#include "sx127x_internal.h"
#include "sx127x_params.h"
#include "sx127x_netdev.h"

#include "fmt.h"

#include "mutex.h"
 
#ifdef with_wdt
#include "periph/wdt.h"
#endif

#ifdef RXROVER
#define SX127X_LORA_MSG_QUEUE   (16U) 
#else // TXBASE
#define SX127X_LORA_MSG_QUEUE   (64U) 
#endif
#define SX127X_STACKSIZE        (THREAD_STACKSIZE_DEFAULT*4) // 1024*2


#ifdef RXROVER
static char stackrx[SX127X_STACKSIZE];
static kernel_pid_t _rsrx_pid;
//static mutex_t _mutexout = MUTEX_INIT;
#else
static char stacktx[SX127X_STACKSIZE];
static kernel_pid_t _rstx_pid;
//static mutex_t _mutexin = MUTEX_INIT;
#endif

#define BUFSIZE (1024*3)

static char message[BUFSIZE]; // both for input or output since only used in one direction
static sx127x_t sx127x;

#include "stdio_uart.h"

#include "periph/timer.h"
#include "xtimer.h"
volatile unsigned int compteurin;
volatile unsigned int indexin;

gpio_t jmf_gpio_out=GPIO_PIN(0,15); // PA15 = P4

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

#ifdef with_event
#include "event/thread.h"
netdev_t *eventdev;

static void _handler_high(event_t *event) {
    (void)event;
    eventdev->driver->isr(eventdev);
}

static event_t event_high = { .handler=_handler_high };
#endif

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
#ifdef with_event
        eventdev=dev;
        event_post(EVENT_PRIO_HIGHEST, &event_high);
#else
        dev->driver->isr(dev);
#endif
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
//            mutex_lock(&_mutexout);
            dev->driver->recv(dev, &message[indexin], len, &packet_info);
            indexin+=len;
//            mutex_unlock(&_mutexout);

// INTERMEDIATE PAYLOAD
//            if (len<SX127X_LORA_MSG_QUEUE) {/*messagein[len]=0;*/ printf("%d\n",(int)len);}
//            else {printf("%d ",(int)len);} 
// INTERMEDIATE PAYLOAD

// DO NOT display the payload as it will generate ISR stack overflow due to
//   excessive communication on UART
/*                "{Payload: \"%s\" (%d bytes), RSSI: %i, SNR: %i, TOA: %" PRIu32 "}\n",
                messagein, (int)len,
                packet_info.rssi, (int)packet_info.snr,
                sx127x_get_time_on_air((const sx127x_t *)dev, len));
*/
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

void *_rstx_thread(void *arg)
{
    (void)arg;
    char val;
    static msg_t _msg_q[SX127X_LORA_MSG_QUEUE];
    msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE);
    puts("TX thread");fflush(stdout);

    while (1) 
       {stdio_read (&val,1);  // requires USEMODULE += shell in Makefile
//        mutex_lock(&_mutexin);
        if (compteurin<BUFSIZE)
          {message[compteurin]=val;
// printf("%d: %d\n",compteurin,val); // MUST be remove for maximum bandwidth
           compteurin++;
//           mutex_unlock(&_mutexin);
          } 
       }
}

void *_rsrx_thread(void *arg)
{
    (void)arg;
    static msg_t _msg_q[SX127X_LORA_MSG_QUEUE*2];
    msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE*2);
    xtimer_ticks32_t last_wakeup;
    uint32_t interval = 25600*2;
    unsigned int oldindexin=0;

    puts("RX thread");fflush(stdout);
    while (1) 
       {
        last_wakeup = xtimer_now();
        xtimer_periodic_wakeup(&last_wakeup, interval);
        if (indexin>0)
          {if (oldindexin==indexin)
             {
//        mutex_lock(&_mutexout);
//            printf("\n%d:",indexin);  // TOTAL PAYLOAD
if (indexin>1432) {indexin=1432;}
               gpio_write(jmf_gpio_out,1);
#ifdef with_wdt
               wdt_kick();
#endif
               stdio_write (message,indexin);  // requires USEMODULE += shell in Makefile
               gpio_write(jmf_gpio_out,0);
               indexin=0; // content of buffer has been dumped, restart
//        mutex_unlock(&_mutexout);
              }
           else {oldindexin=indexin;} // still receiving
          }
       }
}

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void)
{   
#ifndef RXROVER
    int res;
    unsigned int oldcompteurin;
    uint32_t interval = 25600;
    int indice=0;
    xtimer_ticks32_t last_wakeup;
#else
    long int indicedump;
#endif
    gpio_init(jmf_gpio_out, GPIO_OUT);
    gpio_write(jmf_gpio_out,0);
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
    lora_setup_cmd(500,7,5); // 500 kHz, SF=7 => 21875 bps
    channel_cmd(868000000);

#ifdef RXROVER
    for (indicedump=0x40023800;indicedump<0x40023BFF;indicedump+=4)
       {printf("%lx: %lx\n",indicedump,*(long*)(indicedump));}
#endif            // dont print when wathdog is triggered on TX
#ifdef with_wdt
    wdt_setup_reboot(0, 5000); // MAX_TIME in ms
    wdt_start();
#endif
#ifndef RXROVER
       {
        printf("THREAD_STACKSIZE_DEFAULT: %d\n",THREAD_STACKSIZE_DEFAULT);
        _rstx_pid =   thread_create(stacktx, sizeof(stacktx), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _rstx_thread, NULL,
                             "rstx_thread");
        oldcompteurin=0;
        while (1)
          { last_wakeup = xtimer_now();
            xtimer_periodic_wakeup(&last_wakeup, interval);
//            puts("T");
//            mutex_lock(&_mutexin);
            if (compteurin>0)                               // RS232 received
              {
               if (compteurin==oldcompteurin)              // but long silence since
                 {
                  indice=0;
                  printf("TX %d\n",compteurin);
                  gpio_write(jmf_gpio_out,1);
#ifdef with_wdt
                  wdt_kick();
#endif
                  do{
                     if (compteurin>=SX127X_LORA_MSG_QUEUE)
                       {//puts(":");
                        do {res=send_cmd(&message[indice],SX127X_LORA_MSG_QUEUE);
                           } while (res!=0);
                        indice+=SX127X_LORA_MSG_QUEUE;
                        compteurin-=SX127X_LORA_MSG_QUEUE;
                        last_wakeup = xtimer_now();
                        xtimer_periodic_wakeup(&last_wakeup, 30);
                       }
                     else
                      {//puts(".");
                       do {res=send_cmd(&message[indice],compteurin);
                          } while (res!=0);
                       compteurin=0; oldcompteurin=0;
                      }
                    if (res == -ENOTSUP) { puts("*");}
                   } while (compteurin>0);
                  gpio_write(jmf_gpio_out,0);
                 }
               else
                 {oldcompteurin=compteurin;}
              }
//            mutex_unlock(&_mutexin);
          }
       }
#endif
//    else // GPIO
#ifdef RXROVER
       {
        _rsrx_pid =   thread_create(stackrx, sizeof(stackrx), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _rsrx_thread, NULL,
                             "rsrx_thread");
        indexin=0;
        puts("\nRX\n"); listen_cmd();
       }
#endif
    return 0;
}
