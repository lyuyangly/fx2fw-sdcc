#ifndef HARDWARE_H
#define HARDWARE_H

extern void ProgIO_Init(void);
extern void ProgIO_Set_State(unsigned char d);
extern unsigned char ProgIO_Set_Get_State(unsigned char d);
extern void ProgIO_ShiftOut(unsigned char x);
extern unsigned char ProgIO_ShiftInOut(unsigned char x);

#endif

