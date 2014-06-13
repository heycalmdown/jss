#include <stdio.h>
#include <stdarg.h>
#include "error.h"

char *_prefix = "jss";

void _errprint(char *func, char *format, ... )
{
	va_list vl;
	char errstr[1024];

	printf("%s:%s ", _prefix, func);
	va_start(vl, format);
	vsprintf(errstr, format, vl);
	va_end(vl);
	printf(errstr);
	printf("\n");
}