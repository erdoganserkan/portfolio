#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <log.h>


int32_t open_serial_port(int8_t *dev)
{
    int32_t fd;

    fd = open(dev, O_RDWR | O_NOCTTY);
#if 0
    if (fd != -1) {
        fcntl(fd, F_SETFL, 0);
    }
#endif
    return fd;
}

void init_serial_port(int32_t fd)
{
    struct termios options;

    if(0 != tcgetattr(fd, &options)) {
    	ERRL(logi, "tcgetattr(MCU_Serial) FAILED:(%s)\n", strerror(errno));
    	return;
    }

#if(MCU_SERIAL_BAUDRATE == 115200)
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
#else
	#error "What to DO?"
#endif
    /* Set up for 8N1, 8 data bits, no parity, 1 stop bit. Disable hard. flow*/
    options.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS | IXON | CSIZE);
    options.c_cflag |= (CS8 | CLOCAL | CREAD);

    options.c_iflag = IGNPAR;

    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VTIME] = 1;	// wait for 0.1 seconds before first char is ready @ input //
    options.c_cc[VMIN] = 1;		// wait for minimum 1 character //

    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd, TCSANOW | TCSADRAIN, &options);
}

int close_serial_port(int32_t fd)
{
    int res;
    struct termios options;
    if(0 < fd) {
		tcdrain(fd);
		tcgetattr(fd, &options);
		options.c_cflag &= ~CRTSCTS;
		tcsetattr(fd, TCSANOW | TCSADRAIN, &options);
		return res = close(fd);
    } else
    	return -1;
}

int32_t read_serial_port(int32_t fd, uint8_t *result, uint32_t len)
{
    int in = read(fd, result, len);

    if (0 > in) {
    	if((EWOULDBLOCK == errno) || (EAGAIN == errno) || (EINTR == errno) || (EINPROGRESS == errno)) {
            DEBUGL(logi, "SERIAL EAGAIN ERROR\n");
        }
        else {
            DEBUGL(logi, " SERIAL read error %d %s\n", errno, strerror(errno));
        }
    }

    return in;
}

int32_t write_serial_port(int32_t fd, uint8_t *buf, int32_t len)
{
    int32_t ret = write(fd, buf, len);
    tcdrain(fd);
    return ret;
}

int32_t get_baud_rate(int32_t fd)
{
    struct termios termAttr;
    int inputSpeed = -1;
    speed_t baudRate;
    tcgetattr(fd, &termAttr);
    /* Get the input speed.*/
    baudRate = cfgetispeed(&termAttr);
    switch (baudRate) {
    case B0:
        inputSpeed = 0;
        break;
    case B50:
        inputSpeed = 50;
        break;
    case B110:
        inputSpeed = 110;
        break;
    case B134:
        inputSpeed = 134;
        break;
    case B150:
        inputSpeed = 150;
        break;
    case B200:
        inputSpeed = 200;
        break;
    case B300:
        inputSpeed = 300;
        break;
    case B600:
        inputSpeed = 600;
        break;
    case B1200:
        inputSpeed = 1200;
        break;
    case B1800:
        inputSpeed = 1800;
        break;
    case B2400:
        inputSpeed = 2400;
        break;
    case B4800:
        inputSpeed = 4800;
        break;
    case B9600:
        inputSpeed = 9600;
        break;
    case B19200:
        inputSpeed = 19200;
        break;
    case B38400:
        inputSpeed = 38400;
        break;
    case B57600:
        inputSpeed = 57600;
        break;
    case B115200:
        inputSpeed = 115200;
        break;
    }
    return inputSpeed;
}
