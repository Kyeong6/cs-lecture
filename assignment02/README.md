## #1.  **생산자-소비자로 구성된 응용프로그램 만들기**

### 순서

1. 07강 강의자료에서 생성자-소비자 문제를 다시 한번 보기
2. 주어진 조건에 맞는 간단한 생성자-소비자 문제 애플리케이션을 만들어보기
3. 함께 제공된 procon.c 파일은 본 문제의 스켈레톤 코드입니다. [Write here]를 중심으로 작성하면 더욱 쉽게 해결할 수 있다.
<br/></br>

**조건**

“1개의 생산자 스레드와 1개의 소비자 스레드로 구성되는 간단한 응용프로그램을 작성”

- 생산자 스레드
    - 0 ~ 9까지 10개의 정수를, 랜덤한 시간 간격으로, 공유버퍼에 사용
- 소비자 스레드
    - 공유버퍼로부터 랜덤한 시간 간격으로, 10개의 정수를 읽어 출력
- 공유버퍼
    - 4개의 정수를 저장하는 원형 큐로 작성
    - 원형 큐는 배열로 작성
- 2개의 세마포어 사용
    - semWrite : 공유버퍼에 쓰기 가능한 공간(빈 공간)의 개수를 나타냄
    - 초기값이 4인 counter 소유
    - semRead : 공유버퍼에 읽기 가능한 공간(값이 들어있는 공간)의 개수를 나타냄
    - 초기값이 0인 counter 소유
- 1개의 뮤텍스 사용
    - pthread_mutex_t critical_section
    - 공유버퍼에서 읽는 코드와 쓰는 코드를 임계구역으로 설정
    - 뮤텍스를 이용하여 상호배제
<br/></br>

**주의사항**

MacOS에서 Semaphore가 정상적으로 작동하지 않은 경우가 존재합니다. 
<br/></br>

### 설명

[Write here]가 존재하는 함수는 `mywrite` , `myread` `main` 이다. 각각의 함수의 역할은 다음과 같습니다.

- mywrite :  생산자 스레드가 공유 버퍼에 값을 쓰는 함수
- myread : 소비자 스레드가 공유 버퍼에서 값을 읽는 함수
- main : 두 개의 세마포어(semWrite와 semRead)와 하나의 뮤텍스(critical_section)를 적절하게 초기화하고 사용
<br/></br>

**Initial value**

```c
int wptr = 0; // write pointer for queue[]
int rptr = 0; // read pointer for queue[]
```

mywrite() 및 myread()에서 인덱스로 사용하기 위한 초기값을 설정합니다.
<br/></br>

**mywrite()**

```c
// write n into the shared memory
void mywrite(int n) { 
  // wait for empty space
  sem_wait(&semWrite); 
  
  // enter critical section
  pthread_mutex_lock(&critical_section);

  queue[wptr] = n;
  wptr = (wptr + 1) % N_COUNTER;

  // leave critical section
  pthread_mutex_unlock(&critical_section); 

  // signal that there is a new item
  sem_post(&semRead);
}
```

mywrite() 함수의 역할을 다시 언급하자면 생산자 스레드가 공유 버퍼에 값을 쓸 때 호출되는 함수입니다. 

해당 함수는 세마포어와 뮤텍스를 사용하여 동기화 및 상호 배제를 보장한다. 함수의 기능들을 설명하면 다음과 같습니다.

- sem_wait(&semWrite);
    - semWrite 세마포어의 값을 감소시키고, 만약 값이 0이면 생산자 스레드는 대기 상태가 됩니다
    - semWrite 세마포어는 공유 버퍼의 빈 공간을 나타내며, 초기값은 조건에 따라 4입니다.
    - 따라서, 첫 번째 호출일 경우 즉시 통과합니다.
