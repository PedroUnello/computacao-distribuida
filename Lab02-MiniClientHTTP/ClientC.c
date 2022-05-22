#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> 

#include <pthread.h>

#define MAX(a,b) (a > b ? a : b)
#define CLAMP(a, min, max) (a > max ? max : a < min ? min : a)

//Pedro Unello Neto - 41929713
//Alessandro Bezerra Da Silva - 41908767 

pthread_mutex_t mutex;
int lock;

struct sockaddr_in conn;

int PartOf(char* all, char* part){
    int i = 0, j = 0;
    if ( strlen(all) <= 0 ) { return -1; } 
    while (*(all + i) != '\0'){
        while (all[i] == part[j]){
            i++;j++;
            if (part[j] == '\0'){ return i;}
        }
        j = 0;
        i++;
    }
    return -1;
} 

void * GravaImagem(void * dados){
    
    char* urlImage = malloc(sizeof(char) * 2048); 
    strcpy(urlImage, (char*) dados);
    
    pthread_mutex_unlock(&mutex);
    lock = 0;

    char* pathToImage = urlImage;
    pathToImage += MAX(PartOf(urlImage, "="), 0) * sizeof(char);

    int nameIndex = 0;
    char* name = pathToImage;
    while ((nameIndex = PartOf(name, "/")) != -1)
    {
        name += nameIndex;
    }
    int server = socket(AF_INET, SOCK_STREAM, 0);
    connect(server, (struct sockaddr*)&conn, sizeof(conn));
    
    int pathSize = strlen(pathToImage);
    char* getResponse = malloc( sizeof(char) * 2048);
    char* getRequest = malloc( sizeof(char) * (25 + pathSize));
    if (pathToImage[0] == '/' || PartOf(pathToImage, "http://") != -1 || PartOf(pathToImage, "https://") != -1)
        snprintf(getRequest, 24 + pathSize, "GET %s HTTP/1.1\r\n\r\n", pathToImage);
    else 
        snprintf(getRequest, 25 + pathSize, "GET /%s HTTP/1.1\r\n\r\n", pathToImage);

    send(server, getRequest, strlen(getRequest), 0);
    int bytes = recv(server, getResponse, 2048, 0);

    if (PartOf(getResponse, "200 OK") == -1){ printf("\n%s: 404 Not Found\n", getRequest); return NULL; }
    else { printf("\n%s: 200 OK\n", getRequest); }
    
    FILE* htmlArq = fopen(name, "wb");

    char* length = getResponse;
    length += PartOf(getResponse, "Content-Length:");
    int size = atoi(length);
    int total = bytes;

    
    int header = PartOf(getResponse, "\r\n\r\n");
    if (header != -1){
        char* image = getResponse;
        image += header;
        fwrite(image, 1, bytes - header, htmlArq);
    } else {
        fwrite(getResponse, 1, bytes, htmlArq);
    }

    while (total < size){
        bytes = recv(server, getResponse, 2048, 0);
        fwrite(getResponse, 1, bytes, htmlArq);
        total += bytes;
    }

    shutdown(server, SHUT_RDWR); 
	close(server); 
    fclose(htmlArq);

    free(getResponse);
    free(urlImage);
    free(getRequest);

    return NULL;
}

int main(){

    //char* address = "www.gnu.org";
    //char* ip = "209.51.188.116";
    //int port = 80;

    char* address = "127.0.0.1";
    char* ip = "127.0.0.1";
    int port = 8000;

    pthread_mutex_init(&mutex, NULL);
    lock = 0;

    int server = socket(AF_INET, SOCK_STREAM, 0);
    conn.sin_addr.s_addr = inet_addr(ip);
    conn.sin_family = AF_INET;
    conn.sin_port = htons(port);
    
    printf("\nConnected = %d\n", connect(server, (struct sockaddr*)&conn, sizeof(conn)));

    char* getRequest = malloc(sizeof(char) * (32 + strlen(address)));
    snprintf(getRequest, 32 + strlen(address), "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", address);
    send(server, getRequest, strlen(getRequest), 0);
    free(getRequest);
    int inTag, inQuotes, linkIndex, index, bytes;
    char* getReceive = malloc(sizeof(char) * BUFSIZ); 
    char* buff = malloc(sizeof(char) * 2048); //https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers
    do {
        
        memset(getReceive, 0, BUFSIZ);
        bytes = recv(server, getReceive, BUFSIZ, 0);
        index = 0;

        while (index < bytes){
            char current = *(getReceive + index);
            if (inTag){
                
                if (current == '>') {  inTag = 0;}
                else if (current == '"' && !inQuotes) { inQuotes = !inQuotes; }

                else if (inQuotes && (current == ' ' || current == '>' || current == '"')) 
                {

                    if (PartOf(buff, ".png") >= 0 || PartOf(buff, ".jpg") >= 0 || PartOf(buff, ".jpeg") >= 0 ){
                                pthread_t thread;
                                lock = 1;
                                pthread_create(&thread, NULL, GravaImagem, (void*) buff);
                                while (lock){ sleep(1);}
                                pthread_join(thread, NULL);
                    } 
                    memset(buff, 0, 2048); 
                    linkIndex = 0;
                    inQuotes = !inQuotes;
                }
                else{

                    *(buff + linkIndex) = current;
                    linkIndex++;
                
                }
            }

            else if (current == '<'){
                inTag = 1;
            }

            index++;
        }

    } while (PartOf(getReceive, "</html>") == -1);
    
    shutdown(server, SHUT_RDWR); 
	close(server); 
    pthread_mutex_destroy(&mutex);
    free(getReceive);
    free(buff);

    return 0;
}

