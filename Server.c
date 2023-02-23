#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "SC.h"
#include "json-c/json.h"

pthread_t threads[BACKLOG];
pthread_mutex_t lock;

int magne_data[3];
int accele_data[3];
int gyro_data[3];

//JSON variables
struct json_object *parsed_json;
struct json_object *magnetometer_json;;
struct json_object *accelerometer_json;
struct json_object *gyroscope_json;
struct json_object *magnetometer;
struct json_object *accelerometer;
struct json_object *gyroscope;

size_t num_magnets;
size_t num_accelerometers;
size_t num_gyroscope;

FILE *fp;
char json_buffer[1024];

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    int read_size;

    frame_Request client_frame;
    frame_Response server_frame;

    server_frame.preamb = PREAMB;

    while((read_size = recv(sock ,(char *)&client_frame , sizeof(frame_Request) , 0)) > 0) 
    {
         //Checking for errors in client frame
        if(client_frame.checksum != 0xFB) {
            perror("Invalid checksum\n");
            continue;
        }
        printf("Client %i: Sensor-%i\t axis-%i\n", sock, client_frame.sensor, client_frame.axis);

        //Forming response
        if(client_frame.sensor == 1){
            server_frame.sensor = client_frame.sensor;
            switch(client_frame.axis){
                case 1:
                    server_frame.data[0] = magne_data[0];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 2:
                    server_frame.data[1] = magne_data[1];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 3:
                    server_frame.data[2] = magne_data[2];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 4:
                    server_frame.data[0] = magne_data[0];
                    server_frame.data[1] = magne_data[1];
                    server_frame.data[2] = magne_data[2];
                    server_frame.size = 7;
                    server_frame.checksum = 0xF8;
                  break;
            }
        }else if(client_frame.sensor == 2){
            server_frame.sensor = client_frame.sensor;
            switch(client_frame.axis){
                case 1:
                    server_frame.data[0] = accele_data[0];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 2:
                    server_frame.data[0] = accele_data[1];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 3:
                    server_frame.data[0] = accele_data[2];
                    server_frame.size = 5;
                    server_frame.checksum = 0xFA;
                    break;
                case 4:
                    server_frame.data[0] = accele_data[0];
                    server_frame.data[1] = accele_data[1];
                    server_frame.data[2] = accele_data[2];
                    server_frame.size = 7;
                    server_frame.checksum = 0xF8;
            }
         }else if(client_frame.sensor  == 3){
            server_frame.sensor = client_frame.sensor;
            switch(client_frame.axis){
               case 1:
                  server_frame.data[0] = gyro_data[0];
                  server_frame.size = 5;
                  server_frame.checksum = 0xFA;
                  break;
               case 2:
                  server_frame.data[0] = gyro_data[1];
                  server_frame.size = 5;
                  server_frame.checksum = 0xFA;
                  break;
               case 3:
                  server_frame.data[0] = gyro_data[2];
                  server_frame.size = 5;
                  server_frame.checksum = 0xFA;
                  break;
               case 4:
                  server_frame.data[0] = gyro_data[0];
                  server_frame.data[1] = gyro_data[1];
                  server_frame.data[2] = gyro_data[2];
                  server_frame.size = 7;
                  server_frame.checksum = 0xF8;
                  break;
            }
         }else if(client_frame.sensor  == 4){
            server_frame.sensor = client_frame.sensor;
            switch(client_frame.axis){
               case 1:
                  server_frame.data[0] = accele_data[0];
                  server_frame.data[1] = magne_data[0];
                  server_frame.data[2] = gyro_data[0];
                  server_frame.size = 7;
                  server_frame.checksum = 0xF8;
                  break;
               case 2:
                  server_frame.data[0] = accele_data[1];
                  server_frame.data[1] = magne_data[1];
                  server_frame.data[2] = gyro_data[1];
                  server_frame.size = 7;
                  server_frame.checksum = 0xF8;
                  break;
               case 3:
                  server_frame.data[0] = accele_data[2];
                  server_frame.data[1] = magne_data[2];
                  server_frame.data[2] = gyro_data[2];
                  server_frame.size = 7;
                  server_frame.checksum = 0xF8;
                  break;
               case 4:
                  server_frame.data[0] = accele_data[0];
                  server_frame.data[1] = accele_data[1];
                  server_frame.data[2] = accele_data[2];
                  server_frame.data[3] = magne_data[0];
                  server_frame.data[4] = magne_data[1];
                  server_frame.data[5] = magne_data[2];
                  server_frame.data[6] = gyro_data[0];
                  server_frame.data[7] = gyro_data[1];
                  server_frame.data[8] = gyro_data[2];
                  server_frame.size = 13;
                  server_frame.checksum = 0xF2;
                  break;
            }
         }else{
            //Error frame
            server_frame.sensor = SENSOR_FAIL;
            server_frame.size = SIZE_FAIL;;
            server_frame.checksum = CRC_FAIL;
            perror("Invalid axis\n");
            pthread_mutex_unlock(&lock);
            continue;
         }

        if(send(sock, &server_frame, sizeof(frame_Response), 0) < 0) {
            perror("Send failed\n");
        }
    }

      if(read_size == 0) {
         perror("Client disconnected\n");
      } else if(read_size == -1) {
         perror("Receive failed\n");
      }

      pthread_mutex_unlock(&lock);

   if(close(sock) < 0) {
      perror("Close socket failed\n");
   }

   pthread_exit(NULL);
}

