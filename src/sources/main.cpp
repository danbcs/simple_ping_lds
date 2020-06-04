#include "../headers/main.hpp"

int main(int argc, char *argv[]) {
    
}


// model checksun for implement
//
//  () Checksum Method
//
int32_t checksum(uint16_t *buf, int32_t len)
{
    //
    //  1. Variable needed for the calculation
    //
    int32_t nleft = len;    // Save how big is the header
    int32_t sum = 0;        // Container for the calculated value
    uint16_t *w = buf;      // Save the first 2 bytes of the header
    uint16_t answer = 0;    // The state of our final answer

    //
    //  2. Sum every other byte from the header
    //
    while(nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    //
    //  3. Handle odd headers
    //
    if(nleft == 1)
    {
        sum += *(uint8_t *)w;
    }

    //
    //  4. Needed conversions
    //
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    //
    //  5. Invert the bits
    //
    answer = ~sum;

    //
    //  -> Result
    //
    return answer;
}
