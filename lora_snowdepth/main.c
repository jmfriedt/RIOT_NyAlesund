#define LM75_PARAMS 
#define aveclora 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fmt.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "ztimer.h"

#include "net/loramac.h"     /* core loramac definitions */
#include "semtech_loramac.h" /* package API */

#include "loramac_utils.h"

#include "thread.h"

#include "board.h"
#include "msg.h"
#include "ringbuffer.h"
#include "periph/uart.h"

#include "mutex.h"
 
static mutex_t _mutex = MUTEX_INIT;
////////////// UART ///////////////

#ifndef SHELL_BUFSIZE
#define SHELL_BUFSIZE       (128U)
#endif
#ifndef UART_BUFSIZE
#define UART_BUFSIZE        (128U)
#endif

#define PRINTER_PRIO        (THREAD_PRIORITY_MAIN - 1)
#define PRINTER_TYPE        (0xabcd)

#define POWEROFF_DELAY      (250U * US_PER_MS)       // power off delay

#ifdef aveclora
extern semtech_loramac_t loramac;  /* The loramac stack device descriptor */
/* define the required keys for OTAA, e.g over-the-air activation (the
   null arrays need to be updated with valid LoRa values) */

static uint8_t secret[LORAMAC_APPKEY_LEN];
static uint8_t deveui[LORAMAC_DEVEUI_LEN] ;
static uint8_t appeui[LORAMAC_APPEUI_LEN] ;
static uint8_t appkey[LORAMAC_APPKEY_LEN] ;

int decompte;

#define SEND_PERIOD  300 // in sec
#endif

//    uart_write(dev, (uint8_t *)_endline, strlen(_endline));

typedef struct {
    char rx_mem[UART_BUFSIZE];
    ringbuffer_t rx_buf;
} uart_ctx_t;

static uart_ctx_t ctx;

static kernel_pid_t printer_pid;
static char printer_stack[THREAD_STACKSIZE_MAIN];

static void rx_cb(void *arg, uint8_t data)
{   (void)arg;
    uart_t dev = 1;
    ringbuffer_add_one(&ctx.rx_buf, data);
    if (data == '\r'){
        msg_t msg;
        msg.content.value = (uint32_t)dev;
        msg_send(&msg, printer_pid);
    }
}

static void *printer(void *arg)
{
  (void)arg;
  char c;
  char val[128];
  int index=0,res;
  uart_t dev;
  msg_t msg;
  msg_t msg_queue[8];
  gpio_t jmf_gpio1_out=GPIO_PIN(0,9); // PA09 = D9
  gpio_t jmf_gpio2_out=GPIO_PIN(0,0); // PA00 = D0 
  gpio_t jmf_gpio3_out=GPIO_PIN(1,10); // PB10 = D10 

  gpio_init(jmf_gpio1_out, GPIO_OUT);
  gpio_init(jmf_gpio2_out, GPIO_OUT);
  gpio_init(jmf_gpio3_out, GPIO_OUT);
  gpio_write(jmf_gpio1_out,0);
  gpio_write(jmf_gpio2_out,0);
  gpio_write(jmf_gpio3_out,0);
  msg_init_queue(msg_queue, 8);
  mutex_lock(&_mutex);

  while (1) {
    gpio_write(jmf_gpio1_out,0);  // power down sensor
    gpio_write(jmf_gpio2_out,0);
    gpio_write(jmf_gpio3_out,0);
    ztimer_sleep(ZTIMER_MSEC, SEND_PERIOD*1000);    // SEND_PERIOD s
    index=0;
    gpio_write(jmf_gpio1_out,1);  // power up sensor
    gpio_write(jmf_gpio2_out,1);
    gpio_write(jmf_gpio3_out,1);
    do {
        msg_receive(&msg);
        dev = (uart_t)msg.content.value;
       do {
           c=(int)ringbuffer_get_one(&(ctx.rx_buf));
           // uart_write(UART_DEV(dev), (unsigned char*)&c, 1);
           if (c == '\r') {
                  decompte++;
              } else
                {if (decompte>5)   // throw away initial sentence (brand/model)
                    {val[index]=c;
                     index++;
                    }
                }
             } while (c != '\r');
      } while (decompte < 9);     // get the right sentences
      res=semtech_loramac_send(&loramac, (uint8_t *)val, index-1);
      if (res != SEMTECH_LORAMAC_TX_DONE) { 
           uart_write(UART_DEV(dev), (unsigned char*)("FAIL\r\n"), 6);
          }
          else{ uart_write(UART_DEV(dev), (unsigned char*)("\r\nmessage sent: "),16);}
      uart_write(UART_DEV(dev), (unsigned char*)val, index);
      uart_write(UART_DEV(dev), (unsigned char*)'\r', 1);
      uart_write(UART_DEV(dev), (unsigned char*)'\n', 1);
      index=0;
      decompte=0;
//		semtech_loramac_set_tx_mode(&loramac, LORAMAC_TX_UNCNF);
      // uart_poweroff(UART_DEV(1));
      // uart_poweron(UART_DEV(1));
      //xtimer_usleep( SEND_PERIOD* 1000000);
      //thread_sleep();
    }
    /* this should never be reached */
    return NULL;
}