int main(int argc, char **argv) {
   int sockfd, newfd;
   struct sockaddr_in host_addr, client_addr;
   socklen_t sin_size;

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Socket failed");
      exit(1);
   }
   puts("--Socket created\n");

   host_addr.sin_family = AF_INET;
   host_addr.sin_port = htons(PORT);
   host_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(host_addr.sin_zero), '\0', 8);

   if(bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1) {
      perror("Bind failed");
      exit(1);
   }
   puts("--Address binded\n");

   if(listen(sockfd, BACKLOG) == -1) {
      perror("Listen failed");
      exit(1);
   }
   puts("--Listening...\n");

   while(1) {
      sin_size = sizeof(struct sockaddr_in);
      if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
         perror("Accept failed");
         continue;
      }
      printf("Client found! Address: %s\n\n", inet_ntoa(client_addr.sin_addr));

      fp = fopen("data.json","r");
	      fread(json_buffer, 1024, 1, fp);
	      fclose(fp);

	 parsed_json = json_tokener_parse(json_buffer);
         json_object_object_get_ex(parsed_json, "magnetometer", &magnetometer_json);
	 json_object_object_get_ex(parsed_json, "accelerometer", &accelerometer_json);
         json_object_object_get_ex(parsed_json, "gyroscope", &gyroscope_json);

         num_magnets = json_object_array_length(magnetometer_json);
         num_accelerometers = json_object_array_length(accelerometer_json);
         num_gyroscope = json_object_array_length(gyroscope_json);

      for(int i = 0; i < num_accelerometers; i++){
         magnetometer = json_object_array_get_idx(magnetometer_json, i);
         accelerometer = json_object_array_get_idx(accelerometer_json, i);
         gyroscope = json_object_array_get_idx(gyroscope_json, i);
         magne_data[i] = json_object_get_int(magnetometer);
         accele_data[i] = json_object_get_int(accelerometer);
         gyro_data[i] = json_object_get_int(gyroscope);
      }

      if(pthread_create(&threads[newfd], NULL, handle_client, &newfd) < 0) {
         perror("Thread creation failed");
         continue;
      }//Thread sends client id through file descriptor variable
   }

   if(close(sockfd) < 0) {
      perror("Close socket failed\n");
      exit(1);
   }

   return 0;
}
