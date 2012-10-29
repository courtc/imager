#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>

#include "gui.h"

#define VERSION "0.1"

static void version(const char *name)
{
	fprintf(stderr,
" - imager version " VERSION ", by Courtney Cavin - \n");
}
static void usage(const char *name)
{
	fprintf(stderr,
"Usage: %s [OPTION]... [<files>]...\n"
"Display images\n"
"  -F, --fullscreen   operate in fullscreen mode\n"
"  -z, --random       randomize files\n"
"  -s, --sort         sort files\n"
"  -D, --delay        delay before automatically switching pictures\n"
"  -v, --version      output version informatin and exit\n"
"  -h, --help         display this help and exit\n"
" - imager version " VERSION ", by Courtney Cavin - \n", name);
}

static unsigned int get_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned int)((tv.tv_sec % 0x3fffff)*1000 + tv.tv_usec/1000);
}

static int addImage(GUI &gui, const char *name, bool recurse)
{
	struct stat st;
	DIR *dir;

	if (stat(name, &st)) {
		perror(name);
		return -1;
	}

	if (!S_ISDIR(st.st_mode)) {
		gui.addImage(name);
		return 0;
	}

	if (!recurse) {
		fprintf(stderr, "%s: Is a directory (forgot -r ?)\n", name);
		return -1;
	}

	dir = opendir(name);
	if (dir == NULL) {
		perror(name);
		return -1;
	}

	struct dirent *d;
	char newname[4096];
	int index;

	index = snprintf(newname, sizeof(newname), "%s", name);

	while ((d = readdir(dir)) != 0) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		snprintf(newname + index, sizeof(newname) - index, "/%s", d->d_name);
		addImage(gui, newname, recurse);
	}

	closedir(dir);

	return 0;
}

int main(int argc, char **argv)
{
	bool fullscreen = false;
	bool random = false;
	bool sort = false;
	bool hasDelay = false;
	bool paused = false;
	bool recurse = false;
	unsigned int delay;
	const char *filelist = NULL;
	int c;

	for (;;) {
		int idx = 0;
		static struct option long_options[] = {
			{"fullscreen",  0, 0, 'F'},
			{"random",      0, 0, 'z'},
			{"sort",        0, 0, 's'},
			{"delay",       1, 0, 'D'},
			{"recurse",     1, 0, 'r'},
			{"filelist",    1, 0, 'f'},
			{"version",     0, 0, 'v'},
			{"help",        0, 0, 'h'},
		};
		c = getopt_long(argc,argv, "Fzf:vhD:sr", long_options, &idx);
		if (c == -1) break;

		switch (c) {
		case 'F':
			fullscreen = true;
			break;
		case 'z':
			random = true;
			break;
		case 's':
			sort = true;
			break;
		case 'D': {
			hasDelay = true;
			double ddelay = strtod(optarg, NULL);
			delay = (ddelay * 1000 + 0.5);
			}
			break;
		case 'r':
			recurse = true;
			break;
		case 'f':
			filelist = optarg;
			break;
		case 'v':
			version(argv[0]);
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		case '?':
		default:
			usage(argv[0]);
			return 1;
		}
	}

	GUI gui(GRE::Dimensions(1024, 768), fullscreen);

	if (filelist != NULL) {
		char line[4096];
		FILE *fp = fopen(filelist, "rt");
		if (fp == NULL) {
			perror(filelist);
			return -1;
		}
		while (fgets(line, 4096, fp)) {
			int len;

			len = strlen(line);
			if (len < 2)
				continue;
			line[len - 1] = 0;
			addImage(gui, line, recurse);
		}
	}

	for (int i = optind; i < argc; ++i) {
		addImage(gui, argv[i], recurse);
	}

	printf("%d images...\n", gui.imageCount());

	if (random) {
		gui.randomSort();
	} else if (sort) {
		gui.logicalSort();
	}

	unsigned int lastframetime;

	lastframetime = get_ms();
	for (;;) {
		GRE::Event ev;

		while (gui.pollEvent(ev) == 0) {
			switch (ev.type) {
			case GRE::Event::Quit:
				return 0;
			case GRE::Event::Next:
				lastframetime = get_ms();
				gui.next();
				break;
			case GRE::Event::Prev:
				lastframetime = get_ms();
				gui.prev();
				break;
			case GRE::Event::HaltToggle:
				paused = !paused;
				break;
			case GRE::Event::FullscreenToggle:
				fullscreen = !fullscreen;
				gui.setVideoMode(GRE::Dimensions(1024, 768), fullscreen);
				break;
			}
		}

		if (hasDelay && !paused) {
			unsigned int now = get_ms();
			if ((now - lastframetime) >= delay) {
				gui.next();
				lastframetime = now;
			}
		}
		gui.render();
		usleep(33000);
	}

	return 0;
}
