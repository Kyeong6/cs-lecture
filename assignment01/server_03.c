#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// in your browser type: http://localhost:8090
// IF error: address in use then change the PORT number
#define PORT 8090

// Message queue에 필요한 상수 설정
#define QUEUE_SIZE 10
#define MAX_CLIENTS 10

// Queue 구조체 정의
typedef struct {
    int client_sockets[QUEUE_SIZE];
    int head;
    int tail;
    int count;
} MessageQueue;

MessageQueue mq;

void initialize_message_queue(MessageQueue* mq) {
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    printf("Message queue initialized.\n");
}

// Message queue 삽입
void insert(MessageQueue* mq, int client_sock) {
    if (mq->count < QUEUE_SIZE) {
        mq->client_sockets[mq->tail] = client_sock;
        mq->tail = (mq->tail + 1) % QUEUE_SIZE;
        mq->count++;
        printf("Client socket %d enqueued. Queue count: %d\n", client_sock, mq->count);
    } else {
        // printf("Queue is full\n");
    }
}

// Message queue 제거
int pop(MessageQueue* mq) {
    if (mq->count > 0) {
        int client_sock = mq->client_sockets[mq->head];
        mq->head = (mq->head + 1) % QUEUE_SIZE;
        mq->count--;
        printf("Client socket %d dequeued. Queue count: %d\n", client_sock, mq->count);
        return client_sock;
    }
    // printf("Queue is empty\n");
    return -1; // Queue is empty
}

// Server logic 구현
void* server_logic(void* arg) {
    MessageQueue* mq = (MessageQueue*) arg;
    int sock;
    while (1) {
        while ((sock = pop(mq)) == -1) {
            usleep(100000); // Sleep if no clients
        }
        
        char buffer[30000] = {0};
        char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain;Content-Length: 20\n\nMy first web server!";
        read(sock, buffer, sizeof(buffer));
        printf("Client request processed: %s\n", buffer);
        write(sock, response, strlen(response));
        close(sock);
        printf("Response sent to client and connection closed.\n");
    }
    return NULL;
}

// 비율에 따른 server logic 추가 및 제거 구현
void adjust_server_logic(MessageQueue* mq, pthread_t* threads, int* active_threads) {
    printf("Adjusting server logic based on current queue size...\n");
    if (mq->count > 0.8 * QUEUE_SIZE && *active_threads < MAX_CLIENTS) {
        pthread_create(&threads[*active_threads], NULL, server_logic, (void*) mq);
        pthread_detach(threads[(*active_threads)++]);
        printf("New thread created. Total active threads: %d\n", *active_threads);
    } else if (mq->count < 0.2 * QUEUE_SIZE && *active_threads > 1) {
        (*active_threads)--;
        printf("Reduced thread count. Total active threads: %d\n", *active_threads);
    }
}

int main() {
    int server_fd, new_socket, active_threads = 1;
    pthread_t threads[MAX_CLIENTS];
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("In bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    initialize_message_queue(&mq);
    pthread_create(&threads[0], NULL, server_logic, (void*)&mq);
    pthread_detach(threads[0]);

    while (1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            perror("In accept");
            continue;
        }
        printf("New connection accepted: socket %d\n", new_socket);
        insert(&mq, new_socket);
        adjust_server_logic(&mq, threads, &active_threads);
    }

    return 0;
}
