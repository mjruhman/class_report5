
// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"


/**
 * blink once per second for 5 times.
 * provide a sanity check for timer (based on SYS_CLK_FREQ)
 * @param led_p pointer to led instance
 */
void timer_check(GpoCore *led_p) {
   int i;

   for (i = 0; i < 5; i++) {
      led_p->write(0xffff);
      sleep_ms(500);
      led_p->write(0x0000);
      sleep_ms(500);
      debug("timer check - (loop #)/now: ", i, now_ms());
   }
}

/**
 * check individual led
 * @param led_p pointer to led instance
 * @param n number of led
 */
void led_check(GpoCore *led_p, int n) {
   int i;

   for (i = 0; i < n; i++) {
      led_p->write(1, i);
      sleep_ms(100);
      led_p->write(0, i);
      sleep_ms(100);
   }
}

/**
 * leds flash according to switch positions.
 * @param led_p pointer to led instance
 * @param sw_p pointer to switch instance
 */
void sw_check(GpoCore *led_p, GpiCore *sw_p) {
   int i, s;

   s = sw_p->read();
   for (i = 0; i < 30; i++) {
      led_p->write(s);
      sleep_ms(50);
      led_p->write(0);
      sleep_ms(50);
   }
}


void sseg_print_temp(SsegCore *sseg_p, float temp, bool change) {
   int tempy = (int)(temp * 10);

   int d1 = (tempy / 100) % 10;  
   int d2 = (tempy / 10) % 10;   
   int d3 = tempy % 10;           
   
   // Clear all first
   for (int i = 0; i < 8; i++)
        sseg_p->write_1ptn(0xFF, i);

   sseg_p->write_1ptn(sseg_p->h2s(d1), 3);
   sseg_p->write_1ptn(sseg_p->h2s(d2), 2);
   sseg_p->write_1ptn(sseg_p->h2s(d3), 1);
   sseg_p->set_dp(2 << 1); 
   if (change) {
       sseg_p->write_1ptn(0xE, 0);
   }
   else {
       sseg_p->write_1ptn(0xC6, 0);
   }

}




/*
 * read temperature from adt7420
 * @param adt7420_p pointer to adt7420 instance
 */
void adt7420_check(I2cCore *adt7420_p, SsegCore *sseg_p, GpiCore *sw_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   //int ack;
   uint16_t tmp;
   float tmpC;
   float tmpF;
   bool change;



   // read adt7420 id register to verify device existence
   // ack = adt7420_p->read_dev_reg_byte(DEV_ADDR, 0x0b, &id);

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   uart.disp("read ADT7420 id (should be 0xcb): ");
   uart.disp(bytes[0], 16);
   uart.disp("\n\r");
   //debug("ADT check ack/id: ", ack, bytes[0]);
   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
      tmpF = ((float) tmpC * 1.8) + 32;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
      tmpF = ((float) tmpC * 1.8) + 32;
   }

   change = (sw_p->read() & 0x01);

   if(change) {
      sseg_print_temp(sseg_p, tmpF, change);
   }
   else {
      sseg_print_temp(sseg_p, tmpC, change);
   }

   sleep_ms(500);
}


GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));

int main() {
   while (1) {

      adt7420_check(&adt7420, &sseg, &sw);
   } //while
} //main