- pthread_mutex_lock(&critical_section);
    - 뮤텍스를 잠그고 임계 구역에 들어가게 해주는 기능을 담당합니다.
    - 임계 구역은 공유 자원인 queue 배열에 대한 접근을 보호하여 여러 스레드가 동시에 접근하는 것을 방지하여 데이터의 일관성을 유지합니다.
- queue[wptr] = n;
    - 공유 버퍼의 현재 쓰기 위치에 값을 씁니다.
    - wptr은 초기값 설정에서 0으로 설정하였고, 쓰기 위치를 가리키는 포인터입니다.
- wptr = (wptr + 1) % N_COUNTER;
    - wptr를 원형 큐 방식으로 다음 위치로 이동시키는 기능을 합니다.
    - wptr이 N_COUNTER를 초과하면 다시 0으로 돌아가도록 합니다.(원형 큐 방식)
- pthread_mutex_unlock(&critical_section);
    - 뮤텍스를 풀고 임계 구역에서 나오는 기능을 합니다.
    - 즉, 다른 스레드가 임계 구역에서 들어갈 수 있게 되는 상황이 됩니다.
- sem_post(&semRead);
    - semRead 세마포어의 값을 증가시킵니다.
    - semRead 세마포어는 공유 버퍼에 읽을 수 있는 항목의 수를 나타내며, 소비자 스레드가 대기 중인 경우 세마포어의 값이 증가하면 소비자 스레드가 깨어나서 작업을 수행할 수 있게 되는 상황이 됩니다.
<br/></br>

**myread()**

```c
// write a value from the shared memory
int myread() { 
  // wait for an available item
  sem_wait(&semRead);

  // enter critical section
  pthread_mutex_lock(&critical_section);

  int n = queue[rptr];
  rptr = (rptr + 1) % N_COUNTER;

  // leave critical section
  pthread_mutex_unlock(&critical_section);

  // signal that there is empty space
  sem_post(&semWrite);

  return n;
}

```

myread() 함수의 역할은 소비자 스레드가 공유 버퍼에서 값을 읽을 때 호출하는 기능을 담당합니다. 

해당함수도 위의 mywrite() 함수와 같이 세마포어와 뮤텍스를 사용하여 동기화 및 상호 배제를 보장합니다.

함수의 기능들을 설명하면 다음과 같습니다.

- sem_wait(&semRead);
    - semRead 세마포어의 값을 감소시키고, 값이 0이면 소비자 스레드가 대기 상태에 들어가는 상황을 만듭니다.
    - semRead 세마포어는 공유 버퍼에 읽을 수 있는 항목의 수를 의미하며, 초기값은 0입니다.
    - 따라서 생산자 스레드가 값을 쓸 때까지 소비자 스레드는 대기합니다.
- pthread_mutex_lock(&critical_section);
    - 뮤텍스를 잠그고 임계 구역에 들어가게하는 기능을 담당합니다.
- int n = queue[rptr];
    - 공유 버퍼의 현재 읽기 위치에서 값을 읽어 변수 n에 저장합니다
    - rptr은 wptr와 같이 위치를 가리키는 포인터이지만, 읽기 위치를 가리킵니다.
- rptr = (rptr + 1) % N_COUNTER;
    - 위에서 언급한 wptr와 같이 원형 큐 방식으로 다음 위치로 이동시키는 기능입니다.
- pthread_mutex_unlock(&critical_section);
    - mywrite()에서 언급하여 생략
- sem_post(&semWrite);
    - semWrite 세마포어의 값을 증가시키는 역할을 합니다.
<br/></br>

**main**()

해당 부분에서 존재하는 [Write here]은 세마포어의 초기화와 소멸을 담당하는 부분입니다. 
<br/></br>

**세마포어 초기화**

```c
// init semaphore
sem_init(&semWrite, 0, N_COUNTER); // initial value is the size of the buffer
sem_init(&semRead, 0, 0); // initial value is 0
```

