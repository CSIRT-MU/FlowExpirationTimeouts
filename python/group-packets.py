#!/usr/bin/env python3

# The script takes input in form of tshark output
# -Tfields -e ip.src -e ip.dst -e tcp.srcport -e tcp.dstport -e _ws.col.Time

import sys
from collections import defaultdict

flows = defaultdict(list)

for line in sys.stdin:
    fields = line.split()
    key = "-".join(fields[:-1])
    value = [str(int(float(fields[-1])*1000000))]
    flows[key].append(value)

for k, v in flows.items():
    print(k, ";", ";".join("_".join(x) for x in v), sep='')
