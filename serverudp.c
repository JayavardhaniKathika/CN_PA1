#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PORT_NO 8080
#define START_OF_PACKET_IDENTIFIER 0XFFFF
#define END_OF_PACKET_IDENTIFIER 0XFFFF

#define DATA 0XFFF1
#define ACK 0XFFF2
#define REJECT 0XFFF3
//reject sub codes
#define REJECT_OUT_OF_SEQUENCE 0XFFF4
#define REJECT_LENGTH_MISMATCH 0XFFF5
#define REJECT_END_OF_PACKET 0XFFF6
#define REJECT_DUPLICATE_PACKET 0XFFF7


#pragma pack(push,1)
    typedef struct dataPacket{
        uint16_t startPacketId; //2bytes
        uint8_t clientId;  //1 byte
        uint16_t data;   //2bytes
        uint8_t segmentno;  //1 byte
        uint8_t length;  //1byte
        char payload[255] ;
        uint16_t endPacketId; //2bytes
    }dataPacket;

    //ACK packet format
    typedef struct ackPacket{
        uint16_t startPacketId; //2bytes
        uint8_t clientId;  //1 byte
        uint16_t ack; //2bytes
        uint8_t receivedSegmentNo;  //1 byte
        uint16_t endPacketId; //2bytes
    }ackPacket;

    //Reject Packet Format
    typedef struct rejectPacket{
        uint16_t startPacketId; //2bytes
        uint8_t clientId;  //1 byte
        uint16_t reject; //2bytes
        uint16_t rejectSubCode; //2bytes
        uint8_t receivedSegmentNo;  //1 byte
        uint16_t endPacketId; //2bytes
    }rejectPacket;

#pragma pack(pop)

void printPacketDetails(dataPacket dp){
    printf("StartPacketId %x\n",dp.startPacketId);
    printf("ClientId %x\n",dp.clientId);
    printf("Data %x\n",dp.data);
    printf("SegmentNumber %u\n",dp.segmentno);
    printf("Length %x\n",dp.length);
    printf("Payload %s\n",dp.payload);
    printf("EndPacketId %x\n",dp.endPacketId);
    printf("---------------------------------------------\n");
}

//03 35 7f 84 f3 - f3847f3503
int main(void){

    
   
    
    ackPacket a1;
    a1.startPacketId=START_OF_PACKET_IDENTIFIER;
    a1.ack=ACK;
    a1.endPacketId=END_OF_PACKET_IDENTIFIER;

    rejectPacket r;
    r.startPacketId=START_OF_PACKET_IDENTIFIER;
    r.endPacketId=END_OF_PACKET_IDENTIFIER;
    r.reject=REJECT;

    dataPacket receivedPacket;


    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    //char server_message[2000], client_message[2000];
    int client_struct_length = sizeof(client_addr);
    
    
    // Create socket: int socket(int domain, int type, int protocol)
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NO);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    
    printf("Listening for incoming messages...\n\n");
    
   
   uint8_t previousSeqNo=0x00;
   uint8_t expectedSeqNo=0x01;
   int i=0;
    // Receive client's message:
    while(i<13){
        i++;
        if (recvfrom(socket_desc, &receivedPacket, sizeof(receivedPacket), 0,(struct sockaddr*)&client_addr, (socklen_t *)&client_struct_length) < 0){
            printf("Couldn't receive\n");
            return -1;
        }
        printf("----Received message from IP: %s and port: %i\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        printPacketDetails(receivedPacket);


        

        // Respond to client:
        if(receivedPacket.segmentno==0x0B){

         
        }
        //CASE IV-Duplicate packet error
        else if(previousSeqNo==receivedPacket.segmentno){
            r.rejectSubCode=REJECT_DUPLICATE_PACKET;
            r.clientId=receivedPacket.clientId;
            r.receivedSegmentNo=receivedPacket.segmentno;
            if (sendto(socket_desc, &r, sizeof(r), 0,(struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                return -1;
            }
            
        }
        //CASE I- Out of sequence reject
        else if(expectedSeqNo!=receivedPacket.segmentno){
            r.rejectSubCode=REJECT_OUT_OF_SEQUENCE;
            r.clientId=receivedPacket.clientId;
            r.receivedSegmentNo=receivedPacket.segmentno;
            if (sendto(socket_desc, &r, sizeof(r), 0,(struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
            return -1;
            }
           
        }

        //CASE II-length of field mismatch
        else if(receivedPacket.length!=strlen(receivedPacket.payload)){
            r.rejectSubCode=REJECT_LENGTH_MISMATCH;
            r.clientId=receivedPacket.clientId;
            r.receivedSegmentNo=receivedPacket.segmentno;
            if (sendto(socket_desc, &r, sizeof(r), 0,(struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                return -1;
            }
            
        }

        //CASE III- End of packet Identifier error
        else if(receivedPacket.endPacketId !=END_OF_PACKET_IDENTIFIER){
            r.rejectSubCode=REJECT_END_OF_PACKET;
            r.clientId=receivedPacket.clientId;
            r.receivedSegmentNo=receivedPacket.segmentno;
            if (sendto(socket_desc, &r, sizeof(r), 0,(struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                return -1;
            }
            
        }
        
        
        else{
            a1.clientId=receivedPacket.clientId;
            a1.receivedSegmentNo=receivedPacket.segmentno;
            if (sendto(socket_desc, &a1, sizeof(a1), 0,(struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                return -1;
            }
            previousSeqNo=expectedSeqNo;
            expectedSeqNo+=1;
            //sleep(2);
        }
    
    
        
    }
    
    return 0;
}
