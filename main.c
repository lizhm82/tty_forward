/**
 * @file 
 * @brief	TTY forward
 * @date	2014/03/27
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <errno.h>
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

////////////////////////////////////////////////////////////////////////

struct serial_data {
	int	fd;
	char devname[64];
	struct termios	tty;
	struct termios	old_tty;
};

struct serial_data* serial_open(const char *devname)
{
	struct serial_data* _sd = NULL;
	int fd;

	fd = open(devname, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		printf("open serial device %s error. (%s)\n", devname, strerror(errno));
		return NULL;
	}

	_sd = (struct serial_data*)malloc(sizeof(struct serial_data));
	snprintf(_sd->devname, sizeof(_sd->devname), "%s", devname);
	_sd->fd = fd;

	tcgetattr(fd, &_sd->old_tty);	// backup old configure
	//_sd->tty = _sd->old_tty;
	bzero(&_sd->tty, sizeof(_sd->tty));

	return _sd;
}

void serial_close(struct serial_data *_sd)
{
	if (_sd == NULL)
		return;
	
	/* restore the old configure */
	tcsetattr(_sd->fd, TCSANOW, &_sd->old_tty);
	close(_sd->fd);

	free(_sd);
}

void serial_setparams(struct serial_data *_sd, int baudrate, char parity, int databits, int stopbits, int hwflow)
{
	struct termios* ptty = &_sd->tty;
	int fd = _sd->fd;
	int spd = -1;

	switch (baudrate) {
		case 38400:		spd = B38400;	break;
		case 57600:		spd = B57600;	break;
		case 115200:	spd = B115200;	break;
		case 230400:	spd = B230400;	break;
		case 460800:	spd = B460800;	break;
		case 500000:	spd = B500000;	break;
		case 576000:	spd = B576000;	break;
		case 921600:	spd = B921600;	break;
		case 1000000:	spd = B1000000;	break;
		case 1152000:	spd = B1152000;	break;
		case 1500000:	spd = B1500000;	break;
		case 2000000:	spd = B2000000;	break;
		case 2500000:	spd = B2500000;	break;
		case 3000000:	spd = B3000000;	break;
		case 3500000:	spd = B3500000;	break;
		case 4000000:	spd = B4000000;	break;
	}

	if (spd != -1) {
		cfsetispeed(ptty, (speed_t)spd);
		cfsetospeed(ptty, (speed_t)spd);
	
	}

	switch (databits) {
	case 5:
		ptty->c_cflag = (ptty->c_cflag & ~CSIZE) | CS5;
		break;
	case 6:
		ptty->c_cflag = (ptty->c_cflag & ~CSIZE) | CS6;
		break;
	case 7:
		ptty->c_cflag = (ptty->c_cflag & ~CSIZE) | CS7;
		break;
	case 8:
		ptty->c_cflag = (ptty->c_cflag & ~CSIZE) | CS8;
		break;
	}
	
	/* set into raw, no echo mode */
	ptty->c_iflag = IGNBRK;
	ptty->c_lflag = 0;
	ptty->c_oflag = 0;
	ptty->c_cflag |= CLOCAL | CREAD;

	ptty->c_cc[VMIN] = 1;
	ptty->c_cc[VTIME] = 5;	// unit : 100ms
	
	if (0) // software flow
		ptty->c_iflag |= IXON | IXOFF;
	else
		ptty->c_iflag &= ~(IXON | IXOFF | IXANY);

	ptty->c_cflag &= ~(PARENB | PARODD);
	ptty->c_cflag &= ~CMSPAR;
	switch (parity) {
	case 'e':
	case 'E':
		ptty->c_cflag |= PARENB;
		break;
	case 'o':
	case 'O':
		ptty->c_cflag |= (PARENB | PARODD);
		break;
	case 's':
	case 'S':
		ptty->c_cflag |= (PARENB | CMSPAR);
		break;
	case 'm':
	case 'M':
		ptty->c_cflag |= (PARENB | PARODD | CMSPAR);
		break;
	case 'n':
	case 'N':
	default:
		break;
	}

	switch (stopbits) {
	case 2:
		ptty->c_cflag |= CSTOPB;
		break;
	case 1:
	default:
		ptty->c_cflag &= ~CSTOPB;
		break;
	}

	if (hwflow) {
		ptty->c_cflag |= CRTSCTS;
	} else {
		ptty->c_cflag &= ~CRTSCTS;
	}
	
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, ptty);
}

