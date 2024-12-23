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

        # 세그먼트 정보 저장 (SEQ, ACK, Flags, Payload, Window Size)
        seq = tcp_layer.seq
        ack = tcp_layer.ack
        flags = tcp_layer.flags
        payload = bytes(tcp_layer.payload)
        win_size = tcp_layer.window

        # # 로그 확인
        # print(f"[PACKET] {src_ip}:{src_port} -> {dst_ip}:{dst_port} | SEQ={seq} ACK={ack} FLAGS={flags}")

        # 세션 테이블에 패킷(세그먼트) 기록
        sessions[flow_key]["packets"].append((seq, ack, flags, payload, win_size))

        # RST(0x04)/FIN(0x01)이 감지될 경우 'Closed'로 변환
        if (flags & 0x04) or (flags & 0x01):
            sessions[flow_key]["status"] = "Closed"

# 세션 리스트 출력을 위한 함수 설정
def print_session_list():
    print("Session list")
    # 세션(session) 딕셔너리 순회하며 설정한 포맷 출력
    for i, (flow_key, info) in enumerate(sessions.items(), start=1):
        (src_ip, src_port, dst_ip, dst_port) = flow_key
        status = info["status"]
        print(f"{i}) {src_ip}:{src_port} -> {dst_ip}:{dst_port} ({status})")

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
  
    print(f"\nTCP stream for [{flow_index}]")
    
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

if __name__ == "__main__":
    main()