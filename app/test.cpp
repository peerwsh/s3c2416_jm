#include <stdio.h>

int init_crypto( const char *deviceName, int baud ) ;
void uninit_crypto() ;
int check_crypto( const char * msg ) ;

int main(int argc, const char *argv[] ) {
	int baud = 115200 ;
	init_crypto( "/dev/ttyUSB0", baud ) ;
	if( argc>1 )
		printf( "check crypt: %d\n", check_crypto( argv[1] ) );
	else printf( "check crypt: %d\n", check_crypto( "this is a test string !" ) );
	uninit_crypto() ;
}
