/*
	_                _    _                      
  | |__   __ _  ___| | _| |_ _ __ __ _  ___ ___ 
  | '_ \ / _` |/ __| |/ / __| '__/ _` |/ __/ _ \
  | |_) | (_| | (__|   <| |_| | | (_| | (_|  __/
  |_.__/ \__,_|\___|_|\_\\__|_|  \__,_|\___\___|
*/                                                                                                                                 
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static inline void print_stacktrace (void **fake, unsigned int max_frames = 63)
{
	FILE *out = stderr;
	fprintf (out, "stack trace:\n");

	// storage array for stack trace address data
	void *addrlist[max_frames + 1];

	// retrieve current stack addresses
	int addrlen = backtrace (addrlist, sizeof (addrlist) / sizeof (void *));

	if (addrlen == 0)
	{
		fprintf (out, "  <empty, possibly corrupt>\n");
		return;
	}

	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char **symbollist = backtrace_symbols (addrlist, addrlen);

	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char *funcname = (char *) malloc (funcnamesize);

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < addrlen; i++)
	{
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x15c) [0x8048a6d]
		for (char *p = symbollist[i]; *p; ++p)
		{
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if (*p == ')' && begin_offset)
			{
				end_offset = p;
				break;
			}
		}

		if (begin_name && begin_offset && end_offset
				&& begin_name < begin_offset)
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply
			// __cxa_demangle():

			int status = 0;
			char *ret;
			//char* ret = abi::__cxa_demangle(begin_name,
			//                                                  funcname, &funcnamesize, &status);
			if (status == 0)
			{
				funcname = ret;	// use possibly realloc()-ed string
				fprintf (out, "  %s : %s+%s\n",
						symbollist[i], funcname, begin_offset);
			}
			else
			{
				// demangling failed. Output function name as a C function with
				// no arguments.
				fprintf (out, "  %s : %s()+%s\n",
						symbollist[i], begin_name, begin_offset);
			}
		}
		else
		{
			// couldn't parse the line? print the whole line.
			fprintf (out, "  %s\n", symbollist[i]);
		}
	}

	free (funcname);
	free (symbollist);
}


void myfunc3 (void)
{
	int j, nptrs;
#define SIZE 100
	void *buffer[100];
	char **strings;

	//nptrs = print_stacktrace (buffer, SIZE);
	nptrs = backtrace (buffer, SIZE);
	printf ("print_stacktrace() returned %d addresses\n", nptrs);

	/* The call print_stacktrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
		would produce similar output to the following: */

	strings = backtrace_symbols (buffer, nptrs);
	if (strings == NULL)
	{
		perror ("backtrace_symbols");
		exit (EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf ("%s\n", strings[j]);

	free (strings);
}

static void			/* "static" means don't export the symbol... */
myfunc2 (void)
{
	myfunc3 ();
}

void myfunc (int ncalls)
{
	if (ncalls > 1)
		myfunc (ncalls - 1);
	else
		myfunc2 ();
}

int main (int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf (stderr, "%s num-calls\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	myfunc (atoi (argv[1]));
	exit (EXIT_SUCCESS);
}
