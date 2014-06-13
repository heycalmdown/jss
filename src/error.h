#ifndef _ERROR_H
#define _ERROR_H

#if defined (_DEBUG) || defined (DEBUG)
#define errprint(...)		_errprint(__FUNCTION__, ##__VA_ARGS__)
#define errscope(X)			{##X}
#else
#define errprint(...)
#define errscope(X)
#endif



void _errprint(char *func, char *format, ... );

#endif
