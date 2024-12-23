from scapy.all import sniff, TCP, IP

# 단순 패킷 스니핑
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