int main(void)
{
  int dev=1;
  uint32_t baud = 9600;
  int res;
/*
while (1) // test GPIO
{ gpio_write(jmf_gpio1_out,0); gpio_write(jmf_gpio2_out,0); gpio_write(jmf_gpio3_out,0);
  xtimer_usleep( 1000000);
  gpio_write(jmf_gpio1_out,1); gpio_write(jmf_gpio2_out,1); gpio_write(jmf_gpio3_out,1);
  xtimer_usleep( 1000000);
}
*/
  msg_t msg_queue[8];
  msg_init_queue(msg_queue, 8);
  mutex_lock(&_mutex);

  ringbuffer_init(&(ctx.rx_buf), ctx.rx_mem, UART_BUFSIZE); // initialize ringbuffers 
  res = uart_init(UART_DEV(dev), baud, rx_cb, (void *)(intptr_t)dev);
  if (res != UART_OK) {
     uart_write(UART_DEV(dev), (unsigned char*)"Error\r\n",7);
  }
  else {
     uart_write(UART_DEV(dev), (unsigned char*)"Success\r\n",9);
  }
  printer_pid = thread_create(printer_stack, sizeof(printer_stack),
                PRINTER_PRIO, 0, printer, NULL, "printer"); // start the printer thread 
#ifdef aveclora
    // 1. initialize the LoRaMAC MAC layer 
    semtech_loramac_init(&loramac);
 //   thread_create(_recv_stack, sizeof(_recv_stack), THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");
    // Convert identifiers and application key 
    fmt_hex_bytes(secret, SECRET);
    // forge the deveui, appeui and appkey of the endpoint 
    loramac_utils_forge_euis_and_key(deveui,appeui,appkey,secret);
    DEBUG("[otaa] Secret:"); printf_ba(secret,LORAMAC_APPKEY_LEN); DEBUG("\n");
    DEBUG("[otaa] DevEUI:"); printf_ba(deveui,LORAMAC_DEVEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppEUI:"); printf_ba(appeui,LORAMAC_APPEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppKey:"); printf_ba(appkey,LORAMAC_APPKEY_LEN); DEBUG("\n");
    // 2. set the keys identifying the device 
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);
    uart_write(UART_DEV(dev), (unsigned char*)("joining ...\r\n"),13);
    // 3. join the network 
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
		uart_write(UART_DEV(dev), (unsigned char*)("Join failed\r\n"),13);
		return 1;
    }
    uart_write(UART_DEV(dev), (unsigned char*)("Join success\r\n"),14);
#endif
    mutex_unlock(&_mutex);
    thread_sleep();
}
