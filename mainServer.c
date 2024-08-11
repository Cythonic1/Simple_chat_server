
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    struct sockaddr_in addr;
    int connfd;
    int uid;
    char name[32];
} client_t;

client_t *client[10]; // Array of client pointers to accept clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;



//function to find terminate the unnessary spaces;
void delete_space(char * str){
    int i, index = -1;

    i = 0;
    while (str[i] != '\0') {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            index = i;
            
        }   
        i++;
    }
    str[index + 1] = '\0';

}


// Add client to the queue
void queue_add(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 10; i++) {
        if (!client[i]) {
            client[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove client from the queue
void queue_remove(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 10; i++) {
        if (client[i]) {
            if (client[i]->uid == uid) {
                free(client[i]);
                client[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_to_all(char *buffer, int size,int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0 ; i < 10; i++) {
        if (client[i] && (client[i]->uid != uid)) {
            if(send(client[i]->connfd, buffer, size, 0) < 0 ){
                perror("Something went wrong while sending");
                break;
            }    
        }
    }
    pthread_mutex_unlock(&clients_mutex);

}

void send_to_client(char *buffer, int size, int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i  = 0; i < 10; i++) {
        if (client[i] && (client[i]->uid == uid)) {
            if(send(client[i]->connfd, buffer, size, 0) < 0){
                perror("Something went wrong while sending to clinet");
                break;
            }
            break;
        }
    
    }
    pthread_mutex_unlock(&clients_mutex);
}


char *send_to_client_recv(char *buffer, int size, int uid){
    char *tmp = (char *)malloc(sizeof(char) * 32);
    if(!tmp){
        perror("Malloc failed to allocate memory");
        return NULL;
    }
    int recev;
    pthread_mutex_lock(&clients_mutex);
    for (int i  = 0; i < 10; i++) {
        if (client[i] && (client[i]->uid == uid)) {
            if(send(client[i]->connfd, buffer, size, 0) < 0){
                perror("Something went wrong while sending to clinet");
                free(tmp);
                break;
            }
            recev = recv(client[i]->connfd, tmp, 32, 0);
            printf("We receive this number of byrtes: %d\n", recev);
            tmp[recev] = '\0';
            pthread_mutex_unlock(&clients_mutex);
            return tmp;
        } 
    }

    free(tmp);
    return NULL;
}

void userName_Handller(client_t *cli){
    int bytes_read;
    char notify[60];
    char *username = send_to_client_recv("what would you like to be called: ", 34, cli->uid);
    // setting user name.
    delete_space(username);
    
    int userName_len = strnlen(username, 32);
    strncpy(cli->name, username, userName_len);
    if (userName_len == 32) {
        while (1) {
            char flush_buffer[1];
            bytes_read = recv(cli->connfd, flush_buffer, 1, 0);
            if (bytes_read <= 0 || flush_buffer[0] == '\n') {
                break;
            }
        }
    }
    snprintf(notify,sizeof(notify),"\nMr %s has join the chat\n",cli->name);
    printf("The notify content is %s\n", notify);
    send_to_all(notify, sizeof(notify),cli->uid);
    
}


//// Function to handle communication with a client
void *handle_client(void *arg) {
    char buffer[1024], dicli[60], username_with_message[340]; 
    int bytes_read;
    client_t *cli = (client_t *)arg;
    printf("Client %d connected.\n", cli->uid);
    userName_Handller(cli); 
    // Base receving messages;
    while (1) {
        
        memset(username_with_message, 0, sizeof(username_with_message));
        sprintf(username_with_message, "\n%s >", cli->name);
        send_to_client(username_with_message, sizeof(username_with_message), cli->uid);   
        printf("The value of the username buffer is : %s\n", username_with_message);
        
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recv(cli->connfd, buffer, sizeof(buffer), 0);
        printf("am in the while and i receive this number of bytes:%d\n", bytes_read);
        
        
        //strcat(buffer,username_with_message);
        printf("The value inside the buffer after the cat is: %s\n", buffer);
        if (bytes_read <= 0) {
            // Client disconnected or error
            sprintf(dicli,"The user %s has disconnected\n", cli->name);
            send_to_all(dicli, sizeof(dicli), cli->uid);
            printf("Client %d disconnected.\n", cli->uid);
            close(cli->connfd);
            queue_remove(cli->uid);
            break;
        }
        buffer[bytes_read] = '\0';  // Ensure the received message is null-terminated
        strcat(username_with_message, buffer);
        printf("The value inside the buffer after strcat is: %s\n", username_with_message);
        // Broadcast message to all other clients
        if(bytes_read == 1){
            continue;
        }else{
            send_to_all(username_with_message,strlen(username_with_message),cli->uid);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int serverfd, connfd, uid = 0;
    struct sockaddr_in server, client;
    socklen_t size = sizeof(struct sockaddr_in);
    pthread_t tid;

    if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error occurred while initializing the socket\n");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(0); // Port number
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&(server.sin_zero), '\0', 8);
    if (bind(serverfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        printf("Error while binding\n");
        exit(EXIT_FAILURE);
    }
    if (getsockname(serverfd, (struct sockaddr *)&server, &size) < 0) {
        perror("getsockname failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }
    if (listen(serverfd, 10) != 0) {
        printf("Error while listening\n");
        exit(EXIT_FAILURE);
    }

    printf("The server is listening at %s, and port %i\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

    while (1) {
        connfd = accept(serverfd, (struct sockaddr *)&client, &size);
        if (connfd < 0) {
            printf("Error accepting connection\n");
            continue;
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = client;
        cli->connfd = connfd;
        cli->uid = uid++;
        sprintf(cli->name, "%d", cli->uid);

        queue_add(cli);

        // Create a thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, (void *)cli) != 0) {
            printf("Failed to create thread\n");
            free(cli);
        }
    }

    return 0;
}







