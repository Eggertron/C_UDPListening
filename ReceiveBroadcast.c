#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
//#include <net/if.h>
#include <linux/if.h>	// <replaces net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
// for file reading and writing
#include <stdio.h>
#include <stdlib.h>


#define MYPORT 9050     // the port users will be connecting to
#define MAXBUFLEN 512
#define CONFIGFILENAME "config.txt"
#define JSONFILENAME "config.json"

void diep(char *s);

int writeConfigFile(char *buff);

int createConfigFile ();

int readConfigFile();

int getMacAddy(char *buff);

int sendMsg(struct sockaddr_in si_other, char buf[]);

int getLocX();

int getLocY();

int getLocZ();

char * getFriendlyName();

int setLocX(int value);

int setLocY(int value);

int setLocZ(int value);

int setFriendlyName(char * buff);

char * gen_JSON_format();

char * listening();

/* Return the offset of the first newline in text or the length of
   text if there's no newline */
static int newline_offset(const char *text);

// global variables, location of device.
int locX=1, locY=2, locZ=3;
// global variable, friendly name.
char * friendlyName="pineapple";
char * currentMAC="00:00:00:00:00:00";

// ================================== MAIN =============
int main(void)
{
	readConfigFile();
	//createConfigFile();
	char * buff;
	
	while( 1 )
	{
		buff = listening();
	}
}
// ==================================END MAIN ============

// Generate a config.json file
int gen_JSON_file()
{
	unsigned char buf[MAXBUFLEN];
	sprintf(buf, "{ \"name\": \"%s\", \"mac\": \"%s\", \"locX\": \"%d\", \"locY\": \"%d\", \"locZ\": \"%d\"}\n", getFriendlyName(), currentMAC, getLocX(), getLocY(), getLocZ());
	
	FILE *fp = fopen(JSONFILENAME, "wb");
	fprintf(fp, "%s", buf);
	fclose(fp);
	return 0;
}

// Generate and return a string in JSON formatting
char * gen_JSON_format()
{
	unsigned char buf[MAXBUFLEN];
	sprintf(buf, "{ \"name\": \"%s\", \"mac\": \"%s\", \"locX\": \"%d\", \"locY\": \"%d\", \"locZ\": \"%d\"}\n", getFriendlyName(), currentMAC, getLocX(), getLocY(), getLocZ());
	return buf;
}

// Produce error message and exit program
void diep(char *s)
{
	// if error, quit with error.
    perror(s);
    exit(1);
}

// writes the string to the output file.
int writeConfigFile(char *buff)
{
	FILE *fp;
	fp=fopen(CONFIGFILENAME, "wb");
	fprintf(fp, "%s", buff);
	
	fclose(fp);
	return 0;
}

// writes a csv containing friendlyname, MAC, X, Y, Z
int createConfigFile ()
{
	char temp [MAXBUFLEN];
	sprintf(temp, "%s, %s, %d, %d, %d", getFriendlyName(), currentMAC, getLocX(), getLocY(), getLocZ());
	writeConfigFile ( temp );
}

// read and parse csv, if file does not exist, create a default.
int readConfigFile()
{
	char file_name[] = CONFIGFILENAME;
	char str[MAXBUFLEN];
    FILE *fp;
 
    fp = fopen(file_name,"r"); // read mode
 
    if( fp == NULL )
	{
        //diep("Error while opening the file.\n");
		createConfigFile();
		readConfigFile();
		return 0;
	}
	else
	{
		if( fgets( str, MAXBUFLEN, fp) == NULL )
			diep("File Empty??");
		fprintf(stdout, "%s", str);
		
		// deliminate the file
		setFriendlyName( strtok(str, ",") );
		currentMAC		= strtok(NULL, ",");
		setLocX( atoi(strtok(NULL, ",")) );
		setLocY( atoi(strtok(NULL, ",")) );
		setLocZ( atoi(strtok(NULL, ",")) );
		
		fclose(fp);
		return 0;
	}
}

