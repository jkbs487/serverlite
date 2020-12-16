#!/usr/bin/python3

import os

files = os.listdir('cpp')
for file in files:
    os.system('./scripts/readme.sh ' + 'cpp/'+file)
