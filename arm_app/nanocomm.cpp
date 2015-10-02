/*
 * nanocomm.cpp
 *
 * minimal terminal emulator for testing serial ports.
 *
 * Usage:
 *	nanocomm /dev/ttymxc4 11500 [8 [N [1]]]
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/poll.h>
#include <ctype.h>
#include <stdlib.h>
#include <linux/serial.h>
#include <string.h>
#include <termios.h>

#include <iostream> 

using namespace std; 

static unsigned const _standardBauds[] = {
	0,
	50,
	75,
	110,
	134,
	150,
	200,
	300,
	600,
	1200,
	1800,
	2400,
	4800,
	9600,
	19200,
	38400
};
static unsigned const numStandardBauds = sizeof(_standardBauds) / sizeof(_standardBauds[0]);

static unsigned const _highSpeedMask = 0010000 ;
static unsigned const _highSpeedBauds[] = {
	0,
	57600,
	115200,
	230400,
	460800,
	500000,
	576000,
	921600,
	1000000,
	1152000,
	1500000,
	2000000,
	2500000,
	3000000,
	3500000,
	4000000
};

static unsigned const numHighSpeedBauds = sizeof(_highSpeedBauds)/sizeof(_highSpeedBauds[0]);
bool baudRateToConst(unsigned bps, unsigned &constant)
{
	unsigned baudIdx = 0 ;
	bool haveBaud = false ;

	unsigned i ;
	for (i = 0 ; i < numStandardBauds ; i++) {
		if (_standardBauds[i] == bps) {
			haveBaud = true ;
			baudIdx = i ;
			break;
		}
	}

	if (!haveBaud) {
		for (i = 0 ; i < numHighSpeedBauds ; i++) {
			if (_highSpeedBauds[i] == bps) {
				haveBaud = true ;
				baudIdx = i | _highSpeedMask ;
				break;
			}
		}
	}

	constant = baudIdx ;
	return haveBaud ;
}

static bool volatile doExit = false ;
static void ctrlcHandler(int signo)
{
	printf("<ctrl-c>\r\n");
	doExit = true ;
	printf( "ctrl handler process !\n") ;
}

static int setRaw(int fd,
		    int baud,
		    int databits,
		    char parity,
		    unsigned stopBits,
		    struct termios &oldState)
{
	tcgetattr(fd,&oldState);

	/* set raw mode for keyboard input */
	struct termios newState = oldState;
	newState.c_cc[VMIN] = 1;

	bool nonStandard = false ;
	unsigned baudConst ;
	if (baudRateToConst(baud, baudConst)) {
		cfsetispeed(&newState, baudConst);
		cfsetospeed(&newState, baudConst);
	}
	else {
		cfsetispeed(&newState,B38400);
		cfsetospeed(&newState,B38400);
		bool worked = false ;
		struct serial_struct nuts;
		int rval = ioctl(fd, TIOCGSERIAL, &nuts);
		if (0 == rval) {
			unsigned const divisor = nuts.baud_base / baud ;
			nuts.custom_divisor = divisor ;
			nuts.flags &= ~ASYNC_SPD_MASK;
			nuts.flags |= ASYNC_SPD_CUST;
			rval = ioctl(fd, TIOCSSERIAL, &nuts);
			if (0 == rval) {
				printf("baud changed\n");
				rval = ioctl(fd, TIOCGSERIAL, &nuts);
				if (0 == rval) {
					printf("divisor is now %u\n", nuts.custom_divisor);
				}
				else {
					printf("TIOCGSERIAL2\n");
					return -1 ;
				}
			}
			else {
				printf("TIOCSSERIAL\n");
				return -2 ;
			}
		}
		else {
			printf("TIOCGSERIAL\n");
			return -3 ;
		}
	} // non-standard serial

	//
	// Note that this doesn't appear to work!
	// Reads always seem to be terminated at 16 chars!
	//
	newState.c_cc[VTIME] = 0; // 1/10th's of a second, see http://www.opengroup.org/onlinepubs/007908799/xbd/termios.html

	newState.c_cflag &= ~(PARENB|CSTOPB|CSIZE|CRTSCTS);		 // Mask character size to 8 bits, no parity, Disable hardware flow control

	if ('E' == parity) {
		newState.c_cflag |= PARENB ;
		newState.c_cflag &= ~PARODD ;
	}
	else if ('O' == parity) {
		newState.c_cflag |= PARENB | PARODD ;
	}
	else if ('S' == parity) {
		newState.c_cflag |= PARENB | IGNPAR | CMSPAR ;
		newState.c_cflag &= ~PARODD ;
	}
	else if ('M' == parity) {
		newState.c_cflag |= PARENB | IGNPAR | CMSPAR | PARODD ;
	}
	else {
	} // no parity... already set

	newState.c_cflag |= (CLOCAL | CREAD |CS8);			 // Select 8 data bits
	if (7 == databits) {
		newState.c_cflag &= ~CS8 ;
	}

	if (1 != stopBits)
		newState.c_cflag |= CSTOPB ;

	newState.c_lflag &= ~(ICANON | ECHO);				 // set raw mode for input
	newState.c_iflag &= ~(IXON | IXOFF | IXANY|INLCR|ICRNL|IUCLC);	 //no software flow control
	newState.c_oflag &= ~OPOST;			 //raw output
	tcsetattr(fd, TCSANOW, &newState);
	tcflush(fd, TCIFLUSH);	
	return 0 ;
}

