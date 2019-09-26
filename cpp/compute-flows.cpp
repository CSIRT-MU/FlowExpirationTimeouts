#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>

bool read_flows_from_file(char *filename, std::unordered_map<uint64_t, std::vector<uint64_t>> &flow_map) {
  std::ifstream in(filename, std::ifstream::in);
  if (in.fail()) {
    std::cout << "Cannot open file '" << filename << "'" << std::endl;
    return false;
  }

  std::string flow;
  char *pos;
  uint64_t flow_hash, timestamp, num;
  // Read a flow from each line
  while (std::getline(in, flow))
  {
    if(flow.size() > 0) {
      // First comes flow hash
      flow_hash = strtoull(flow.c_str(), &pos, 10);
      std::vector<uint64_t> &flow_vector = flow_map[flow_hash];
      pos++;
      num = pos - flow.c_str();
      while (num < flow.size()) {
        // Now read the timestamps
        timestamp = strtoull(pos, &pos, 10);
        flow_vector.push_back(timestamp);
        pos++;
        num = pos - flow.c_str();
      }
    }
  }

  in.close();

  return true;
}

uint64_t compute_flows(std::unordered_map<uint64_t, std::vector<uint64_t>> flow_map, uint16_t active_s, uint16_t inactive_s, bool summary, std::ostream &of = std::cout) {
  uint64_t packets, start, last, total = 0, active = active_s * 1000000, inactive = inactive_s * 1000000;

  for (auto const &flow: flow_map) {
    packets = 0;
    for (auto const &current: flow.second) {

      // First packet of a flow
      if (packets == 0) {
        start = current;
        last = start;
      }

      // Check for inactive timeout first
      if (inactive > 0 && last + inactive < current) {
        if (!summary) {
          of << flow.first << ";" << last - start << ";" << packets << ";I\n";
        }
        total++;
        start = current;
        last = start;
        packets = 1;
        continue;
      }

      // Check for active timeout
      if (active > 0 && start + active < current) {
        if (!summary) {
          of << flow.first << ";" << last - start << ";" << packets << ";A\n";
        }
        total++;
        start = current;
        last = start;
        packets = 1;
        continue;
      }

      packets++;
      last = current;
    }

    // Print what is left in the flow
    if (packets > 0) {
      if (!summary) {
        of << flow.first << ";" << last - start << ";" << packets << ";E\n";
      }
      total++;
    }
  }

  return total;
}

void compute_interpacket_gaps(std::unordered_map<uint64_t, std::vector<uint64_t>> flow_map, std::ostream &of = std::cout) {
  uint64_t last, current;

  for (auto const &flow: flow_map) {
    last = flow.second.front();
    for (auto it = ++flow.second.begin(); it != flow.second.end(); it++) {
      current = *it;
      if (current >= last) {
        of << current - last << '\n';
      }
      last = current;
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cerr << "Give me file with flow packet times to work with!\n";
    return 1;
  }

  std::unordered_map<uint64_t, std::vector<uint64_t>> flow_map;

  if (!read_flows_from_file(argv[1], flow_map)) return 2;

  // Now we have everything in memory, let's compute some flows

  // connections without timeouts
  std::ofstream file_connections("connections.txt", std::ofstream::out);
  compute_flows(flow_map, 0, 0, false, file_connections);
  file_connections.close();

  // active timeout only
  std::cerr << "Computing active timeout 1..600\n";
  std::ofstream file_active("active.txt", std::ofstream::out);
  for (int a = 0; a <= 600; a++) {
    if (a % 10 == 0) std::cerr << a << " ";
    file_active << a << ";" << compute_flows(flow_map, a, 0, true) << '\n';
  }
  std::cerr << "\n\n";
  file_active.close();

  // inactive timeout only
  std::ofstream file_inactive("inactive.txt", std::ofstream::out);
  std::cerr << "Computing inactive timeout 1..600\n";
  for (int i = 0; i <= 600; i++) {
    if (i % 10 == 0) std::cerr << i << " ";
    file_inactive << i << ";" << compute_flows(flow_map, 0, i, true) << '\n';
  }
  std::cerr << "\n\n";
  file_inactive.close();

  // Combination of timeouts
  std::ofstream file_timeouts("timeouts.txt", std::ofstream::out);
  std::cerr << "Computing timeouts active 1..600, inactive 1..active";
  for (int a = 0; a <= 600; a+=1) {
    if (a % 10 == 0) std::cerr << "\nactive: " << a << ": ";
    for (int i = 0; i <= a; i+=1) {
      if (a % 10 == 0 && i % 10 == 0) std::cerr << i << " ";
      file_timeouts << a << ";" << i << ";" << compute_flows(flow_map, a, i, true) << '\n';
    }
  }
  std::cerr << "\n\n";
  file_timeouts.close();

  // Interpacket gaps
  std::ofstream file_gaps("packet-gaps.txt", std::ofstream::out);
  compute_interpacket_gaps(flow_map, file_gaps);
  file_gaps.close();

  return 0;
}