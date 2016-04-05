/*
	bptunnel.c:	ip over bp tunnel client and server.
									*/
/*									*/
/*	Copyright (c) 2015, BeiJingJiaoTong University.		*/
/*	All rights reserved.						*/
/*	Author: Haifeng Lee Haifeng233@gmail.com		*/
/*									*/
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <bpP.h>
//#include <pcap.h>
#include <pthread.h>
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_ICMPV6, INET6_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <netinet/ip6.h>      // struct ip6_hdr
#include <netinet/tcp.h>
#include <netinet/icmp6.h>    // struct icmp6_hdr and ICMP6_ECHO_REQUEST
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
//#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <sys/time.h>         // gettimeofday()
#include <inttypes.h> // uint8_t 
#include <limits.h> // UINT_MAX 
#include <stdbool.h>
#include <time.h> // clock_gettime() 
#include <poll.h> // poll()
#include <sys/uio.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h> // getifaddrs and freeifaddrs
#include <errno.h>            // errno, perror()
#include <sys/times.h> //times() fallback 
#include <net/route.h>
#include <sys/types.h>
#include <sys/socket.h>


#if defined (CLOCK_HIGHRES) && !defined (CLOCK_MONOTONIC)
# define CLOCK_MONOTONIC CLOCK_HIGHRES
#endif

// Define some constants.
#define ETH_HDRLEN 14  // ethernet header length
#define IP6_HDRLEN 40  // IPv6 header length
#define TCP_HDRLEN 20  // TCP header length, excludes options data


/* Version number of package */
#define BP_TUNNEL_VERSION "bptunnel for IPv4 20151125"

#define clock_nanosleep( c, f, d, r ) nanosleep( d, r )

//unsigned char srcip[4];
//unsigned char dstip[4];
//static char	dstEid2[] = "ipn:2.2";
//static char	dstEid3[] = "ipn:3.2";
//static char	dstEid4[] = "ipn:4.2";

typedef struct{
	BpSAP	sap;
	int	running;
} BptestState;

typedef struct {
	int tun_fd;
	char *DTNEth;
	char *ownEid;
    char *dstEid;
    char *dstEid2;
    char *dstEid3;
    char *dstEid4;

	BpSAP *sap;
	Sdr sdr;
} BpArg;


/**
 *  激活接口
 */
