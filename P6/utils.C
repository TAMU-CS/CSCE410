/* 
    File: utils.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University

    Date  : 09/02/12

*/

/* Some of the code comes from Brandon Friesens OS Tutorial: 
*  bkerndev - Bran's Kernel Development Tutorial
*  By:   Brandon F. (friesenb@gmail.com)
*  Desc: Interrupt Descriptor Table management
*
*  Notes: No warranty expressed or implied. Use at own risk. */


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

  /* -- (none ) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */ 
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* ABORT (USED e.g. IN _ASSERT()  */ 
/*--------------------------------------------------------------------------*/

void abort() {
  for(;;);
}

/*********************************************************
 * Debugging
 *********************************************************/

/* debug_out_E9: output to stdout, using bochs 0xE9 hack, a string (up to the initial 255 characters) */
void debug_out_E9(const char *_string) {
     int string_size = strlen(_string);
     if (string_size > 255) {
          // will print only first 255 characters
          string_size = 255;
     }
     for (int i=0; i < string_size; i++) {
          outportb(0xE9, _string[i]);
     }
}

void debug_out_E9_msg_value(const char *msg, const unsigned int value) {
    debug_out_E9(msg);
    char blank[2] = {' ', 0};
    debug_out_E9(blank);
    char localstr[32];
    uint2str(value, localstr);
    debug_out_E9(localstr);
    char nline[2] = {10, 0};
    debug_out_E9(nline);
}


/*--------------------------------------------------------------------------*/
/* MEMORY OPERATIONS  */ 
/*--------------------------------------------------------------------------*/

void *memcpy(void *dest, const void *src, int count)
{
    const char *sp = (const char *)src;
    char *dp = (char *)dest;
    for(; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void *memset(void *dest, char val, int count)
{
    char *temp = (char *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

unsigned short *memsetw(unsigned short *dest, unsigned short val, int count)
{
    unsigned short *temp = (unsigned short *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

/*--------------------------------------------------------------------------*/
/* STRING OPERATIONS  */ 
/*--------------------------------------------------------------------------*/


int strlen(const char *_str) {
    /* This loops through character array 'str', returning how
    *  many characters it needs to check before it finds a 0.
    *  In simple words, it returns the length in bytes of a string */
    int len = 0;
    while (*_str != 0) {
	_str++;
	len++;
    }
    return len;
}
void strcpy(char* _dst, char* _src) {
    while (*_src != 0) {
        *_dst = *_src;
        _dst++;
        _src++;
    }
    *_dst = 0;  // put terminating 0 at end.
}

void strncat ( char* dest, char* src, int num ) {
    char *d = dest;
    /* Find the end of destination  */
    d += strlen (dest);
    int srcsize = strlen (src);
    if (srcsize > num) {
	srcsize = num;
    }
    d[srcsize] = '\0';
    memcpy (d, src, srcsize);
}

void int2str(int _num, char * _str) {
        /* -- THIS IMPLEMENTATION IS ONE PRETTY BAD HACK. */
        int     i;
        char    temp[11];

        temp[0] = '\0';
        for(i = 1; i <= 10; i++)  {
                temp[i] = _num % 10 + '0';
                _num /= 10;
        }
        for(i = 10; temp[i] == '0'; i--);
        if( i == 0 )
                i++;
        while( i >= 0 )
                *_str++ = temp[i--];
}


void uint2str(unsigned int _num, char * _str) {
        /* -- THIS IS A BAD HACK AS WELL. */
        int     i;
        char    temp[11];

        temp[0] = '\0';
        for(i = 1; i <= 10; i++)  {
                temp[i] = _num % 10 + '0';
                _num /= 10;
        }
        for(i = 10; temp[i] == '0'; i--);
        if( i == 0 )
                i++;
        while( i >= 0 )
                *_str++ = temp[i--];
}

void ulong2hexstr(unsigned long _num, char * _str) {
        /* -- THIS IS A WORSE HACK AS WELL. */
        int     i;
        char    temp[9];

        temp[0] = '\0';
        for(i = 1; i <= 8; i++)  {
	    int digit = _num % 16;
	    if (digit < 10) {
                temp[i] = digit + '0';
	    } else {
		temp[i] = digit - 10 + 'A';
	    }
            _num /= 16;
        }
	*_str++ = '0';
	*_str++ = 'x';
        while( i >= 0 )
                *_str++ = temp[--i];
}

/*--------------------------------------------------------------------------*/
/* PORT I/O OPERATIONS  */ 
/*--------------------------------------------------------------------------*/

/* We will use this later on for reading from the I/O ports to get data
*  from devices such as the keyboard. We are using what is called
*  'inline assembly' in these routines to actually do the work */
char inportb (unsigned short _port) {
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

unsigned short inportw (unsigned short _port) {
    unsigned short rv;
    __asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

/* We will use this to write to I/O ports to send bytes to devices. This
*  will be used in the next tutorial for changing the textmode cursor
*  position. Again, we use some inline assembly for the stuff that simply
*  cannot be done in C */
void outportb (unsigned short _port, char _data) {
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void outportw (unsigned short _port, unsigned short _data) {
    __asm__ __volatile__ ("outw %1, %0" : : "dN" (_port), "a" (_data));
}
