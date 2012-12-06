#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>
#include <termios.h>
#include <poll.h>

#include "gui.h"
#include "thread.h"
extern "C" {
#include "server.h"
}

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
"  -F, --fullscreen     operate in fullscreen mode\n"
"  -z, --random         randomize files\n"
"  -s, --sort           sort files\n"
"  -r, --recurse        recurse directories\n"
"  -f, --filelist <lst> read file names from list (one file per line)\n"
"  -D, --delay <time>   delay before automatically switching pictures\n"
"  -a, --fade  <time>   amount of time to dedicate to fading between pictures\n"
"  -v, --version        output version information and exit\n"
"  -h, --help           display this help and exit\n"
"      in-app key support: \n"
"          h         -  toggle pause\n"
"          f         -  toggle fullscreen\n"
"          alt-enter -  toggle fullscreen\n"
"          alt-tab   -  exit fullscreen (SDL is broken)\n"
"          mwheel-up -  prev image\n"
"          mwheel-dn -  next image\n"
"          left      -  prev image\n"
"          right     -  next image\n"
"          return    -  next image\n"
"          spacebar  -  next image\n"
"          esc       -  quit\n"
"          q         -  quit\n"
	, name);
	version(name);
}

struct CharBuffer {
public:
	CharBuffer() : read(0),write(0) { }
	void push(char ch)
	{
		buf[write++] = ch;
		count++;
	}

	char *peek(void)
	{
		return &buf[read];
	}
	char pop(void)
	{
		count--;
		return buf[read++];
	}

	int size(void) const
	{ return count; }

private:
	int count;
	char buf[256];
	int read;
	int write;

};

struct TTY {
public:
	TTY(void)
	{

		struct termios nsettings;
		tcgetattr(0, &_settings);
		memcpy(&nsettings, &_settings, sizeof(struct termios));
		nsettings.c_lflag &= ~(ICANON | ECHO | ISIG);
		nsettings.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
		nsettings.c_cc[VTIME] = 0;
		tcgetattr(0, &_settings);
		nsettings.c_cc[VMIN] = 1;
		tcsetattr(0, TCSANOW, &nsettings);
	}

	~TTY()
	{
		tcsetattr(0, TCSANOW, &_settings);
	}

private:
	struct termios _settings;
};

class CLI {
public:
	CLI() : m_tty(NULL), m_buffer(NULL), m_enabled(false) { }
	~CLI()
	{
		if (m_tty != NULL) delete m_tty;
		if (m_buffer != NULL) delete m_buffer;
	}

	void enable(void)
	{
		if (m_enabled) return;
		m_enabled = true;
		m_tty = new TTY;
		m_buffer = new CharBuffer;
	}

	int pollEvent(GRE::Event &ev);

private:
	TTY *m_tty;
	CharBuffer *m_buffer;
	bool m_enabled;
};


int CLI::pollEvent(GRE::Event &ev)
{
	struct pollfd fd;
	bool food = false;
	char ch;
	int idx;
	int r;

	if (!m_enabled)
		return -1;

	idx = 0;
	do {
		fd.fd = STDIN_FILENO;
		fd.events = POLLIN | POLLRDHUP;
		fd.revents = 0;

		r = poll(&fd, 1, 0);
		if (r < 0) {
			m_enabled = false;
			return -1;
		} else if (r == 0) {
			break;
		}

		if (fd.revents & POLLRDHUP) {
			m_enabled = false;
			return -1;
		} else {
			r = read(STDIN_FILENO, &ch, 1);
			if (r == 1) {
				m_buffer->push(ch);
			} else {
				m_enabled = false;
				return -1;
			}
		}
	} while (r > 0);

	while (m_buffer->size()) {
		food = true;
		switch ((ch = m_buffer->pop())) {
		case 0xd:
		case 0x20:
			ev.type = GRE::Event::Next;
			break;
		case 'q':
			ev.type = GRE::Event::Quit;
			break;
		case 'h':
			ev.type = GRE::Event::HaltToggle;
			break;
		case 'F':
			ev.type = GRE::Event::FilterToggle;
			break;
		case 'f':
			ev.type = GRE::Event::FullscreenToggle;
			break;
		case 0x1b:
			if (m_buffer->size() < 2) {
				ev.type = GRE::Event::Quit;
				break;
			}
			if (!memcmp(m_buffer->peek(), "[C", 2)) {
				ev.type = GRE::Event::Next;
				m_buffer->pop(), m_buffer->pop();
				break;
			}
			if (!memcmp(m_buffer->peek(), "[D", 2)) {
				ev.type = GRE::Event::Prev;
				m_buffer->pop(), m_buffer->pop();
				break;
			}
			food = false;
			break;
		default:
			food = false;
			break;
		}
		if (food)
			break;
	}

	return -(food == false);
}


