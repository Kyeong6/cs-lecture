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

        # 로그 확인
        print(f"[PACKET] {src_ip}:{src_port} -> {dst_ip}:{dst_port} | SEQ={seq} ACK={ack} FLAGS={flags}")

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
