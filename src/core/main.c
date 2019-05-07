





#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strdup();

#include <stdarg.h>
#include <signal.h>

#include "defs.h"
#include "input.h"
#include "rc.h"


#include "Version"

bool emuquit = false;

static char *defaultconfig[] =
{
	"bind up +up",
	"bind down +down",
	"bind left +left",
	"bind right +right",
	"bind ctrl +a",
	"bind alt +b",
	"bind enter +start",
	"bind esc +select",
	"bind tab savestate",
	"bind backspace loadstate",
	"source gnuboy.rc",
	NULL
};


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


void doevents()
{
	event_t ev;
	int st;

	ev_poll();
	while (ev_getevent(&ev))
	{
		if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
			continue;
		st = (ev.type != EV_RELEASE);
		rc_dokey(ev.code, st);
	}
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
	int i;
	FILE* fp;
	char *opt, *arg, *cmd, *s, *rom = 0, *var, *var2;

	sys_sanitize(argv[0]);

	/* Avoid initializing video if we don't have to */
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--help"))
			help(base(argv[0]));
		else if (!strcmp(argv[i], "--version"))
			version(base(argv[0]));
		else if (!strcmp(argv[i], "--copying"))
			copying();
		else if (!strcmp(argv[i], "--bind")) i += 2;
		else if (!strcmp(argv[i], "--source")) i++;
		else if (!strcmp(argv[i], "--showvars"))
		{
			show_exports();
			exit(0);
		}
		else if (argv[i][0] == '-' && argv[i][1] == '-');
		else if (argv[i][0] == '-' && argv[i][1]);
		else rom = argv[i];
	}

	if (!rom) usage(base(argv[0]));
	sys_sanitize(rom);

	/* If we have special perms, drop them ASAP! */
	vid_preinit();
	init_exports();

	// load skin specific to rom - <romname>.tga
	var = malloc(strlen(rom)+5);
	strcpy(var,rom);
	s = strrchr(var, '.');
	if (s) *s = 0;
	strcat(s, ".tga");

	// load default skin - gnuboy.tga
	var2 = basefolder(rom);
	var2 = realloc(var2,strlen(var2) + strlen("gnuboy.tga") + 1);
	sprintf(var2,"%sgnuboy.tga",var2);

	s = strdup(argv[0]);
	sys_sanitize(s);
	sys_initpath(s);

	for (i = 0; defaultconfig[i]; i++)
		rc_command(defaultconfig[i]);

	cmd = malloc(strlen(rom) + 11);
	sprintf(cmd, "source %s", rom);
	s = strrchr(cmd, '.');
	if (s) *s = 0;
	strcat(cmd, ".rc");
	rc_command(cmd);


	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--bind"))
		{
			if (i + 2 >= argc) die("missing arguments to bind\n");
			cmd = malloc(strlen(argv[i+1]) + strlen(argv[i+2]) + 9);
			sprintf(cmd, "bind %s \"%s\"", argv[i+1], argv[i+2]);
			rc_command(cmd);
			free(cmd);
			i += 2;
		}
		else if (!strcmp(argv[i], "--source"))
		{
			if (i + 1 >= argc) die("missing argument to source\n");
			cmd = malloc(strlen(argv[i+1]) + 6);
			sprintf(cmd, "source %s", argv[++i]);
			rc_command(cmd);
			free(cmd);
		}
		else if (!strncmp(argv[i], "--no-", 5))
		{
			opt = strdup(argv[i]+5);
			while ((s = strchr(opt, '-'))) *s = '_';
			cmd = malloc(strlen(opt) + 7);
			sprintf(cmd, "set %s 0", opt);
			rc_command(cmd);
			free(cmd);
			free(opt);
		}
		else if (argv[i][0] == '-' && argv[i][1] == '-')
		{
			opt = strdup(argv[i]+2);
			if ((s = strchr(opt, '=')))
			{
				*s = 0;
				arg = s+1;
			}
			else arg = "1";
			while ((s = strchr(opt, '-'))) *s = '_';
			while ((s = strchr(arg, ','))) *s = ' ';

			cmd = malloc(strlen(opt) + strlen(arg) + 6);
			sprintf(cmd, "set %s %s", opt, arg);

			rc_command(cmd);
			free(cmd);
			free(opt);
		}
		/* short options not yet implemented */
		else if (argv[i][0] == '-' && argv[i][1]);
	}

	/* FIXME - make interface modules responsible for atexit() */
	//atexit(shutdown);
	catch_signals();
	vid_init(var,var2);
	free(var);
	free(var2);
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