int interface_up(char *interface_name)
{
    int s;

    if((s = socket(PF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("Error create socket \n");
        return -1;
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name,interface_name);

    short flag;
    flag = IFF_UP;
    if(ioctl(s, SIOCGIFFLAGS, &ifr) < 0)
    {
        printf("Error up %s \n",interface_name);
        return -1;
    }

    ifr.ifr_ifru.ifru_flags |= flag;

    if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
    {
        printf("Error up %s \n",interface_name);
        return -1;
    }

    return 0;

}

/**
 *  设置接口ip地址
 */
 /*
int set_ipaddr(char *interface_name, char *ip)
{
    int s;

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error up %s \n",interface_name);
        return -1;
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface_name);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    inet_aton(ip, &addr.sin_addr);

    memcpy(&ifr.ifr_ifru.ifru_addr, &addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFADDR, &ifr) < 0)
    {
        printf("Error set %s ip\n",interface_name);
        return -1;
    }

    return 0;
}
*/
/**
 *  创建接口
 */
int tun_create(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        printf("Error\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags |= flags;

    if (*dev != 0)
    {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) 
    {
        printf("Error\n");
        close(fd);
        return -1;
    }
    strcpy(dev, ifr.ifr_name);

    return fd;
}

static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit()
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static int tunnel_bpsend( BpSAP sap, Sdr sdr, char *dstEid, const u_char *buf,long len)
{
	int 		ttl = 300;
	Object		extent;
	Object		bundleZco;
	Object		newBundle;

	if (buf)
	{
		if (len == 0)
		{
			writeMemo("[?] length of packet sent by bp tunnel is 0.");
			bp_close(sap);
			return 0;
		}

		CHKZERO(sdr_begin_xn(sdr));
		extent = sdr_malloc(sdr, len);
		if (extent)
		{
			sdr_write(sdr, extent,(char *) buf, len);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("No space for ZCO extent.", NULL);
			bp_close(sap);
			return 0;
		}

		bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, len,
				BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
		if (bundleZco == 0 || bundleZco == (Object) ERROR)
		{
			putErrmsg("Can't create ZCO extent.", NULL);
			bp_close(sap);
			return 0;
		}
		if (bp_send(sap, dstEid, NULL, ttl, BP_STD_PRIORITY,
				NoCustodyRequested, 0, 0, NULL, bundleZco,
				&newBundle) < 1)
		{
			putErrmsg("bptunnel can't send ADU.", NULL);
		}

		return 0;
	}
	return -1;
}

static void *ipOverDtn(void *arg){
	unsigned char buf[4096];
	int ret;
	BpArg *bpArg = (BpArg *)arg;
	BpSAP sap = *(bpArg->sap);
	Sdr sdr = bpArg->sdr;
	//char *dstEid = bpArg->dstEid;
	char *dstEid = NULL;
	int tun = bpArg->tun_fd;	
	struct ip* ip;
	struct tcphdr *tcp;
	unsigned short LOCALPORT;
	char *tempip;
	
	while(1)
	{	
		//bzero(tempip, sizeof(tempip));
		ret = read(tun, buf, sizeof(buf));
		if(ret<0)
			continue;
		printf("read %d bytes\n", ret);
		ip = (struct ip*)buf;
		printf("src overdtn address is %s\n",inet_ntoa(ip->ip_src));
		
		printf("dest overdtn address is %s\n",inet_ntoa(ip->ip_src));

		tempip = inet_ntoa(ip->ip_dst);

		if(strcmp(tempip,"192.168.99.22") == 0)
			dstEid = bpArg->dstEid; //-t
		
		if(strcmp(tempip,"192.168.99.23") == 0)
			dstEid = bpArg->dstEid2; //-y

		if(strcmp(tempip,"192.168.99.24") == 0)	
			dstEid = bpArg->dstEid3; //-u
		
		if(strcmp(tempip,"192.168.99.29") == 0)	
			dstEid = bpArg->dstEid4; //-i

		tcp = (struct tcphdr *)(buf + sizeof(struct ip));
		LOCALPORT = ntohs(tcp->source);
		printf("ipOverDtn:286: LOCALPORT = %d\n",LOCALPORT);
		tunnel_bpsend(sap, sdr, dstEid, buf,ret); 

		printf("OverDtn\n");

	}
	return 0;
}

int send2DataLink(const char *packet, long pktLen)
{
	struct sockaddr_in client_addr;
	int sd,bytes;
	struct ip* ip;
	struct tcphdr *tcp;
	int on = 1;
	
	// Submit request for a raw socket descriptor.
	if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
		putErrmsg ("send2DataLink socket() failed ", NULL);
		exit (EXIT_FAILURE);
	}
	setsockopt(sd,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on));

	ip = (struct ip*)packet;
	printf("send2DataLink:310: src address is %s\n",inet_ntoa(ip->ip_src));

	printf("send2DataLink:312: dest address is %s\n",inet_ntoa(ip->ip_dst));	
	
	
	tcp = (struct tcphdr *)(packet + sizeof(struct ip));

	memset(&client_addr,0,sizeof(struct sockaddr_in));	
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = (tcp->source); //这里是否需要改成目的端口?虽然源端口也可以通，未试过目的端口
										  //尝试过注释该行，也可以通!!!!!!!!!!!
	client_addr.sin_addr = ip->ip_dst;

	// Send ethernet frame to socket.
	if ((bytes = sendto (sd, packet, pktLen, 0, (struct sockaddr*)&client_addr, sizeof(struct sockaddr_in))) <= 0) {
		putErrmsg ("send2DataLink sendto() failed", NULL);
		exit (EXIT_FAILURE);
	}

	 // Close socket descriptor.
	close (sd);
 
	return 0;
}

static void *ipOutDtn(void *arg){

/*	static char	*deliveryTypes[] =	{
				"Payload delivered.",
				"Reception timed out.",
				"Reception interrupted.",
				"Endpoint stopped."
						};*/
	long		pktLen;
	int		len;
	char		packet[2000];
	char		line[2000];
	BptestState	state = { NULL, 1 };
	BpDelivery	dlv;
	ZcoReader	reader;
	BpArg *bpArg = (BpArg *)arg;
	state.sap = *(bpArg->sap);
	Sdr sdr = bpArg->sdr;


#ifndef mingw
	setlinebuf(stdout);
#endif

	oK(_bptestState(&state));
	while (state.running)
	{

	isignal(SIGINT, handleQuit);
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpsink bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		//PUTMEMO("ION event", deliveryTypes[dlv.result - 1]);
		if (dlv.result == BpReceptionInterrupted)
		{
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			state.running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			CHKZERO(sdr_begin_xn(sdr));
			pktLen = zco_source_data_length(sdr, dlv.adu);
			sdr_exit_xn(sdr);
			isprintf(line, sizeof line, "\tpayload length is %d.",
					pktLen);
		//	PUTS(line);
			if (pktLen < sizeof packet)
			{
				zco_start_receiving(dlv.adu, &reader);
				CHKZERO(sdr_begin_xn(sdr));
				len = zco_receive_source(sdr, &reader, pktLen, (char *)packet);
				if (sdr_end_xn(sdr) < 0 || len < 0)
				{
					putErrmsg("Can't handle delivery.",
							NULL);
					state.running = 0;
					continue;
				}

				printf("outDtn is going\n");

				send2DataLink(packet, pktLen);
			}
		}

		bp_release_delivery(&dlv, 1);
printf("outDtn\n");
	}
return 0;
}

static int
version(void)
{
	printf("bptunnel: IPv4 in Bundle tunnel transfer tool %s (%s)\n",
            BP_TUNNEL_VERSION, "$Rev$");

	printf("Written by Haifeng Lee\n");

	printf("Copyright (C) %u-%u Haifeng Lee & Xu Qi\n", 2014, 2015);
	printf("This is free software; see the source for copying conditions.\n"
	        "There is NO warranty; not even for MERCHANTABILITY or\n"
	        "FITNESS FOR A PARTICULAR PURPOSE.\n");
	return 0;
}

static void usage(const char *name)
{
	fprintf(stderr,
		"Usage V%s: %s [-q] [-d] [-s OWN-SEND-EID] [-r OWN-REC-EID]\n"
	        "	[-p IP-INTERFACE] [-b DTN-INTERFACE]\n" 
			"	[-g GATEWAY] [-f MAPTABLE-FILE]\n"
		"\n"
		"IP over BP PROTO tunnel client and server, surpport IPv4 and IPv6.\n"
		"The program can let traditional network go through the DTN network\n"
                " with bp tunnel.\n"
		"\n"
		"Options:\n"
		"  -s OWN-SEND-EID       the source EID to send\n"
		"  -r OWN-REC-EID        the source EID to receive\n"		
		"  -t DEST-REC-EID		 the destOVS EID1 to receive\n"		
		"  -y DEST-REC-EID		 the destOVS EID2 to receive\n"		
		"  -u DEST-REC-EID		 the destOVS EID3 to receive\n"		
		"  -i DEST-REC-EID		 the destOVS EID4 to receive\n",
		BP_TUNNEL_VERSION, name);
}


#if defined (ION_LWT)
int	bptunnel(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ownEid = (char *) a1;
	char	*dstEid = (char *) a2;
	char	*fileName = (char *) a3;
	char	*classOfService = (char *) a4;
#else
int	main(int argc, char **argv)
{
	char	DTNEth[10] = "";
	char	 ownSendEid[20] = "";
	char 	ownRecEid[20] = "";
	char 	dstEid[20] = "";	
	char 	dstEid2[20] = "";
	char 	dstEid3[20] = "";	
	char 	dstEid4[20] = "";
	int 	opt;
	Sdr		sendSdr;  //ptr
	Sdr		recSdr;  //ptr
	BpSAP	sendSap;   //value
	BpSAP	recSap;   //value
	BpArg *bpSendArg = NULL;
	BpArg *bpRecArg = NULL;
	pid_t fpid;

    int tun; 
    char tun_name[IFNAMSIZ];
    tun_name[0] = 0;
    tun = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
    if (tun < 0) 
    {
	printf("tun_create faild\n");
        return 1;
    }
	printf("TUN name is %s\n",tun_name);
    interface_up(tun_name);


	while ((opt = getopt(argc, argv, ":hqdv:b:s:t:r:y:u:i:")) >= 0)
	{
		switch (opt)
		{
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 'q':
			//flags &= ~OUTPUT_TRACE_MSG;
			break;

		case 'd':
			//flags |= RUNNING_AS_DAEMON;
			break;

		case 'b':
			strcpy(DTNEth, optarg);
			break;

		case 's':
			strcpy(ownSendEid, optarg);
			break;

		case 'r':
			strcpy(ownRecEid, optarg);
			break;

		case 't':
			strcpy(dstEid, optarg);
			break;
		case 'y':
			strcpy(dstEid2, optarg);
			break;
		case 'u':
			strcpy(dstEid3, optarg);
			break;
		case 'i':
			strcpy(dstEid4, optarg);
			break;
		
		case 'v':
			return version();
			
		case '?':
			fprintf(stderr, "unknown option '-%c'\n", optopt);
			break;
	        default:
			break;
		}
	}


	if( !ownRecEid[0] || !ownSendEid[0] || !dstEid[0]||!dstEid2[0]||!dstEid3[0]||!dstEid4[0] ){
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}
	printf("ownRecEid:%s\nownSendEid:%s\ndstEid:%s\n", ownRecEid, ownSendEid, dstEid);
#endif

//return 0;
	fpid = fork();
	//fpid = 0;
	if(fpid < 0){
		printf("error in fork,%d\n", (int)fpid);
		putErrmsg("error in fork", (char *)&fpid);
		return -1;
	}else if(fpid == 0){
		if (bp_attach() < 0)
		{
			putErrmsg("Can't attach to BP to send.", NULL);
			return 0;
		}
		
		printf("bp_attach done (send)\n");
		if (bp_open(ownSendEid, &sendSap) < 0)
		{
			putErrmsg("Can't open own endpoint(send).", ownSendEid);
			return 0;
		}
		printf("bp_open done (send)\n");
		
	       sendSdr = bp_get_sdr();
		bpSendArg = (BpArg *)malloc(sizeof(BpArg));
		if(bpSendArg == NULL) {
			putErrmsg("Can't malloc bpSendArg!.",(char *)bpSendArg);
			return -1;
		}
		
		bpSendArg->tun_fd=tun;
		bpSendArg->DTNEth = DTNEth;
		bpSendArg->ownEid = ownSendEid;
		bpSendArg->dstEid = dstEid;
		bpSendArg->dstEid2 = dstEid2;
		bpSendArg->dstEid3 = dstEid3;
		bpSendArg->dstEid4 = dstEid4;
		bpSendArg->sap = &sendSap;
		bpSendArg->sdr = sendSdr;

		isignal(SIGINT, handleQuit);
		ipOverDtn((void *)bpSendArg);

		bp_close(sendSap);
		writeErrmsgMemos();
		bp_detach();
		return 0;
	}else{
		if (bp_attach() < 0)
		{
			putErrmsg("Can't attach to BP to receive.", NULL);
			return 0;
		}
		
		printf("bp_attach done (receive)\n");
		if (bp_open(ownRecEid, &recSap) < 0)
		{
			putErrmsg("Can't open own endpoint to receive.", ownRecEid);
			return 0;
		}
		printf("bp_open done (receive)\n");
		
	        recSdr = bp_get_sdr();
		bpRecArg = (BpArg *)malloc(sizeof(BpArg));
		if(bpRecArg == NULL) {
			putErrmsg("Can't malloc bpRecArg!.",(char *)bpRecArg);
			return -1;
		}
	
		bpRecArg->tun_fd = tun;
		bpRecArg->DTNEth = DTNEth;
		bpRecArg->ownEid = ownRecEid;
		bpRecArg->dstEid = dstEid;
		bpRecArg->dstEid2 = dstEid2;
		bpRecArg->dstEid3 = dstEid3;
		bpRecArg->dstEid4 = dstEid4;
		bpRecArg->sap = &recSap;
		bpRecArg->sdr = recSdr;
		
		isignal(SIGINT, handleQuit);
		ipOutDtn((void *)bpRecArg);
		
		bp_close(recSap);
		writeErrmsgMemos();
		bp_detach();
		return 0;
	}
}
