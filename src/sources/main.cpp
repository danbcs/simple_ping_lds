#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <iomanip>
#include <memory>

#define PACKET_SIZE 1024        /* Max size for packet */
#define PORT_NO 0
#define TIMEOUT_US  1000000
#define CHRONO_TYPE std::chrono::time_point<std::chrono::high_resolution_clock>

#define ICMP_ECHOREPLY	0	/* Echo Reply	*/
#define ICMP_ECHO		8	/* Echo Request	*/

pid_t pid;             // process id type
int data_length = 56;  // well-formed ping packet size
int sock;
int nsent=0, nreceived=0;
bool debug = false;
sockaddr_in outgoing_addr;
sockaddr_in incoming_addr;
u_char out_buffer[PACKET_SIZE];
u_char in_buffer[PACKET_SIZE];
CHRONO_TYPE t_start, t_end;

void ping();
void receive();
void statistics(int signal);
u_short checksum(u_short *b, int len);

int main(int argc, char *argv[]) {
    try {
        // Check args
        if (argc < 2) {  // missing argument
            throw std::runtime_error("[ERROR] Missing ip address as positional terminal argument.");
        }
        
        // Initialize protocol
        std::unique_ptr<protoent> protocol(new protoent());
        //auto protocol = std::shared_ptr<protoent>(getprotobyname("icmp"));

        protocol->p_proto = getprotobyname("icmp")->p_proto;
        if (!protocol) throw std::runtime_error("[ERROR] getprotobyname error.");

        // Instantiate socket
        sock = socket(PF_INET, SOCK_RAW, protocol->p_proto /* ICMP protocol */);
        if (sock <= 0) throw std::runtime_error("[ERROR] socket error.");

        // Socket options
        const int val = 255;
        
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val));  // set buffer size for input, size only for tcp
        bzero(&outgoing_addr, sizeof(outgoing_addr));
        outgoing_addr.sin_family = AF_INET;
        outgoing_addr.sin_port = htons(PORT_NO);

        
        u_int inaddr = inet_addr(argv[1]);
        /* 
            Initialize hostname:
            ex: google.co -> 172.217.162.174 
        */
        if (inaddr == INADDR_NONE) {
            
            if (gethostbyname(argv[1]) == nullptr) {
                  std::cerr << "[ERROR] gethostbyname error." << std::endl;
                  exit(1);
            }
            // GTest
            memcpy((char *)&outgoing_addr.sin_addr, gethostbyname(argv[1])->h_addr, gethostbyname(argv[1])->h_length);  // copy hostname addr to packet
        } else {
            outgoing_addr.sin_addr.s_addr = inet_addr(argv[1]);
        }

        // Configure timeout recvfrom - receive()
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT_US; //500ms
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

        // Ping
        std::cout << "PING " << argv[1] << "(" << inet_ntoa(outgoing_addr.sin_addr) << "): " << data_length << " data bytes\n";
        //call statistics() when ctrl+C
        signal(SIGINT, statistics);

        while (1) {
            pid = getpid();  // unique process id
            ping();
            receive();
            sleep(1);       // 1 ping per seconds
        }
        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

// Generate checksum
u_short checksum(u_short *b, int len) {
    int sum = 0;
    u_short result = 0;
    u_short *buffer = b;

    // Adding 16 bit segments of the packet (len is in number of bytes)
    for (; len > 1; len -= 2) {
        sum += *buffer++;
    }
    // For odd numbers
    if (len == 1) {
        sum += *(u_char *)buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);  // Carries
    sum += (sum >> 16);
    result = ~sum;  // 1's complement
    return result;
}

/*
    Convert uint16_t in uint8_t when:
    pos -> true:    first byte 
    pos -> false:   second byte
*/
uint8_t conv2bytesfor1(uint16_t valor, bool pos) {
    if (pos) 
        return (uint8_t) (valor & 0xFF);
    else 
        return (uint8_t) ((valor >> 8) & 0xFF);
}

