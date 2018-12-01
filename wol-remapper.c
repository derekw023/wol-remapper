#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// Typedef for mac addresses
typedef struct {unsigned char mac[6];} macAddr;


// 6 bytes of header plus 16 repetitions of an IP
#define WOL_PKT_SIZE (6 + 16*6)

// Buffers to hold a mac address and a packet buffer
unsigned char sendbuf[WOL_PKT_SIZE];
macAddr mac;

// Filter address, this will cause a magic packet to be sent to to_wake
macAddr in_mac = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// Mac address list to wake (hardcoded since this is known at compile time)
macAddr to_wake[] = {{{0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0x55}},
					 {{0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA}}}; 

void fprintmac(FILE *stream,  macAddr macAddr){
			// DEBUG: print the mac address to wake
			printf("MAC: ");
			for (int i = 0; i < 6; i++) printf("%02X", mac.mac[i]);
			printf("\n");
}

// Buffer compare with hardcoded 6 byte length
int maccmp (unsigned const char *left, unsigned const char *right){
	for (int i = 0; i < 6; i++) {
		if (*left++ != *right++) return 1;
	}
	return 0;
}

int parse_wol(unsigned const char *recvd, int len,  macAddr *mac_to_wake) {
	
	// Search for magic packet content (6 FF's ina row)
	//
	// Validate all copies of mac address match
	//
	// Write mac address back and return zero 
	//
	// else return -1 if packet isnt magic
	//
	
	// Verify and then parse the WOL message format, 6 0xFF and then 16 repetitions of the same MAC address
	while (len) {
		if (*recvd == 0xFF) {
			int num_fs = 0;
			while (*recvd == 0xFF || num_fs < 6) {
				num_fs++;
				recvd++;
				len--;
			}
			// need 6 Fs or else break (to return -1)
			if (num_fs == 6) {
				// Read in the mac
				memcpy(mac_to_wake->mac, recvd, 6);
				len -= 6;
				recvd += 6;

				// Compare the stored mac with the other (supposed) 15 copies to validate
				// len was validated before, there is space for 15 more copies of the address
				while (len) {
					if (maccmp(mac_to_wake->mac, recvd)) return -1; 
					len -= 6;
					recvd += 6;
				}

				// Mac is written into mac_to_wake. Return 0
				return 0;

			// Number of FF's was not 6, not a magic packet. 
			} else break;
		} else {
			// Not 0xFF, advance buffer pointer and len counter
			recvd++;
			len--;
		}
	} // while (len)

	return -1;
}

void wake_wol (int sockfd, macAddr mac){
	unsigned char tosend[WOL_PKT_SIZE];

	// 6*0xFF start header
	for (int i = 0; i<6; i++) tosend[i] = 0xFF;

	// Copy the mac address 16 times
	for (int i = 0; i<16; i++) memcpy(&(tosend[6*i+6]), mac.mac, 6);

	// Build address
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof dest_addr);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(9);
	dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); // Broadcast to all

	// Sendto
	sendto(sockfd, tosend, WOL_PKT_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof dest_addr);

}
	

int main (int argc, char *argv[]){
	fprintf(stderr, "Starting wake-on-lan-listener\n");
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	int ret = 0;

	if (sockfd == -1){
		fprintf(stderr, "ERROR: Could not create socket, %m\n");
		return 1;
	}

	int broadcast = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);  
	
	if (ret) {
		fprintf(stderr, "ERROR: Could not set sockopts, %m\n");
		return 1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(9);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

	if (ret){
		fprintf(stderr, "ERROR: Could not bind socket, %m\n");
		return ret;
	}

	int len;
	while(1){
		// WAITALL to ensure the entire WOL message is received, this also constrains message size
		len = recv(sockfd, sendbuf, WOL_PKT_SIZE, MSG_WAITALL);
		if (len > 0) {
			// Check to see if the packet we got is magic, and parse the mac address out
			if (parse_wol(sendbuf, len, &mac)){
				 //fprintf(stderr, "Not a magic packet\n");
				 continue;
			}

			if (maccmp(in_mac.mac, mac.mac) == 0) {
				for (int i =0; i< (sizeof to_wake / sizeof *to_wake); i++) {
				wake_wol(sockfd, to_wake[i]);
				}
			}

		} // Else ignore the packet. Length here is verified to be exactly WOL_PKT_SIZE
	}

	return 0;
}


// vim: sw=4 ts=4 si 