// Queries local machine for MAC address and saves to string buff
int getMacAddy(char *buff)
{
	int fd;
    struct ifreq ifr;
    char *iface = "eth0";
    unsigned char *mac;
     
    fd = socket(AF_INET, SOCK_DGRAM, 0);
 
    ifr.ifr_addr.sa_family = AF_INET;		// AF_INET addresses from the internet (IP address)
    strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
 
    ioctl(fd, SIOCGIFHWADDR, &ifr);			// input-output control
 
    close(fd);
     
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
     
    //display mac address in xx:xx:xx:xx:xx format
    sprintf(buff, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
 
    return 0;
}

// sends a message to a specified address, with string buf.
int sendMsg(struct sockaddr_in si_other, char buf[])
{
	//struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");
	

    //memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(MYPORT);	// convert host byte order to network byte order.
	/*
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        diep("inet_aton()");
    }
	*/
	
	printf("Send packets back to %s\n", inet_ntoa(si_other.sin_addr)); // convert ip to binary
    if (sendto(s, buf, strlen(buf), 0, &si_other, slen)==-1)
        diep("sendto()");

    close(s);
    return 0;
}

// listens to the network (udp) for broadcasting key words.
char * listening()
{
        int sockfd;
        struct sockaddr_in my_addr;     // my address information
        struct sockaddr_in their_addr; // connector's address information
		//memset((char *) &their_addr, 0, sizeof(their_addr));
        socklen_t addr_len;
        int numbytes;
        char buf[MAXBUFLEN];


        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                perror("socket");
                exit(1);
        }

        my_addr.sin_family = AF_INET;            // host byte order
        my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
        my_addr.sin_port = htons(MYPORT);        // short, network byte order
        memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

        if (bind(sockfd, (struct sockaddr *)&my_addr,
                sizeof(struct sockaddr)) == -1) {
                perror("bind");
                exit(1);
        }

        addr_len = sizeof(struct sockaddr);
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
        }

        printf("got packet from %s\n",inet_ntoa(their_addr.sin_addr));
        printf("packet is %d bytes long\n",numbytes);
        buf[numbytes] = '\0';
        printf("packet contains \"%s\"\n",buf);
		
		close(sockfd);
		
		// read the broadcast from any source and deliminate message
		char * part1 = strtok(buf, " ");
		char * part2 = strtok(NULL, " ");
		char * part3 = strtok(NULL, " ");
		unsigned char msg[38];
		
		if ( strcmp( part1, "hello") == 0 )
		{	// if message was simple Hello, respond with MAC
			// crate temp var to obtain Mac of this machine.
			unsigned char mac_address[38];
			getMacAddy ( &mac_address );
		
			// send a message with the MAC
			sendMsg ( their_addr, &mac_address );
		}
		else if ( (strcmp( part1, friendlyName) == 0) || (strcmp( part1, currentMAC) == 0) )
		{
			if ( strcmp( part2, "location") == 0)
			{
				//*msg = "current location";
				sprintf(msg, "Location [X, Y, Z] = [%d, %d, %d]\n", getLocX(), getLocY(), getLocZ());
				sendMsg( their_addr, msg);
			}
			else if ( strcmp( part2, "name") == 0)
			{
				sprintf(msg, "Friendly Name: %s\n", getFriendlyName());
				sendMsg( their_addr, msg);
			}
			else if ( strcmp( part2, "mac") == 0)
			{
				sprintf(msg, "Physical MAC: %s\n", currentMAC);
				sendMsg( their_addr, msg);
			}
			else if ( strcmp( part2, "setname") == 0)
			{
				// make sure this is a deep copy.
				//friendlyName = part3;
				setFriendlyName(part3);
				createConfigFile ();
				sendMsg( their_addr, "name changed.\n");
			}
			else if ( strcmp( part2, "setmac") == 0)
			{
				// make sure this is a deep copy.
				currentMAC = part3;
				createConfigFile ();
				sendMsg( their_addr, "mac has been manually changed.\n");
			}
			else if ( strcmp( part2, "setlocx") == 0)
			{
				//locX = atoi(part3);
				setLocX(atoi(part3));
				createConfigFile ();
				sendMsg( their_addr, "X coordinate has been set.\n");
			}
			else if ( strcmp( part2, "setlocy") == 0)
			{
				//locY = atoi(part3);
				setLocY(atoi(part3));
				createConfigFile ();
				sendMsg( their_addr, "Y coordinate has been set.\n");
			}
			else if ( strcmp( part2, "setlocz") == 0)
			{
				//locZ = atoi(part3);
				setLocZ(atoi(part3));
				createConfigFile ();
				sendMsg( their_addr, "Z coordinate has been set.\n");
			}
		}
		
		
		
		return &buf; // just put that there for now as place holder.
		
}
/*
/* Return the offset of the first newline in text or the length of
   text if there's no newline 
static int newline_offset(const char *text)
{
    const char *newline = strchr(text, '\n');
    if(!newline)
        return strlen(text);
    else
        return (int)(newline - text);
}*/

int getLocX()
{
	return locX;
}

int getLocY()
{
	return locY;
}

int getLocZ()
{
	return locZ;
}

char * getFriendlyName()
{
	return friendlyName;
}

int setLocX(int value)
{
	locX = value;
	return 0;
}

int setLocY(int value)
{
	locY = value;
	return 0;
}

int setLocZ(int value)
{
	locZ = value;
	return 0;
}

int setFriendlyName(char * buff)
{
	friendlyName = buff;
	if (friendlyName == NULL)
		return 1;
	else
		return 0;
}