extern string RSAEncryptString(const char *pubString, const char *seed, const char *message) ;
extern string RSADecryptString(const char *privString, const char *ciphertext) ;
//extern char *pubKey ;
//extern char *priKey ;
//extern char *seed ;
char priKey[] = "30820276020100300D06092A864886F70D0101010500048202603082025C02010002818100E2497AA07A7D9AD090BE33D485B3643F6B495D1E2A1B4275F9BAF65263A8C392A86EC62ADE81801AE1C36D600F83A480907575DC426C17E466789481267A880BF29747FD440E02A25D069D5B263038A0311394E6A5947360DF9DE40A65E08B388601CA45FFF7676346B590E38FD95C781A4EECCB6288324F3A0DD75A15050DC70201110281800D4F9DCD3461BDD00883A8B225FB7E5E155EAB1FE45BF4D9C36559C89C7356CC6442C05CDFE9800194CF42AB4C34EB8F178E7058402479E04243540798DA08009794BDA3746D7727F225E783783E6DCF6E9288249857C0057A2BBBBC90F11AF77C4B9DBA99D508F3344EE0D9701F9E474B6CEC0CD1B1C8B66D1D9CD9F9F0AE11024100F9F0EF0B3DC838F62DDF9F23FF169A2D0DAE7011479B33E0E21E82CBCD43C83FD514FEA5E64187AD4BFA09DA6444824243F6957A0B386A9289C65D91AF01B4F7024100E7C5C1174B00E0051AA29E7C2AF453ACCBAA1A6740257F22E097E9B8F899F8896FE7523BE290478F817E9898B97B5779D41CAA776A81739F71500F4ECD07C9B10241008452606F5CF187916394816D68EDD926E9201D365315EE49E11F362FA8E7A63FE94759A31F8C1AA70A1AF62853335404F6CDD6AA05F0B0E42AD26DC598E2C937024100CC8150057E6A2F138FF8E6315313D15C3B3BBCF1B11206C46BD155C153D3269762BD0C52F515E4C9EABAFF1D585DC5A7BB285A2D21BD842345DD3AAEF124FD41024100C009216716A87B7B85CBBA57971AFF1A0207F702350E6FFD88F1121324510E08380533CA5D2EB1A8CB5F02384A8DD29295A914C29F6291AC852BF5E63809A25E" ;
char pubKey[] = "30819D300D06092A864886F70D010101050003818B0030818702818100E2497AA07A7D9AD090BE33D485B3643F6B495D1E2A1B4275F9BAF65263A8C392A86EC62ADE81801AE1C36D600F83A480907575DC426C17E466789481267A880BF29747FD440E02A25D069D5B263038A0311394E6A5947360DF9DE40A65E08B388601CA45FFF7676346B590E38FD95C781A4EECCB6288324F3A0DD75A15050DC7020111" ;
char seed[] = "PeterWSH2010" ;


