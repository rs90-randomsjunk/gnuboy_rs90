#undef _GNU_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <signal.h>

#include "defs.h"
#include "input.h"
#include "sys.h"
#include "emu.h"
#include "loader.h"

#include "Version"

bool emuquit = false;

static void banner()
{
	printf("\ngnuboy " VERSION "\n");
}

static void copyright()
{
	banner();
	printf(
"Copyright (C) 2000-2001 Laguna and Gilgamesh\n"
"Portions contributed by other authors; see CREDITS for details.\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
"\n");
}

static void usage(char *name)
{
	copyright();
	printf("Type %s --help for detailed help.\n\n", name);
	exit(1);
}

static void copying()
{
	copyright();
	exit(0);
}

static void help(char *name)
{
	banner();
	printf("Usage: %s [options] romfile\n", name);
	printf("\n"
"      --source FILE             read rc commands from FILE\n"
"      --bind KEY COMMAND        bind KEY to perform COMMAND\n"
"      --VAR=VALUE               set rc variable VAR to VALUE\n"
"      --VAR                     set VAR to 1 (turn on boolean options)\n"
"      --no-VAR                  set VAR to 0 (turn off boolean options)\n"
"      --showvars                list all available rc variables\n"
"      --help                    display this help and exit\n"
"      --version                 output version information and exit\n"
"      --copying                 show copying permissions\n"
"");
	exit(0);
}


static void version(char *name)
{
	printf("%s-" VERSION "\n", name);
	exit(0);
}

static void shutdown()
{
	pcm_close();
	vid_close();
}

void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	loader_unload();
	shutdown();
	exit(1);
}

static int bad_signals[] =
{
	/* These are all standard, so no need to #ifdef them... */
	SIGINT, SIGSEGV, SIGTERM, SIGFPE, SIGABRT, SIGILL,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGPIPE
	SIGPIPE,
#endif
	0
};

static void fatalsignal(int s)
{
	die("Signal %d\n", s);
}

static void catch_signals()
{
	int i;
	for (i = 0; bad_signals[i]; i++)
		signal(bad_signals[i], fatalsignal);
}

static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}

static char *basefolder(char *s)
{
	char *p,*ns;
	p = strrchr(s, '/');
	if (p)
	{
		ns = malloc(p-s+2);
		memset(ns, 0, p-s+2);
		strncpy(ns,s,(p-s+1));
		return ns;
	}
	ns = malloc(2);
	sprintf(ns, ".");
	return ns;
}

extern void cleanup();

int main(int argc, char *argv[])
{
	char* rom, s;
	
	sys_sanitize(argv[0]);

	rom = argv[1];

	if (!rom) usage(base(argv[0]));
	sys_sanitize(rom);

	/* If we have special perms, drop them ASAP! */
	vid_preinit();
	
	s = strdup(argv[0]);
	sys_sanitize(s);
	sys_initpath(s);

	catch_signals();
	vid_init();

	pcm_init();

	rom = strdup(rom);

	sys_sanitize(rom);

	loader_init(rom);

	emu_reset();
	emu_run();

	cleanup();
	loader_unload();
	shutdown();

	/* never reached */
	return 0;
}
