# Flow Expiration Timeouts

This repository contains tools for computing flow records from pcap files. The flows are computed using different timeouts in order to analyse the impact of the timeouts on the flow creation process.

There are two implementations of the flow computation that produce similar output. A simple python scripts, which are usable for small dataset and a C++ implementation that has high performance.

## Python implementation
The python implementation is very simple. It uses tshark (https://www.wireshark.org/docs/man-pages/tshark.html) to parse pcaps.

These examples create three different colon separated files where the first fields are flow keys and the rest are packet arrival times:
```
tshark -r example.pcap -Tfields -e ip.src -e ip.dst -e frame.time_epoch -E 'occurrence=f' 'icmp and ip.version == 4' | ./group-packets.py > icmp.times &
tshark -r example.pcap -Tfields -e ip.src -e ip.dst -e tcp.srcport -e tcp.dstport -e frame.time_epoch -E 'occurrence=f' 'tcp and !icmp and ip.version == 4' | ./group-packets.py > tcp.times &
tshark -r example.pcap -Tfields -e ip.src -e ip.dst -e udp.srcport -e udp.dstport -e frame.time_epoch -E 'occurrence=f' 'udp and !icmp and ip.version == 4' | ./group-packets.py > udp.times &
```

When the data is prepared in this way, the `compute-flows.py` script can be used to compute flows. 

This example lists all connections, their lengths, and numbers of packets, because settings timeouts to 0 makes the script to ignore them:
```
./compute-flows.py -f tcp.times -i 0 -a 0
```

The output format is `flow_id;length in microseconds;number of packets;flow expiration reason`. The flow expiration reason can be `E` for natural connection end, `I` for inactive timeout, and `A` for active timeout.

When only a number of created flows is required, `-s` option can be used. Following example gives number of flows when active timeout is set to 300 seconds and inactive timeout to 30 seconds:
```
./compute-flows.py -f tcp.times -i 30 -a 300 -s
```

Feeding large pcaps to tshark can cause the computer to run out of memory. To process large pcaps, use the C++ implementation.

## C++ implementation

The C++ implementation has two parts just as the python one. The `pcap_parser` takes a single pcap file as an argument and produces three files: `times.tcp`, `times.udp`, and `times.icmp`, which is similar to running all three tshark commands above. The difference is that the flow keys (first columns of `*.times` files) are 64bit lzma hashes instead of strings.
```
./compute_flows example.pcap
```

The `compute_flows` program takes an the same input as the python `compute-flows.py`, which is a file with packet times for each flow (output of the `pcap_parse`) and creates following output files:
- `connections.txt`: list of found connections in the format `flow_hash;length in microseconds;number of packets;E`
- `active.txt`: number of flows for active timeouts 0..600 and no inactive timeout
- `inactive.txt`: number of flows for inactive timeouts 0..600 and no active timeout
- `timeouts.txt`: number of flows for active timeouts 0..600 and inactive timeouts 0..active timeout
- `packet-gaps.txt`: list of gaps between packets in microseconds

### Building the C++ code

Dependencies:
- libtins (https://github.com/mfontanini/libtins/), release >= 4.2
- xz-devel (lzma.h)
- cmake

Building:
```
cd cpp
mkdir build && cd build
cmake ..
make
```

## Analysing the data using gnuplot

The data is best analyzed using plots. Here are several examples for using gnuplot for this task.


Plot for number of connections using active and inactive timeout separately:
```
gnuplot
set term qt size 1600,1024
set datafile separator ";"
set xtics 0,10,600
set grid
set xlabel "Timeout"
set ylabel "# of flows"
plot "active.txt" with line, "inactive.txt" with line
```

Compute and plot histogram of connection lengths using 5 second bins:
```
cat connections.txt| cut -d ';' -f 2 | awk '//{print int($1/5000000)}' | sort -n | uniq -c | awk '//{print $2" "$1}' > histogram.txt

gnuplot
set term qt size 1600,1024
#set logscale y
set grid
set xlabel "Length of connections"
set ylabel "# of connections"
#set xrange [10:2000]
plot "histogram.txt" using (5*$1):2 with lines
```

Create 3D graph for number of flows based on active and inactive timeout:
```
# convert data for gnuplot
for i in {0..600}; do for j in $(seq $((i+1)) 600); do echo "$i;$j;?"; done; done > undefined.txt
cat timeouts.txt undefined.txt | sort -n -n -t ";" -k 1 -k 2 -n | awk -F ";" 'BEGIN{pr=1} //{if ($1!=pr){print ""} pr=$1; print $0}' > 3d.txt

gnuplot
set term qt size 1600,1024
set datafile separator ";"
set xlabel "Active timeout"
set ylabel "Inactive timeout"
set zlabel "# of flows"
splot "3d.txt" with pm3d
```

Create a heatmap for number of flows based on active and inactive timeout:
```
gnuplot
set term qt size 1600,1024
set datafile separator ";"
set xlabel "Active timeout"
set ylabel "Inactive timeout"
set zlabel "# of flows"
set pm3d map
splot "3d.txt" with pm3d
```