static int databits = 8 ;
static char parity = 'N' ;
static unsigned stopBits = 1 ;
static int fdSerial = -1 ;
static int baud=115200 ;
struct termios oldSerialState;

int init_crypto( const char *deviceName, int baud ) {
   	fdSerial = open(deviceName, O_RDWR);
    if (0 > fdSerial) {
		printf( "open serial: %s error !\n", deviceName );
		return -1;
	}
	fcntl(fdSerial, F_SETFD, FD_CLOEXEC);
	fcntl(fdSerial, F_SETFL, O_NONBLOCK);
	if( setRaw(fdSerial, baud, databits, parity, stopBits, oldSerialState) < 0 ) {
		close(fdSerial) ;
		fdSerial = -1 ;
		return -2 ;
	}
	return 0 ;
}

void uninit_crypto() {
	if( fdSerial>0 ) {
		tcsetattr(fdSerial, TCSANOW, &oldSerialState);
		close(fdSerial);
		fdSerial = -1 ;		
	}
}

static int comm_send( string &encryptedText ) {
	int rw_num ;
	int total_len = encryptedText.length() ;
	
	write( fdSerial, &total_len, 2 ) ;
	total_len = 0 ;
	while( total_len < encryptedText.length() ) {
		rw_num = write( fdSerial, encryptedText.c_str() + total_len, encryptedText.length() - total_len ) ;
		if( rw_num < 0 ) {
			printf( "COMM error: %d !\n", rw_num ) ;
			return -1 ;
		}
		total_len += rw_num ;
	}
	return 0 ;	
}

static int comm_recv( char *buf, int wait_ms ) {
	int total_len = 0 ;
	int retry = 0 ;
	int buf_len ;
	int rw_num ;
	pollfd fds[1];
	
	fds[0].fd = fdSerial ;
	fds[0].events = POLLIN | POLLERR ;	

	while( retry<(wait_ms/100)+1 ) {
		::poll(fds, 1, 100);
		if (fds[0].revents & POLLIN) {
			retry = 0 ;
			buf_len = 0 ;
			while( buf_len < 2 ) {
				rw_num = read( fdSerial, buf+buf_len, 2-buf_len ) ; 
				if( rw_num < 0 ) {
					printf( "recv header error : %d !\n", rw_num ) ;
					return -3 ;
				}
				buf_len += rw_num ;
				::poll(fds, 1, 100 ) ;
			}
			memcpy( &total_len, buf, 2 ) ;
			printf( "total len: %d\n", total_len ) ;
			buf_len = 0 ;
			while( buf_len<total_len && retry<wait_ms/100+1) {
				::poll(fds, 1, 100 ) ;
				if( fds[0].revents & POLLIN ) {
					rw_num = read( fdSerial, buf+buf_len, total_len-buf_len) ;
					if( rw_num<0 ) {
						printf( "recv error: %d !\n", rw_num ) ;
						return -1 ;
					}
					//printf( "buf len: %d, rw num: %d\n", buf_len, rw_num ) ;
					buf_len += rw_num ;
				}
				else {
					retry ++ ;
				}
			}
			//printf( "buf len: %d, rw num: %d\n", buf_len, rw_num ) ;
			buf[buf_len] = 0 ;
			return buf_len ;
		}
		retry++ ;				
	}
	//printf( "recv timeout !\n") ;
	return -2 ;
}

