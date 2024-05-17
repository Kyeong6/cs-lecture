#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

// in your browser type: http://localhost:8090
// IF error: address in use then change the PORT number
#define PORT 8090

// 스레드 간 동기화를 위한 뮤텍스
pthread_mutex_t mutex_lock;  

// 노드 구조체 정의
typedef struct Node {
    int socket_fd;  // 클라이언트 소켓 파일 디스크립터
    struct Node* next;
} Node;

// 큐 구조체 정의
typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

// 큐 초기화 함수
void initialize_queue(Queue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
}

// 큐에 새로운 노드를 추가하는 함수
void enqueue(Queue* queue, int socket) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->socket_fd = socket;
    new_node->next = NULL;

    if (queue->rear == NULL) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
}

// 큐에서 노드를 제거하고 소켓을 반환하는 함수
int dequeue(Queue* queue) {
    if (queue->front == NULL) {
        return -1; // 큐가 비어있음
    }

    Node* temp_node = queue->front;
    int socket = temp_node->socket_fd;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp_node);
    return socket;
}

// 큐의 크기를 반환하는 함수
int get_queue_size(Queue* queue) {
    int size = 0;
    Node* current_node = queue->front;
    while (current_node != NULL) {
        size++;
        current_node = current_node->next;
    }
    return size;
}

// 클라이언트 연결을 저장하는 큐
Queue connection_queue;

// 활성화된 스레드 수
int active_threads = 1;  

// 클라이언트 요청을 처리하는 함수
void *client_handler(void *arg) {
    int thread_id = *((int *)arg);
    int queue_size = get_queue_size(&connection_queue);

    int client_socket;
    while (queue_size >= 2 || (queue_size > 0 && active_threads > 1)) {
        pthread_mutex_lock(&mutex_lock);
        client_socket = dequeue(&connection_queue);
        pthread_mutex_unlock(&mutex_lock);

        if (client_socket != -1) {
            long valread;
            char buffer[30000] = {0};
            char response[30024] = {0};
            valread = read(client_socket, buffer, 30000);
            printf("Thread %d received: %s\n", thread_id, buffer);

            sleep(5);

            sprintf(response, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 20\n\nMy first web server!\n");
            write(client_socket, response, strlen(response));
            printf("Thread %d sent response\n", thread_id);

            close(client_socket);
            queue_size = get_queue_size(&connection_queue);
        }
    }

    printf("Thread %d exiting. Active threads: %d\n", thread_id, active_threads - 1);
    active_threads--;
    pthread_exit(NULL);
}

// 서버 로직 함수
void manage_server() {
    int queue_size = get_queue_size(&connection_queue);

    // 큐에 8개 이상의 요청이 있는 경우 새로운 스레드를 생성
    if (queue_size >= 8) {
        pthread_t new_thread;
        int thread_id = active_threads + 1;
        pthread_create(&new_thread, NULL, client_handler, &thread_id);
        active_threads++;
        printf("Created new thread.");
    }
}

int main(int argc, char const *argv[]) {
    int server_fd, client_socket;
    struct sockaddr_in server_address;
    int address_len = sizeof(server_address);

    // socket 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero);

    // socket bine
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("In bind");
        exit(EXIT_FAILURE);
    }

    // 연결 대기열 설정
    if (listen(server_fd, 10) < 0) {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    initialize_queue(&connection_queue);

    while (1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        printf("Queue size: %d\n", get_queue_size(&connection_queue));
        printf("Thread count: %d\n", active_threads);

        if ((client_socket = accept(server_fd, (struct sockaddr *)&server_address, (socklen_t *)&address_len)) < 0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&mutex_lock);
        enqueue(&connection_queue, client_socket); // 요청을 큐에 추가
        manage_server();
        pthread_mutex_unlock(&mutex_lock);
    }

    return 0;
}
