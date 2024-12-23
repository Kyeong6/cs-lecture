## 1. 개발환경 세팅

해당 과제를 진행하기 위해서 python과 scrapy를 이용합니다. 

먼저 python의 가상환경을 이용하기 위하여 conda를 사용했습니다.

```bash
conda create -n network python=3.12
```

이후 `scrapy` 를 설치했습니다.

```bash
pip install scapy
```

## 2. TCP Session tracker 진행

![ㅇㄹㅇ](https://github.com/user-attachments/assets/934e38c5-baae-4076-ac73-4e7540a37b2e)

과제를 진행하는 운영체제가 MAC OS이므로 Wi-Fi 인터페이스인 `en0` 에 한정하여 트래킹을 진행하였습니다.

위의 스크린샷을 통해 `ifconfig` 명령어를 통해 `en0`의 status가 `activte`임을 확인할 수 있습니다.

**2.1 패킷 스니핑(00_packet_sniffing.py)**

```python
from scapy.all import sniff, TCP, IP

# Step0: 단순 패킷 스니핑
def packet_handler(packet):
    
    # IP 헤더, TCP 헤더 객체
    ip_layer = packet[IP]
    tcp_layer = packet[TCP]

    # 소스 정보, 목적지 정보 추출
    src_ip = ip_layer.src
    dst_ip = ip_layer.dst
    src_port = tcp_layer.sport
    dst_port = tcp_layer.dport
    
    print(f"TCP Packet: {src_ip}:{src_port} -> {dst_ip}:{dst_port}")

def main():
    # 과제를 진행하는 운영체제가 MAC이므로 Wi-Fi 인터페이스인 en0 사용
    # store=False : 메모리 저장 x
    sniff(iface="en0", prn=packet_handler, store=False)

if __name__ == "__main__":
    main()

```

위의 코드는 Step 0에 관한 내용으로 Scapy 라이브러리의 `sniff()` 함수를 이용하여 MAC OS의 `en0` 인터페이스 패킷을 실시간으로 확인하는 로직입니다.

해당 코드를 통해 스니핑 자체는 가능하지만, “**TCP 세션 트래킹**” 로직은 존재하지 않아 추가적으로 구현을 진행해야합니다.

다음은 00_packet_sniffing.py를 실행한 결과입니다.

![스크린샷 2024-12-23 오후 11 17 26](https://github.com/user-attachments/assets/f5b9c269-5931-405c-aec8-ba46923520f3)

**2.2 TCP 세션 트래킹 로직 구현을 위한 기본 구조(01_tcp_session_basecode.py)**

```python
from scapy.all import sniff, TCP, IP

# 세션 테이블 정의
sessions = {}

# 해당 함수 인자 및 반환값은 세션 키(식별자)로 사용
def make_flow_key(src_ip, src_port, dst_ip, dst_port):

    return (src_ip, src_port, dst_ip, dst_port)

def packet_handler(packet):
    # TCP + IP 계층 패킷 필터링 진행
    if packet.haslayer(IP) and packet.haslayer(TCP):
        # IP 헤더, TCP 헤더 객체
        ip_layer = packet[IP]
        tcp_layer = packet[TCP]

        # 소스 정보, 목적지 정보 추출
        src_ip = ip_layer.src
        dst_ip = ip_layer.dst
        src_port = tcp_layer.sport
        dst_port = tcp_layer.dport

        # 세션 키 만들기
        flow_key = make_flow_key(src_ip, src_port, dst_ip, dst_port)

        # 세션 테이블에 해당 키가 없으면 생성 진행
        if flow_key not in sessions:
            sessions[flow_key] = {
                # 기본값
                "status": "Active",  
                "packets": []
            }

        # 세그먼트 정보 저장 (SEQ, ACK, Flags, Payload ...)
        seq = tcp_layer.seq
        ack = tcp_layer.ack
        flags = tcp_layer.flags
        payload = bytes(tcp_layer.payload)

        # 세션 테이블에 패킷(세그먼트) 기록
        sessions[flow_key]["packets"].append((seq, ack, flags, payload))

        # RST(0x04)/FIN(0x01)이 감지될 경우 'Closed'로 변환
        if (flags & 0x04) or (flags & 0x01):
            sessions[flow_key]["status"] = "Closed"

def main():
    # en0 인터페이스에서 패킷 모니터링
    sniff(iface="en0", prn=packet_handler, store=False)

if __name__ == "__main__":
    main()
```

해당 코드는 2.1의 코드에서 과제의 1,2,3번 요구사항을 구현하기 위한 기본 틀(뼈대)입니다.

TCP + IP 계층을 갖는 패킷을 필터링한 뒤, make_flow_key 함수의 인자 및 반환값인 (src_ip, src_port, dst_ip, dst_port)을 세션 키로 사용하여 `sessions` 딕셔너리에 쌓아둡니다.

이때 IP도 세션 키로 사용하는 이유는 과제의 주요 요구사항인 TCP 세션을 트래킹하기 위해서는 결국 해당 TCP가 어떤 IP에서 다른 IP로 연결된 것인지 알아야하기 때문에 IP 계층 정보도 확인해야하기 때문입니다.

따라서, IP 헤더를 확인하여 출발지·목적지 IP 주소를 추출한 뒤에 TCP 헤더(포트, SEQ, ACK …)를 살펴봐야 세션 식별이 가능합니다.

코드에 관한 설명을 계속 진행하자면, 각 세션에는 `status(key 값)` 와 `packets(key 값)` 를 존재시켜 (seq, ack, flags, playload) 정보를 누적합니다. 

마지막으로 RST(0x04) / FIN(0x01) 플래그를 감지하면 해당 세션을 `status="Closed"` 로 표시합니다. 

RST / FIN은 TCP 세그먼트 필드 안에 CONTROL BIT 또는 FLAG BIT로 정의되어있습니다.

- RST(Reset) : 재연결 종료
    - 재설정을 하는 과정이며, 비정상적인 세션 연결 끊기에 해당한다. 즉, 이 패킷을 보내는 곳이 현재 접속하고 있는 곳과 즉시 연결을 끊고자 할 때 사용한다.
- FIN(Finish) : 연결 종료 요청
    - 세션 연결을 종료시킬 때 사용되며 더 이상 전송할 데이터가 없음을 나타낸다.

해당 코드를 한 문장으로 정리하자면 세션을 “추적”하기 위한 기초를 마련한 코드입니다.

아래는 코드 구동 여부를 위해 세그먼트 정보 저장 후 로그를 확인한 결과입니다.(위의 코드에서는 로그 내용이 주석 처리 되었기 때문에 반영하지 않았습니다.)

![스크린샷 2024-12-23 오후 11 16 26](https://github.com/user-attachments/assets/e0b96f7f-df6b-44ec-b627-cb3fdaf99302)

**2.3 Step01: 활성화된/연결되었던 세션(Flow)들의 나열(02_tcp_session_list.py)**

“활성화된/연결되었던 세션들의 나열”이라는 것은 현재 / 과거에 연결된 모든 TCP 세션(Flow)의 목록을 보여줘야 한다는 것입니다.

2.2에서의 베이스코드에서는 “세션 목록을 출력”하는 로직이 존재하지 않아, 스니핑이 끝난 후 `session` 딕셔너리를 순회하여 세션 별 src_ip, src_port, dst_ip, dst_port와 status를 출력하는 로직을 구현해야 합니다.

아래는 해당 로직을 구현한 코드입니다.

```python
# 세션 리스트 출력을 위한 함수 설정
def print_session_list():
    print("Session list")
    # 세션(session) 딕셔너리 순회하며 설정한 포맷 출력
    for i, (flow_key, info) in enumerate(sessions.items(), start=1):
        (src_ip, src_port, dst_ip, dst_port) = flow_key
        status = info["status"]
        print(f"{i}) {src_ip}:{src_port} -> {dst_ip}:{dst_port} ({status})")
```

로직을 추가하여 2.2의 packet_handler 함수아래에 위치시킨 후 main 함수에 패킷 모니터링 수행 로직 아래에 위치시켰습니다.(02_tcp_session_list.py 참고)

패킷 모니터링이 끝나야지 세션 리스트 출력이 가능하므로, 02_tcp_session_list.py에서는 `timeout` 을 설정하여 해당 시간이 되면 패킷 모니터링이 종료되게 하였습니다.(30초로 설정)

```python
# en0 인터페이스에서 패킷 모니터링(02_tcp_session_list.py 적용)
sniff(iface="en0", prn=packet_handler, store=False, timeout=30)
```

아래는 세션 리스트 출력을 위한 함수 및 timeout을 설정한 뒤 얻은 결과물입니다.

![스크린샷 2024-12-23 오후 11 35 37](https://github.com/user-attachments/assets/923eced6-bdee-4097-a771-e0ad38e2159f)

**2.3 Step02: 해당 플로우의 세그먼트 스트림 나열(03_tcp_session_stream_list.py)**

2.3의 요구사항은 2.2에서 나온 세션 리스트 중에서 특정 번호를 선택하면, 해당 세션이 주고받은 TCP 세그먼트들을 보여주는 것입니다.

```python
# 세션 테이블에 패킷(세그먼트) 기록
sessions[flow_key]["packets"].append((seq, ack, flags, payload))
```

코드에서는 `sessions[flow_key]["packets"]` 리스트에 SEQ, ACK, FLAGS, PAYLOAD 형태로 데이터를 저장하고 있습니다.

TCP Window Size도 확인하기 위해 `tcp_layer.window` 를 추가하고, `sessions[flow_key]["packets"]` 리스트에 추가 저장합니다.

```python
# 세그먼트 정보 저장 (SEQ, ACK, Flags, Payload, Window Size)
seq = tcp_layer.seq
ack = tcp_layer.ack
flags = tcp_layer.flags
payload = bytes(tcp_layer.payload)
win_size = tcp_layer.window

# 세션 테이블에 패킷(세그먼트) 기록
sessions[flow_key]["packets"].append((seq, ack, flags, payload, win_size))
```

Window Size를 추가하였기 때문에 2.3의 요구사항인 “특정 번호를 선택하면, 해당 세션에서 전달될/수신한 세그먼트들을 SEQ로 나열하고 정보를 출력”하는 로직(print_stream 함수)을 구현할 수 있습니다. 

```python
# 특정 세션(Flow) 번호를 입력받아, SEQ 순으로 TCP 세그먼트 정보 출력하는 함수 설정
def print_stream(flow_index):

    # 세션 딕셔너리에서 인덱스별로 Key를 뽑기 위한 list 변환 진행
    keys = list(sessions.keys())
    
    # 범위(flow_index) 예외처리
    if flow_index < 1 or flow_index > len(keys):
        print("flow_index exception")
        return

    # 사용자가 선택한 세션 Key
    flow_key = keys[flow_index - 1]
    segments = sessions[flow_key]["packets"]

    # SEQ 기준 정렬 진행
    segs_sorted = sorted(segments, key=lambda x: x[0])
  
    print(f"\nTCP stream for [{flow_index}] ({flow_key})")
    
    # Flags(byte)를 문자열로 변환
    for i, (seq, ack, flags, payload, win_size) in enumerate(segs_sorted, start=1):
        # Flags
        flag_list = []
        if flags & 0x02: flag_list.append("SYN")
        if flags & 0x10: flag_list.append("ACK")
        if flags & 0x01: flag_list.append("FIN")
        if flags & 0x04: flag_list.append("RST")
        if flags & 0x08: flag_list.append("PSH")
        flag_str = "/".join(flag_list) if flag_list else "NONE"

        # Payload(길이)
        length_payload = len(payload)

        # 출력 형식
        print(f"#{i}\n"
              f"SEQ={seq}\n"
              f"ACK={ack}\n"
              f"FLAGS={flag_str}\n"
              f"WIN={win_size}\n"
              f"LEN={length_payload}"
              )
    print()
```

해당 코드를 설명하자면 사용자가 선택한 세션 Key를 이용해 SEQ 기준으로 정렬한 뒤 Flags의 byte를 문자열로 변환하여 정보를 출력합니다. 정보는 다음과 같습니다.

- SEQ
- ACK
- FLAGS : Flag Type
- WIN : Window Size
- LEN : Payload Length

```python
def main():
    # en0 인터페이스에서 패킷 모니터링
    sniff(iface="en0", prn=packet_handler, store=False, timeout=30)

    # 세션 리스트 출력
    print_session_list()

    # 전체 세션 개수 확인
    total_sessions = len(sessions)
    if total_sessions == 0:
        print("No sessions")
        return

    # 특정 번호 입력받아 스트림 출력
    print(f"\n세션은 1번부터 {total_sessions}번까지 존재합니다.")
    index_str = input("TCP STREAM FOR : ").strip()
    if index_str.isdigit():
        print_stream(int(index_str))
```

`timeout` 을 30초로 설정하였기 때문에 사용자가 어떤 세션까지 패킷 모니터링이 되었는지 모르기 때문에 main 함수에 확인할 수 있는 세션 번호를 로그로 출력하여 범위 안에 있는 세션 번호를 선택할 수 있게 했습니다.  

아래는 03_tcp_session_stream_list.py 실행 결과입니다.

![스크린샷 2024-12-24 오전 12 27 55](https://github.com/user-attachments/assets/110d7784-13b7-429e-ab0c-a2f08f860ce8)

16개까지 패킷 모니터링을 진행하였기 때문에 “세션은 1번부터 16번까지 존재합니다.”라는 문장이 출력되고 3번 세션을 선택하면 이와 관련된 정보가 출력됩니다. 

**2.4 Step03: 해당 플로우의 TCP 세그먼트 재조합하여  데이터 스트림 나열(04_tcp_session_reassemble.py)**

TCP는 스트림 기반 프로토콜이므로, (seq, payload)를 순서대로 이어 붙이면(조립하면) 원래의 데이터를 복원할 수 있습니다. 

요구사항을 확인해보면 “HTTP 사이트 접속”을 권장하였기 때문에 제공된 예시 사이트로 진행하려고 합니다.

- 사용한 예시 사이트 : http://www.testingmcafeesites.com/

```python
# 특정 세션(Flow)의 세그먼트를 재조합하여 전체 바이트 스트림 복원
def reassemble_data(flow_index):
    keys = list(sessions.keys())
    if flow_index < 1 or flow_index > len(keys):
        print("reassmble_data: flow_index exception")
        # 빈 바이트 반환
        return b""  

    flow_key = keys[flow_index - 1]
    segs = sessions[flow_key]["packets"]

    # SEQ 기준 정렬 후 payload 재조합
    segs_sorted = sorted(segs, key=lambda x: x[0])
    # seg[3] = payload
    data = b''.join(seg[3] for seg in segs_sorted)
    return data
```

특정 세션의 세그먼트를 재조합하여 전체 바이트 스트림을 복원하는 함수 reassmble_data를 정의했습니다. 

```python
# 재조합된 스트림(byte)을 텍스트로 변환
def print_reassembled_data(flow_index):
    data = reassemble_data(flow_index)
    if not data:
        print("print_reassembled_data: No data")
        return
    try:
        # HTTP일 경우 utf-8
        text = data.decode('utf-8', errors='replace')
        print(text)
    except UnicodeDecodeError:
        print(data)
    print("=== End of Stream ===\n")
```

reassemble_data 함수를 통해 재조합된 스트림을 텍스트로 변환하여 출력하는 함수 print_reassembled_data 함수를 정의했습니다. 

요구사항인 HTTP를 디코딩하기 때문에 디코딩 방식을 `utf-8` 로 설정했습니다.

2.3에서는 main 함수에서 스트림을 나열했다면, 2.4에서는 범위 내의 패킷의 데이터 스트림을 나열하는 것으로 변경하였습니다.

```python
def main():
    # en0 인터페이스에서 패킷 모니터링
    sniff(iface="en0", prn=packet_handler, store=False, timeout=10)

    # 세션 리스트 출력
    print_session_list()

    # 전체 세션 개수 확인
    total_sessions = len(sessions)
    if total_sessions == 0:
        print("No sessions")
        return

    # 특정 번호 입력받아 스트림 출력
    print(f"\n세션은 1번부터 {total_sessions}번까지 존재합니다.")
    index_str = input("DATA STREAM FOR : ").strip()
    if index_str.isdigit():
        # 데이터 재조합 
        flow_index = int(index_str)
        print_reassembled_data(flow_index)
```

아래는 실행 결과(04_tcp_session_reassemble.py)입니다.

![스크린샷 2024-12-24 오전 1 15 34](https://github.com/user-attachments/assets/6b4236b7-41c3-4fc1-803b-a80f2bbef0b6)

**2.5 모든 요구사항을 수행하는 스크립트 : 05_tcp_session_tracker.py**

해당 스크립트를 실행하면 2.1 ~ 2.4의 기능을 모두 수행하는 `TCP Session Tracker` 도구 구현이 가능합니다. 

2.3과 2.4에서의 변경 사항은 2.5는 두 개 모두 수행하며, 특정 flow를 선택하면 세그먼트 스트림을 나열한 후 재조합한 데이터 스트림 나열 출력 여부를 물어보고 난뒤 `y` 를 선택하면 출력하게 설정하였습니다. 

이와 관련된 결과(05_tcp_session_tracker.py)는 다음과 같습니다.

![스크린샷 2024-12-24 오전 1 30 50](https://github.com/user-attachments/assets/d6c814ad-0bd1-42c4-b173-ed1b1211cc9d)