- sem_init(&semWrite, 0, N_COUNTER);
    - semWrite 세마포어를 초기화합니다.
    - &semWrite : 초기화할 세마포어의 포인터를 의미합니다.
    - 0 : 세마포어의 유형을 지정합니다.(0은 스레드 간 공유 의미)
    - N_COUNTER : 세마포어의 초기값 지정(현재 N_COUNTER가 4로 설정되어있어, 공유버퍼의 크기가 4라는 의미이다)

sem_init(&semRead, 0, 0);은 위의 인자의 유형과 동일하여 생략하겠습니다.

정리하자면 sem_init(&semWrite, 0, N_COUNTER); 은 공유 버퍼의 쓰기 가능한 빈 공간의 수를 나타내고, 
sem_init(&semRead, 0, 0); 은 공유 버퍼에 읽을 수 있는 항목이 없음을 나타냅니다.
<br/></br>

**세마포어 소멸**

```c
//destroy the semaphores
sem_destroy(&semWrite);
sem_destroy(&semRead);
```

sem_destroy(&semWrite);, sem_destroy(&semRead); 각각의 해당하는 세마포어와 관련된 리소스를 해제하는 역할을 합니다. (프로그램이 종료될 때 세마포어를 소멸시켜야 한다.)
<br/></br>

### 실행 결과

<img width="173" alt="procon c 실행결과_01" src="https://github.com/Kyeong6/operating-system/assets/100195725/0ece478a-38d1-4534-ab74-e764556aa414">

gcc procon.c; ./a.out 명령어로 실행한 결과 다음과 같은 결과를 얻었습니다. (MacOS이지만 정상적으로 작동하였습니다.)
<br/></br>

## #2.  소프트웨어로 문을 만드는 방법

### 순서

1. 소프트웨어 기반 동기화 방식(알고리즘)들에 대해 간단한 조사를 해보고, 그에 대한 설명을 작성하기
2. 조사한 동기화 방식들 중 하나의 방식을 선택하여 직접 구현해보고, 고른 이유도 간단하게 설명하기
3. 2번 과정에서 구현한 알고리즘을 #1의 문제에서 pthread_mutex 대신 활용해보기
4. 오리지널 pthread_mutex와 선택한 software lock과의 프로그램의 성능 비교해보기
<br/></br>

### 설명

**소프트웨어 기반 동기화 방식(알고리즘)**

- Dijkstra Algorithm
    - 세마포어를 사용하여 프로세스 간의 상호 배제를 구현합니다.
    - 두 가지 기본 연산인 P(proveren), V(verhogen) 사용합니다.
- Dekker Algorithm
    - 두 프로세스가 임계 구역에 진입하기 위한 권리를 번갈아가며 획득하는 방식으로 진행됩니다.
    - 플래그와 턴 변수를 사용하여 두 프로세스가 동시에 임계 구역에 진입하지 못하도록 합니다.
- Peterson Algorithm
    - 두 프로세스 간의 상호 배제를 보장하는 알고리즘(Dekker 알고리즘을 개선한 형태)
    - 두 개의 플래그와 턴 변수를 사용하여 두 프로세스가 임계 구역에 진입하는 순서를 결정합니다.
- Bakery(Lamport) Algorithm
    - 여러 프로세스 간의 상호 배제를 보장하는 알고리즘
    - 네트워크 상에서의 분산 시스템에서 사용됩니다.
    - 각 프로세스가 임계 구역에 진입하기 위해 Time Stamp 개념을 사용하며, Time Stamp가 작은 프로세스가 먼저 임계 구역에 진입하는 원리입니다.
<br/></br>

**Peterson Algorithm 선정 이유**

위의 나머지 알고리즘에 비해 구현이 비교적 간단한 Peterson Algorithm을 선택하여 진행하였습니다.
<br/></br>

**Peterson 알고리즘의 전반적인 흐름**

Petereson 알고리즘은 두 프로세스가 임계 구역(Critical Section)에 진입할 수 있도록 조정하여, 동시에 진입하는 것을 방지합니다. 알고리즘은 두 개의 플래그 배열과 턴 변수를 사용하여 작동합니다.
<br/></br>

