#ifndef I2C_H
#define I2C_H

// returns non-zero if successful, else 0
unsigned char i2c_read(unsigned char addr, __xdata unsigned char *buf, unsigned char len);

// returns non-zero if successful, else 0
unsigned char i2c_write(unsigned char addr, __xdata const unsigned char *buf, unsigned char len);

#endif