static void transform( char *buf ) {
	char *pCh = buf ;
	while( *pCh ) {
		*pCh = 'A' + (((*pCh ^ 'P') + 32) % 26) ;
		pCh++ ;
	}
	//printf( "transform: %s\n", buf ) ;
}


#ifdef MASTER

int check_crypto( const char * msg ){
	int total_len  ;
	int rw_num  ;
	int retry = 0 ;
	char buf[2048] ;
	int buf_len ;

	//printf( " encrypt msg: %s\n", msg ) ;
	string encryptedText = RSAEncryptString(pubKey, seed, msg );
	//printf( "encrypt: %s, len:%d\n", encryptedText.c_str(), encryptedText.length() ) ;
	tcflush(fdSerial, TCIFLUSH);	
	if( comm_send( encryptedText) == 0 ) {
		buf_len = comm_recv( buf, 3000 ) ;
		printf( "Comm recv: %d\n", buf_len ) ;
		if( buf_len <= 0 ) {
			printf( "Comm recv error : %d\n", buf_len ) ;
			return 0 ;
		}
		else {
			string decryptedText = RSADecryptString(priKey, buf );
			//printf( "dencrypt msg : %s\n", decryptedText.c_str() ) ;

			char trans_buf[1024] ;
			strcpy( trans_buf, msg ) ;
			transform( trans_buf ) ;
			return strcmp( trans_buf, decryptedText.c_str() )==0 ? 1 : 0 ;
		}
	}
	return 0 ;
}

#if 0
int main(int argc, char *argv[] ) {
	init_crypto( "/dev/ttyUSB0", baud ) ;
	if( argc>1 )
		printf( "check crypt: %d\n", check_crypto( argv[1] ) );
	else printf( "check crypt: %d\n", check_crypto( "this is a test string !" ) );
	uninit_crypto() ;
}
#endif

#else

static int fdWDT = -1 ;
#include <linux/watchdog.h>

void init_wdt(int timeout_sec) {
    fdWDT = open("/dev/watchdog", O_WRONLY);
	if (fdWDT == -1) {
		printf("open watchdog failure !\n");
		return ;
	}
	if( timeout_sec<5 ) 
		timeout_sec = 5 ;
    printf( "watchdog set timeout: %d , result: %d\n", timeout_sec, 
    	ioctl(fdWDT, WDIOC_SETTIMEOUT, &timeout_sec) ) ;
}

int arm_check() {
	pollfd fds[1];
	int total_len  ;
	int rw_num  ;
	int retry = 0 ;
	char buf[2048] ;
	int buf_len ;

	while( 1 ) {
		buf_len = comm_recv( buf, 1000 ) ;
		if( buf_len == -1 ) {
			uninit_crypto() ;
			init_crypto("/dev/ttySAC2", baud) ;
		} else if( buf_len>0 ) {
			printf( "recv len: %d, msg: %s\n", buf_len, buf ) ;
			string decryptedText = RSADecryptString(priKey, buf );
			printf( "dencrypt msg : %s\n", decryptedText.c_str() ) ;
			strcpy( buf, decryptedText.c_str() ) ;
			buf[ decryptedText.length() ] = 0 ;
			transform( buf ) ;
			decryptedText = RSAEncryptString(pubKey, seed, buf );
			printf( "send msg: %s, length: %d\n", decryptedText.c_str(), decryptedText.length() ) ;
			comm_send( decryptedText ) ;
		}
		if( fdWDT>0 ) {
			printf("tick watchdog result: %d !\n",
				ioctl(fdWDT, WDIOC_KEEPALIVE, 0) ) ;
		}
	}
}

/**/
int main( )
{
	init_crypto( "/dev/ttySAC2", baud ) ;
	init_wdt(10) ;
	arm_check() ;
	uninit_crypto() ;
	close(fdWDT) ;
	return 0 ;
}
/**/

#endif
