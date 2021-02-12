#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <time.h>  
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>




#define PORT_NO 8080
#define START_OF_PACKET_IDENTIFIER 0XFFFF
#define END_OF_PACKET_IDENTIFIER 0XFFFF

#define DATA 0XFFF1
#define ACK 0XFFF2
#define REJECT 0XFFF3

#define REJECT_OUT_OF_SEQUENCE 0XFFF4
#define REJECT_LENGTH_MISMATCH 0XFFF5
#define REJECT_END_OF_PACKET 0XFFF6
#define REJECT_DUPLICATE_PACKET 0XFFF7

#pragma pack(push,1) //Will prevent padding
typedef struct dataPacket{
    uint16_t startPacketId; //2bytes
    uint8_t clientId;  //1 byte
    uint16_t data;   //2bytes
    uint8_t segmentno;  //1 byte
    uint8_t length;  //1byte
    char payload[255];
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
    printf("-------------------------------------------------------\n");
}

void printAckDetails(ackPacket a){
    printf("StartPacketId %x\n",a.startPacketId);
    printf("ClientId %x\n",a.clientId);
    printf("ACK %x\n",a.ack);
    printf("SegmentNumber %u\n",a.receivedSegmentNo);
    printf("EndPacketId %x\n",a.endPacketId);
    printf("--------------------------------------------------------\n");
}

void printRejectDetails(rejectPacket r){
    printf("----------Received Reject with subcode %x--------\n",r.rejectSubCode); 
    printf("StartPacketId %x\n",r.startPacketId);
    printf("ClientId %x\n",r.clientId);
    printf("Reject %x\n",r.reject);
    printf("Reject subcode %x\n",r.rejectSubCode);
    printf("SegmentNumber %u\n",r.receivedSegmentNo);
    printf("EndPacketId %x\n",r.endPacketId);
    printf("------------------------------------------------------------\n");
}




int main(void){

    uint8_t clientId=0XFA;
    uint8_t segmentnumber=0x1;

    dataPacket dp;
    rejectPacket server_message;
    dp.startPacketId=START_OF_PACKET_IDENTIFIER;
    dp.clientId=clientId;
    dp.data=DATA;
    dp.endPacketId=END_OF_PACKET_IDENTIFIER; 
    
    ackPacket a;

    int socket_desc;
    struct sockaddr_in server_addr;
    int server_struct_length = sizeof(server_addr);
    void * buffer=malloc(sizeof(rejectPacket));

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


    for(int i=1;i<=11;i++){
        dp.endPacketId=END_OF_PACKET_IDENTIFIER; 
        int retryCounter=0;
        fd_set rfds;
        struct timeval tv;
        int retval;

        
        while(retryCounter<3){


            dp.segmentno=segmentnumber;
            sprintf(dp.payload,"Packet%u",dp.segmentno);
            dp.length=strlen(dp.payload);

            
            //REJECT out of sequence reject code 0XFFF4
            if(i==7){
                dp.segmentno=segmentnumber+2;  
            }

            //End of packetId reject code 0XFFF6
            if(i==9){
                dp.endPacketId=0XFFF0;
            }
            
            //Length Mismatch reject code 0XFFF5
            if(i==10){
                dp.length+=1;
            }
            //Duplicate packet reject code 0XFFF7
            if(i==8){
                dp.segmentno=segmentnumber-1;
            }
            if(i==11){
                dp.segmentno=0x0B;
            }


            // Send the message to server:
            if(sendto(socket_desc, &dp, sizeof(dp), 0,
            (struct sockaddr*)&server_addr, server_struct_length) < 0){
                printf("Unable to send message\n");
                return -1;
            }
            else{
                printf("Sending Packet\n\n");
                
            } 

            /* Watch stdin (fd 0) to see when it has input. */
            FD_ZERO(&rfds);
            FD_SET(socket_desc, &rfds);
            tv.tv_sec = 3;
            tv.tv_usec = 0;
           
            retval=select(socket_desc+1, &rfds, NULL, NULL, &tv);
            //Returns 0 if socket timed out
            
            //Receive the server's response:
            
            if(retval){
                recvfrom(socket_desc, buffer, sizeof(server_message), 0,
                    (struct sockaddr*)&server_addr, (socklen_t *)&server_struct_length);
                
                void * ptr;
                ptr=buffer;
                buffer+=sizeof(uint16_t)+sizeof(uint8_t);
                uint16_t type;
                memcpy(&type,buffer,sizeof(uint16_t));
                if(type==ACK){
                    printf("-------Received ACK------------\n");
                    ackPacket a;
                    memcpy(&a,ptr,sizeof(ackPacket));
                    printAckDetails(a);
                    segmentnumber++;
                }
                else{
                    
                    rejectPacket r;
                    memcpy(&r,ptr,sizeof(rejectPacket));
                    printRejectDetails(r);
                }
                
                break;
            }
            else{
                retryCounter++;
                if(retryCounter<3){
                    printf("\nError: Server didnot respond Resending the packet\n");
                }
                
                
            }

        }
        if(retryCounter==3){
            printf("\nError: Server does not respond\n");
            break;
        }

        
        
    } 
    // Close the socket:
    close(socket_desc);
    
    return 0;
}