static int serial_read(struct serial_data *_sd, void *buf, int len)
{
	int ret;

	ret = read(_sd->fd, buf, len);
	if (ret <= 0) {
		if (ret == 0) {
			printf("read %s EOF\n", _sd->devname);
		} else if (ret < 0 && errno == EINTR) {
			printf("read %s interrupt!\n", _sd->devname);
		} else {
			printf("read %s error - %s\n", _sd->devname, strerror(errno));
		}
	}

	return ret;
}

static int serial_write(struct serial_data *_sd, void *buf, int len)
{
	int ret;

	ret = write(_sd->fd, buf, len);

	return ret;
}
////////////////////////////////////////////////////////////////////////

static void show_usage(const char* program)
{
	printf("Usage:\n");
	printf("%s <%s> <%s>\n", program, "tty device 1", "tty device 2");
	printf("\n");
	printf("tty device syntax : <device name>[,baud rate]\n");
	printf("Examples:\n");
	printf("\t%s /dev/ttyP1,115200 /dev/ttyGS0,115200\n", program);

}


static void start_forward(struct serial_data *s1, struct serial_data *s2)
{
	int ret;
	char buf[1024];
	fd_set fds;
	int maxfd = s1->fd > s2->fd ? s1->fd : s2->fd;
	int quit = 0;

	while (!quit) {
		FD_ZERO(&fds);
		FD_SET(s1->fd, &fds);
		FD_SET(s2->fd, &fds);

		ret = select(maxfd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0 && errno != EINTR && errno != 0) {
			printf("select error - %s\n", strerror(errno));
			quit = 1;
			break;
		}

		if (FD_ISSET(s1->fd, &fds)) {
			ret = serial_read(s1, buf, sizeof(buf));
			if (ret > 0)
				serial_write(s2, buf, ret);
			else
				quit = 1;
		}

		if (FD_ISSET(s2->fd, &fds)) {
			ret = serial_read(s2, buf, sizeof(buf));
			if (ret > 0)
				serial_write(s1, buf, ret);
			else
				quit = 1;
		}

	}
}

/* input string : <serial name>[,baud] */
static void parse_serial_name(const char *s, char **name, int *baud, int defbaud)
{
	char *tmp;
	char *ss = strdup(s);

	tmp = strchr(ss, ',');
	if (tmp == NULL)
	{
		if (baud != NULL)
		{
			// give a default value
			*baud = defbaud;
		}

		if (name != NULL)
		{
			*name = ss;
		}
		else
		{
			*name = NULL;
			free(ss);
		}

	}
	else
	{
		if ((baud != NULL))
		{
			if (tmp[1] != '\0')
			{
				*baud = atoi(&tmp[1]);
			}
			else
			{
				// default
				*baud = defbaud;
			}
		}

		if (name != NULL)
		{
			if (tmp != ss)
			{
				*tmp = '\0';
				*name = ss;
			}
		}
		else
		{
			*name = NULL;
			free(ss);
		}
	}

}

int main(int argc, char** argv)
{
	int i;
	struct serial_data *s1, *s2;
	char *s1name, *s2name;
	int s1baud, s2baud;

	if (argc != 3) {
		show_usage(argv[0]);
		return -1;
	}

	parse_serial_name(argv[1], &s1name, &s1baud, 115200);
	parse_serial_name(argv[2], &s2name, &s2baud, 115200);

	s1 = serial_open(s1name);
	if (s1 == NULL) {
		printf("cannot open %s\n", s1name);
		return -1;
	}

	s2 = serial_open(s2name);
	if (s2 == NULL) {
		printf("cannot open %s\n", s2name);
		serial_close(s1);
		return -1;
	}

	free(s1name);
	free(s2name);

	serial_setparams(s1, s1baud, 'n', 8, 1, 0);
	serial_setparams(s2, s2baud, 'n', 8, 1, 0);

	start_forward(s1, s2);

	serial_close(s2);
	serial_close(s1);


	return 0;
}