**Peterson 알고리즘 구현 및 적용**

1. 전역 변수 및 세마포어 선언

```c
int flag[2] = {0, 0}; // Peterson algorithm flags
int turn = 0; // Peterson algorithm turn
```

#1 에서 수정 및 추가한 내용만 설명하도록 하겠습니다.

- flag : 프로세스가 임계영역을 사용하겠다고 알리는 역할
    - flag[0]은 프로세스 0의 상태
    - flag[1]은 프로세스 1의 상태
- turn : 어떤 프로세스를 실행시킬건지 가르키는 변수
<br/></br>

2. Peterson 알고리즘을 이용한 lock 및 unlock 함수

```c
void peterson_lock(int id) {
  int other = 1 - id;
  flag[id] = 1;
  turn = other;
  asm("mfence"); // Memory fence
  while (flag[other] && turn == other) {
    // busy wait
  }
}

void peterson_unlock(int id) {
  flag[id] = 0;
  asm("mfence"); // Memory fence
}
```

- peterson_lock 함수 : Peterson 알고리즘을 사용하여 임계 구역에 대한 진입 제어
- peterson_unlock 함수 : 임계 구역에서 나올 때 호출 진행
- asm(”mfence”) : 메모리 순서를 보장하기 위해 사용
<br/></br>

3. 생상자 스레드 함수

```c
// producer thread function
void* producer(void* arg) { 
  for(int i = 0; i < 10; i++) {
    peterson_lock(0);
    mywrite(i); // write i into the shared memory
    printf("producer : wrote %d\n", i);
    peterson_unlock(0);

    // sleep m milliseconds
    int m = rand() % 10;
    usleep(MILLI * m * 10); // m * 10 milliseconds
  }
  return NULL;
}

```

- 생산자 스레드는 0부터 9까지의 값을 공유 버퍼를 사용하므로 i의 범위 0 ~ 9
- peterson_lock(0)으로 임계 구역에 진입하고, mywrite()를 호출하여 값 작성
- peterson_unlock(0)으로 임계 구역에서 나가기
<br/></br>

4. 소비자 스레드 함수

```c
// consumer thread function
void* consumer(void* arg) { 
  for(int i = 0; i < 10; i++) {
    peterson_lock(1);
    int n = myread(); // read a value from the shared memory
    printf("\tconsumer : read %d\n", n);
    peterson_unlock(1);

    // sleep m milliseconds
    int m = rand() % 10;
    usleep(MILLI * m * 10); // m * 10 milliseconds
  }
  return NULL;
}
```

- 소비자 스레드는 공유 버퍼에서 값 읽기 진행
- peterson_lock(1)을 통해 임계 구역에 진입, myread()를 호출하여 값 읽기
- peterson_unlock(1)으로 임계 구역에서 나가기

1. 공유 버퍼에 값 쓰기 및 읽기 함수 및 main 함수
- procon.c와 동일한 내용이여서 생략하겠습니다.
<br/></br>

**pthread_mutex vs Peterson algorithm**

time 라이브러리를 이용해 main() 함수에 추가하여 다음과 같은 실행 시간 결과를 얻었습니다.

- pthread_mutex : Elapsed time: 0.467514 seconds

<img width="187" alt="procon c time check" src="https://github.com/Kyeong6/operating-system/assets/100195725/f1739117-596f-46b8-bc8e-f4dae5e4de1b">

- peterson algorithm : Elapsed time: 0.551858 seconds

<img width="187" alt="procon2 c time check" src="https://github.com/Kyeong6/operating-system/assets/100195725/2d35acec-3501-481d-936b-c9554d2cd47b">
<br/></br>

시간 차이(성능)의 원인은 다음과 같습니다.
<br/></br>

