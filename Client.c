#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "SC.h"

pthread_mutex_t mutex;

int main(int argc, char *argv[])
{
   int fd, numbytes;
   struct sockaddr_in server;
   struct hostent *lh;//localhost

   //Frames
   frame_Request client_frame;
   frame_Response server_frame;
   client_frame.preamb = PREAMB;

   if ((fd = socket(AF_INET, SOCK_STREAM, 0))==-1){
      printf("failed to create socket\n");
      exit(-1);
   }
   puts("--Socket created\n");

   if ((lh=gethostbyname("localhost")) == NULL){
      printf("failed to get hostname\n");
      exit(-1);
   }//obtain localhost

   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr = *((struct in_addr *)lh->h_addr);
   bzero(&(server.sin_zero),8);

   if(connect(fd, (struct sockaddr *)&server,  sizeof(struct sockaddr))==-1){
      printf("failed to connect\n");
      exit(-1);
   }
   puts("--Connection successful\n");

   while(1){
         printf("===================================== MENU =====================================\n");
         printf("Select the number of your desired option\n");
         printf("Sensors:\n1 = Magnetometer\n2 = Accelerometer\n3 = Gyroscope\n4 = All sensors\n");
         printf("================================================================================\n");
         scanf("%d",(int *)&client_frame.sensor);

         printf("Select the number of your desired axis:\n1 = X\n2 = Y\n3 = Z\n4 = All axis\n");
         printf("================================================================================\n");
         scanf("%d",(int *)&client_frame.axis);

         client_frame.checksum = 0xFB;
         pthread_mutex_lock(&mutex);

         send(fd,(char *)&client_frame,sizeof(frame_Request),0);//Request frame

      if ((numbytes=recv(fd,(char *)&server_frame,sizeof(frame_Response),0)) == -1){
         printf("failed to recieve\n");
         continue;
      }//Response frame

      //Invalid checksums
      if(server_frame.sensor < 4 && server_frame.sensor > 0){
         switch(server_frame.size){
            case 5:
               if(server_frame.checksum != 0xFA){
                  perror("Invalid checksum\n");
                  continue;
               }
               break;
            case 7:
               if(server_frame.checksum != 0xF8){
                  perror("Invalid checksum\n");
                  continue;
               }
               break;
         }
      }else if(server_frame.sensor == 4){
         switch(server_frame.size){
            case 7:
               if(server_frame.checksum!= 0xF8){
                  perror("Invalid checksum\n");
                  continue;
               }
               break;
            case 13:
               if(server_frame.checksum != 0xF2){
                  perror("Invalid checksum\n");
                  continue;
               }
               break;
         }
      }

      system("clear");
      printf("\nFrame:\n\n");

      if(server_frame.sensor != 4){
         if(server_frame.size != 7){
            printf("%X-%X-%X-%X-%X\n",server_frame.preamb,server_frame.sensor,server_frame.size, server_frame.data[0],server_frame.checksum);
         }else{
            printf("%X-%X-%X",server_frame.preamb,server_frame.sensor,server_frame.size);
            for(int i=0;i<3;i++){
               printf("-%X",server_frame.data[i]);
            }
            printf("-%X\n", server_frame.checksum);
         }
      }else{
         if(server_frame.size != 13){
            printf("%X-%X-%X",server_frame.preamb,server_frame.sensor,server_frame.size);
            for(int i=0;i<3;i++){
               printf("-%X",server_frame.data[i]);
            }
            printf("-%X\n", server_frame.checksum);
         }else{
            printf("%X-%X-%X", server_frame.preamb,server_frame.sensor,server_frame.size);
            for(int i=0;i<13;i++){
               printf("-%X",server_frame.data[i]);
            }
            printf("-%X\n", server_frame.checksum);
         }
      }

      pthread_mutex_unlock(&mutex);
   }
   return 0;
}
