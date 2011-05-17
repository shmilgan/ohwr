#ifndef __mi2c_h
#define __mi2c_h
void mi2c_start();
void mi2c_repeat_start();
void mi2c_stop();
unsigned char mi2c_put_byte(unsigned char data);
void mi2c_get_byte(unsigned char *data);
void mi2c_init();

#endif
