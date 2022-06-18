#!/usr/bin/python3
import requests
import os
import time
import sys

os.chdir('/mnt/h3site')
r = requests.get('http://192.168.178.128/data')
if (r.status_code != 200):
        print("ERROR")
        print(r.status_code)
        sys.exit(99)
json = r.json()
logfile = open("keller_log.csv", "a")
logfile.write(str(int(time.time())))
logfile.write(",")
logfile.write(str(json["temp"]))
logfile.write(",")
logfile.write(str(json["hum"]))
logfile.write(",")
logfile.write(str(json["leak_raw"]))
logfile.write("\n")
logfile.close()