static int addImage(GUI &gui, const char *name, bool recurse)
{
	struct stat st;
	DIR *dir;

	if (!recurse) {
		gui.addImage(name);
		return 0;
	}

	if (stat(name, &st)) {
		perror(name);
		return -1;
	}

	if (!S_ISDIR(st.st_mode)) {
		gui.addImage(name);
		return 0;
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

class Server : public Runnable
{
public:
	Server(int port, GUI &gui)
	: m_gui(gui), m_port(port), m_semaphore(0)
	{
		m_thread = new Thread(*this);
	}

	~Server()
	{
		m_semaphore.post();
		m_thread->join();

		delete m_thread;
	}


	void run(void)
	{
		server_t *server;
		char line[4096];
		int n_wait = 0;

		server = server_create(m_port);

		while (m_semaphore.wait(n_wait) != 0) {
			if (server_readline(server, line, sizeof(line)) == 0) {
				int len;

				len = strcspn(line, "\n\r");
				if (len == 0)
					continue;
				line[len] = 0;
				addImage(m_gui, line, false);
				n_wait = 0;
			} else {
				n_wait = 10;
			}
		}
		server_destroy(server);
	}

private:
	GUI &m_gui;
	int m_port;
	Semaphore m_semaphore;
	Thread *m_thread;
};

int main(int argc, char **argv)
{
	bool fullscreen = false;
	bool random = false;
	bool sort = false;
	bool sortdir = false;
	bool hasDelay = false;
	bool hasFade  = false;
	bool paused = false;
	bool recurse = false;
	bool filtering = true;
	bool text = false;
	int listenport = -1;
	int offset = 0;
	int currentImage = 0;
	Server *server = NULL;
	Timestamp delay;
	Timestamp fade;
	CLI cli;
	const char *filelist = NULL;
	int c;

	for (;;) {
		int idx = 0;
		static struct option long_options[] = {
			{"fullscreen",  0, 0, 'F'},
			{"random",      0, 0, 'z'},
			{"sort",        0, 0, 's'},
			{"randir",      0, 0, 'd'},
			{"delay",       1, 0, 'D'},
			{"fade",        1, 0, 'a'},
			{"listen",      1, 0, 'l'},
			{"recurse",     0, 0, 'r'},
			{"offset",      0, 0, 'o'},
			{"nofilter",    0, 0, 'n'},
			{"stdin",       0, 0, 'S'},
			{"filelist",    1, 0, 'f'},
			{"version",     0, 0, 'v'},
			{"help",        0, 0, 'h'},
		};
		c = getopt_long(argc,argv, "FzvhsordSl:f:a:D:", long_options, &idx);
		if (c == -1) break;

		switch (c) {
		case 'n':
			filtering = false;
			break;
		case 'F':
			fullscreen = true;
			break;
		case 'z':
			random = true;
			break;
		case 's':
			sort = true;
			break;
		case 'd':
			sortdir = true;
			break;
		case 'S':
			cli.enable();
			break;
		case 'o':
			offset = 1;
			break;
		case 'l':
			listenport = strtol(optarg, 0, 0);
			break;
		case 'a': {
			hasFade = true;
			double dfade = strtod(optarg, NULL);
			fade = (dfade * 1000 + 0.5);
			}
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

	if (listenport != -1)
		server = new Server(listenport, gui);

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
		srand(time(NULL));
		gui.randomSort();
	} else if (sort) {
		gui.logicalSort();
	} else if (sortdir) {
		srand(time(NULL));
		gui.directorySort();
	}

	if (offset) {
		srand(time(NULL));
		gui.randomOffset();
	}

	if (hasFade && fade != 0) {
		gui.setFadeDuration(fade);
	}

	gui.start();
	gui.enableSpinner(false);
	gui.enableFiltering(filtering);
	gui.enableText(text);

	Timestamp lastframetime = Time::MS();
	for (;;) {
		GRE::Event ev;

		while (gui.pollEvent(ev) == 0 || cli.pollEvent(ev) == 0) {
			switch (ev.type) {
			case GRE::Event::Quit:
				return 0;
			case GRE::Event::Next:
				if (currentImage < 0)
					currentImage = 0;
				currentImage++;
				break;
			case GRE::Event::Prev:
				if (currentImage > 0)
					currentImage = 0;
				currentImage--;
				break;
			case GRE::Event::TextToggle:
				text = !text;
				gui.enableText(text);
				break;
			case GRE::Event::HaltToggle:
				paused = !paused;
				currentImage = 0;
				break;
			case GRE::Event::FullscreenToggle:
				fullscreen = !fullscreen;
				gui.setVideoMode(GRE::Dimensions(1024, 768), fullscreen);
				break;
			case GRE::Event::FilterToggle:
				filtering = !filtering;
				gui.enableFiltering(filtering);
				break;
			}
		}

		if (hasDelay && !paused) {
			Timestamp now = Time::MS();
			if ((now - lastframetime) >= delay) {
				if (currentImage == 0)
					currentImage++;
			}
		}

		if (currentImage != 0) {
			int rc;
			int way = 0;

			if (currentImage > 0) {
				rc = gui.next();
				way = 1;
			} else {
				rc = gui.prev();
				way = -1;
			}
			if (rc == 0) {
				gui.enableSpinner(false);
				currentImage += -way;
				lastframetime = Time::MS();
			} else {
				gui.enableSpinner(true);
			}
		}
		gui.render();
		usleep(33000);
	}

	if (server != NULL)
		delete server;

	return 0;
}