void ping() {
    try {
        // Pack data
        int packet_size;
        //icmp *packet;
        u_short result;

        // mount header package of msg
        out_buffer[0] = ICMP_ECHO;  // type of msg
        out_buffer[1] = 0;          // sub code
        out_buffer[2] = 0;          // checksum: init with 0
        out_buffer[3] = 0;          // checksum: init with 0
        out_buffer[4] = conv2bytesfor1(pid, true);          // id type first byte
        out_buffer[5] = conv2bytesfor1(pid, false);         // id type second byte
        out_buffer[6] = conv2bytesfor1(nsent, true);        // num sends first byte
        out_buffer[7] = conv2bytesfor1(nsent, false);       // num sends second byte
        packet_size = 8 + data_length; // skip ICMP header. size: 56 + 8

        
        result = checksum((u_short *) out_buffer, packet_size);
        
        //subst value of checksum
        out_buffer[2] = (result & 0xFF);                 
        out_buffer[3] = ((result >> 8) & 0xFF);   

        t_start = std::chrono::high_resolution_clock::now();
        int send_result = sendto(sock, out_buffer, packet_size, 0, (struct sockaddr *)&outgoing_addr, sizeof(outgoing_addr));
        if (send_result <= 0) throw std::runtime_error("[ERROR] Sendto error.");
        nsent++;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void receive() {
    int len;
    unsigned int incoming_len = sizeof(incoming_addr);

        // Try to receive and unpack packet from host
        len = recvfrom(sock, in_buffer, sizeof(in_buffer), 0, (struct sockaddr *)&incoming_addr, &incoming_len);
        if (len != -1) {
            // Non-zero length

            int iphdrlen;  // ip header length

            // ip pointer cast on buffer
            iphdrlen = (uint32_t) in_buffer[0] & 0xF;
            iphdrlen = iphdrlen << 2;

            len -= iphdrlen; // length of packet without header

            // Packet for verif checksum        *(get better)*
            u_char packet_checksum[PACKET_SIZE];
            int packet_size = 8 + data_length; // skip ICMP header. size: 56 + 8
            for(int i = 0; i<packet_size; i++) 
                packet_checksum[i] = in_buffer[iphdrlen+i];
            
            packet_checksum[2] = 0;
            packet_checksum[3] = 0;

            //checksum for comparison
            ushort result = checksum((u_short *) packet_checksum, packet_size);

            //checksum from in_buffer
            ushort rec_checksum = ((ushort) in_buffer[iphdrlen+3]<<8)+(uint8_t) in_buffer[iphdrlen+2];

            if (len > 8 && result == rec_checksum && in_buffer[iphdrlen] == ICMP_ECHOREPLY) {
                nreceived++;
                // Only reach this point if the packet is complete
                t_end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> duration = (t_end - t_start)*1000; // Conversion to ms
                auto rtt = duration.count();
                //int ttl = ip_addr->ip_ttl;
                int ttl = (uint32_t) in_buffer[8];

                std::cout
                    << len << " bytes from " << inet_ntoa(incoming_addr.sin_addr)
                    << ": icmp_seq=" << (uint32_t) (in_buffer[iphdrlen +6]+1)
                    << ",\tttl=" << std::fixed << (ttl)
                    << ", time=" << std::fixed << std::setprecision(2) << rtt << " ms,\t"
                    << "packets out=" << nsent << ",\t"
                    << "packets in=" << nreceived << ",\t"
                    << std::fixed << std::setprecision(2) << ((float)(nsent - nreceived) / nsent * 100) << "% packet loss\n";
            } 
        }
}

// Display ping statistics upon interrupt
void statistics(int signal) {
    std::cout << "\n-------- PING STATISTICS --------\n";
    std::cout << nsent << " total packets transmitted, "
    << nreceived << " total packets received, " 
    << std::setprecision(3) << ((float)(nsent - nreceived) / nsent * 100) << "% packet loss\n";
    exit(0);
}
