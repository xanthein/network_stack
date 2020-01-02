#ifndef TUN_IF_H
#define TUN_IF_H


int tap_init();
int tap_close();
int tap_read(char *buf, int len);
int tap_write(char *buf, int len);

#endif
