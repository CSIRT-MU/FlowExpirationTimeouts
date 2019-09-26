#!/usr/bin/env python3

from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument("-f", "--file", dest="filename", required=True,
                    help="input CSV file", metavar="input_file")
parser.add_argument("-i", "--inactive", dest="inactive", metavar="timeout",
                    help="inactive timeout in seconds", type=int, default=-1)
parser.add_argument("-a", "--active", dest="active", metavar="timeout",
                    help="active timeout in seconds", type=int, default=-1)
parser.add_argument("-s", "--summary", dest="summary",
                    help="print only total number of flows", action='store_true')
args = parser.parse_args()

active = args.active * 1000000
inactive = args.inactive * 1000000
total = 0

with open(args.filename, 'r') as ifile:
    for line in ifile:
        fields = line.split(";")
        start = int(fields[1])
        last = start
        packets = 1
        current = 0
        for current in (int(x) for x in fields[2:]):
            # check for inactive timeout first
            if inactive > 0 and last + inactive < current:
                if not args.summary:
                    print(fields[0], ";", last - start, ";", packets, ";I", sep='')
                total +=1
                start = current
                last = start
                packets = 1
                continue

            # check for active timeout
            if active > 0 and start + active < current:
                if not args.summary:
                    print(fields[0], ";", last - start, ";", packets, ";A", sep='')
                total += 1
                start = current
                last = start
                packets = 1
                continue

            packets += 1
            last = current

        # always print what is left (at least one packet will be there)
        if not args.summary:
            print(fields[0], ";", last - start, ";", packets, ";E", sep='')
        total += 1

if args.summary:
    print(total)