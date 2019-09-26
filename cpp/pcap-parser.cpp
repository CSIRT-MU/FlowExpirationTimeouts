#include <iostream>
#include <tins/tins.h>
#include <fstream>
#include <lzma.h>
#include <unordered_map>

using namespace Tins;

std::unordered_map<uint64_t, std::vector<uint64_t>> icmp_map;
std::unordered_map<uint64_t, std::vector<uint64_t>> udp_map;
std::unordered_map<uint64_t, std::vector<uint64_t>> tcp_map;

uint64_t counter = 0;

uint64_t get_flow_id(uint32_t sip, uint32_t dip, uint16_t sport = 0, uint16_t dport = 0) {
  uint8_t id[12] = {0};
  memcpy(id + 0, &sip, 4);
  memcpy(id + 4, &dip, 4);
  memcpy(id + 8, &sport, 2);
  memcpy(id + 10, &dport, 2);
  return lzma_crc64(id,12,0);
}

bool process_packets(const Packet &packet) {
  counter++;
  if (counter % 100000000 == 0) std::cout << counter <<std::endl;

  auto const pdu = packet.pdu();
  auto const ip = pdu->find_pdu<IP>();
  if (ip) {
    auto const tcp = ip->find_pdu<TCP>();
    if (tcp) {
      uint64_t key = get_flow_id(ip->src_addr(), ip->dst_addr(), tcp->sport(), tcp->dport());
      tcp_map[key].push_back(packet.timestamp().seconds() * 1000000 + packet.timestamp().microseconds());
      return true;
    }

    auto const udp = ip->find_pdu<UDP>();
    if (udp) {
      uint64_t key = get_flow_id(ip->src_addr(), ip->dst_addr(), udp->sport(), udp->dport());
      udp_map[key].push_back(packet.timestamp().seconds() * 1000000 + packet.timestamp().microseconds());
      return true;
    }

    auto const icmp = ip->find_pdu<ICMP>();
    if (icmp) {
      uint64_t key = get_flow_id(ip->src_addr(), ip->dst_addr());
      icmp_map[key].push_back(packet.timestamp().seconds() * 1000000 + packet.timestamp().microseconds());
    }
  }
  return true;
}

void print_map(std::unordered_map<uint64_t, std::vector<uint64_t>> &map, std::ofstream &file) {
  for (auto const &i: map) {
    file << i.first;
    for (auto const &t: i.second) {
      file << ";" << t;
    }
    file << '\n';
  }

  file.close();
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cerr << "Give me a pcap file to work with!\n";
    return 1;
  }

  try {
    FileSniffer sniffer(argv[1]);
    sniffer.sniff_loop(process_packets);
  } catch (exception_base &e) {
    std::cerr << "Cannot process given pcap file: " << e.what() << std::endl;
    return 2;
  }

  std::ofstream file_tcp("tcp.times", std::ofstream::out);
  std::ofstream file_udp("udp.times", std::ofstream::out);
  std::ofstream file_icmp("icmp.times", std::ofstream::out);

  print_map(icmp_map, file_icmp);
  print_map(udp_map, file_udp);
  print_map(tcp_map, file_tcp);

  return 0;
}