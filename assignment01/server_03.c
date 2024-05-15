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

// queue size 설정
#define QUEUE_SIZE 10

// message buffer size 설정
#define MESSAGE_BUFFER_SIZE 150

// Queue struct 정의
typedef struct {
    char* messages[QUEUE_SIZE]; // message 저장 배열
    int head; 
    int tail; 
    int count; 
} MessageQueue;

// Queue 초기화
void initialize_message_queue(MessageQueue* mq) {
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    for (int i = 0; i < QUEUE_SIZE; i++) {
        mq->messages[i] = NULL;
    }
}

// Queue 메시지 추가
void insert(MessageQueue* mq, const char* message) {
    if (mq->count < QUEUE_SIZE) {
        int nextTail = mq->tail;
        // message 복사하여 저장
        mq->messages[nextTail] = strdup(message);
        // 순환 queue 구현 
        mq->tail = (nextTail + 1) % QUEUE_SIZE;
        mq->count++;
    } else {
        printf("full\n");
    }
}


// thread에서 실행되며 client와 연결을 관리하는 함수
void *socket_connection(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[30000] = {0};
    char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain" \
                  "Content-Length: 20\n\nMy first web server!";
    read(sock, buffer, 30000);
    printf("%s\n", buffer);
    sleep(5);
    write(sock, hello, strlen(hello));
    printf("-------------Hello message sent---------------\n");
    close(sock);
    free(socket_desc);
    return 0;
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain" \
                  "Content-Length: 20\n\nMy first web server!";
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        // thread 생성
        pthread_t tid;
        int *new_sock = malloc(1);
        *new_sock = new_socket;
        if(pthread_create(&tid, NULL, socket_connection, (void*) new_sock) < 0)
        {
            perror("not create thread");
            return 1;
        }
        pthread_detach(tid);
    }
    return 0;
}