1. busy-waiting 
    - peterson algorithm : busy-waiting을 사용하여 lock을 얻는동안 CPU 자원을 소모하여, 임계 구역에 진입할 때마다 CPU를 낭비하게 되어 성능에 부정적인 영향을 끼칩니다.
    - pthread_mutex : Kernel에서 lock을 관리해서 peterson_algorithm과 달리 busy-waiting을 피합니다.
2. 구조적 차이
    - peterson algorithm : 단일 프로세스 환경에서 설계되었기 때문에 멀티코어 환경에서 성능 저하가 발생합니다.
    - pthread_mutex : 멀티코어 환경에서 최적화되어있고, 각 프로세스나 스레드가 독립적으로 임계 구역을 관리할 수 있습니다.

조사를 하면서 알게된 점은 실제 사용 환경에서는 pthread_mutex를 사용하는 것이 성능 면에서 유리하며, peterson algorithm은 상호 배제를 이해하는 데 유용하지만, 실제 사용 환경에서는 사용하지 않는다고 합니다.
<br/></br>

## #3.  내 컴퓨터의 페이지 크기는 얼마일까?

### 순서

1. page.c를 컴파일하고 실행, 실행시킬때 time 명령어와 함께 실행하여 실행 시간을 측정하기
(e.g. time ./a.out)
2. 컴파일된 파일은 pagesize 변수를 인자값으로 받습니다. 값을 변경해가며 실행시간의 변화를 확인해보기
(e.g. time ./a.out 1024)
3. 입력 값이 특정 값에 이르면 (i.e., 실제 page size) 갑자기 실행시간이 증가한다. 이를 통해 본인의 컴퓨터의 페이지 크기를 확인해보기. 혹시 시간 값의 차이가 잘 보이지 않거나, 실행에 이상이 있으면 #define된 각종 값들을 변경해보기.
4. 리눅스(or MacOS)에서 본인의 컴퓨터에 설정된 페이지 크기를 확인하는 명령어가 무엇인지 찾아보고, 찾아낸 값과 일치하는지 비교해보기. 거꾸로 실제 값을 확인해놓고, 값을 찾아보는 것도 좋은 방법이다. 
예를 들어 실제 값이 1000이라면, 999, 1000, 1001 이렇게 실행시켜가면서 확인하기
<br/></br>

### 설명

**page.c 컴파일 및 time 명령어와 함께 실행**

<img width="784" alt="page_size_01" src="https://github.com/Kyeong6/operating-system/assets/100195725/5aee7912-48aa-4845-a009-87b4aca9b7e7">

time a./out {page_size}를 통해서 실행한 결과입니다.
<br/></br>

**실행시간의 변화 및 특정 값에서의 실행 시간 변화 확인**

<img width="784" alt="page_size_02" src="https://github.com/Kyeong6/operating-system/assets/100195725/885cbbe0-b11d-4863-a1af-9ff572be5cfe">

2의 제곱으로 pagesize 값을 설정하여 실행시간의 변화를 확인했습니다.

- pagesize = 64
- pagesize = 2048

위의 pagesize를 사용했을 때 유난히 실행 시간이 길어졌음을 확인이 가능합니다.
<br/></br>

**리눅스 또는 MacOS에서의 페이지 크기 확인**

리눅스나 MacOS에서 시스템 구성설정 변수값을 확인하는 명령어는 `getconf` 입니다. 

```bash
# pagesize check
getconf PAGESIZE
```

위의 명령어를 통해 실제 페이지 크기를 확인한 후 해당 페이지 크기에 대한 실행시간은 다음과 같습니다.

<img width="784" alt="page_size_03" src="https://github.com/Kyeong6/operating-system/assets/100195725/76c5a621-e5cc-464a-ab18-b745fc5a86c9">
<br/></br>

실제 페이지 크기인 16384를 얻고 인접한 값들로 실행시간을 수행한 결과 실제 페이지 크기에서 인접 값보다 실행 시간이 긴 것을 확인했습니다.
