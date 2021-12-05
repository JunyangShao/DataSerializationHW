from random import random
import sys

type1 = """  <RECORD>
    <OPCODE>OP_START_LOG_SEGMENT</OPCODE>
    <DATA>
      <TXID>{txid}</TXID>
    </DATA>
  </RECORD>
"""

type2 = """  <RECORD>
    <OPCODE>OP_MKDIR</OPCODE>
    <DATA>
      <TXID>{txid}</TXID>
      <LENGTH>0</LENGTH>
      <INODEID>{inodeid}</INODEID>
      <PATH>/user/history/done</PATH>
      <TIMESTAMP>{timestamp}</TIMESTAMP>
      <PERMISSION_STATUS>
        <USERNAME>mapred</USERNAME>
        <GROUPNAME>hadoop</GROUPNAME>
        <MODE>504</MODE>
      </PERMISSION_STATUS>
    </DATA>
  </RECORD>
"""


type3 = """  <RECORD>
    <OPCODE>OP_SET_PERMISSIONS</OPCODE>
    <DATA>
      <TXID>{txid}</TXID>
      <SRC>/user/history/done_intermediate</SRC>
      <MODE>1023</MODE>
    </DATA>
  </RECORD>
"""


type4 = """  <RECORD>
    <OPCODE>OP_END_LOG_SEGMENT</OPCODE>
    <DATA>
      <TXID>{txid}</TXID>
    </DATA>
  </RECORD>
"""

NO_OF_LOGS = int(sys.argv[1])
logs_type = sys.argv[2]
xmlfilename = "editlogs_" + str(NO_OF_LOGS) + "_" + logs_type + ".xml"

inodeid = 100000
timestamp = 10000000000000
txid = 10000000000

editLogFile = open(xmlfilename, "w")

editLogFile.write("""<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<EDITS>
  <EDITS_VERSION>-66</EDITS_VERSION>
""")


if logs_type == "plain": #contains only start and end log statements
  for i in range(NO_OF_LOGS):
    log_type = i%2 + 1
    
    if log_type==1:
      log = type1.format(txid=int(random()*txid))
    else:
      log = type4.format(txid=int(random()*txid))
    editLogFile.write(log)
elif logs_type == "moderate": #do not include OP_MKDIR
  for i in range(NO_OF_LOGS):
    log_type = i%3 + 1
    
    if log_type==1:
      log = type1.format(txid=int(random()*txid))
    elif log_type==2:
      log = type3.format(txid=int(random()*txid))
    else:
      log = type4.format(txid=int(random()*txid))
    editLogFile.write(log)

else:
  for i in range(NO_OF_LOGS):
    log_type = i%4 + 1
    
    if log_type==1:
      log = type1.format(txid=int(random()*txid))
    elif log_type==2:
      log = type2.format(txid=int(random()*txid), inodeid=int(random()*inodeid), timestamp=int(random()*timestamp))
    elif log_type==3:
      log = type3.format(txid=int(random()*txid))
    else:
      log = type4.format(txid=int(random()*txid))
    editLogFile.write(log)


editLogFile.write("""</EDITS>""")
editLogFile.close()

'''
from subprocess import call
from time import time

filename =  xmlfilename.replace(".xml", "")

#serialize
x = time()
call("hdfs oev -p binary -i " + xmlfilename + " -o " + filename, shell=True)
print("time taken to deserialize : " + str(round(time()-x,3)))

x = time()


#deserialize
call("hdfs oev -i " + filename + " -o " + xmlfilename, shell=True)
print("time taken to deserialize : " + str(round(time()-x,3)))



#calculate compression
import os

xmlsize = os.path.getsize(xmlfilename)
binsize = os.path.getsize(filename)

compression = (xmlsize-binsize)*100/(xmlsize)

print("compression: " + str(round(compression,3)) + "%")

'''

#inodeid  100000
#timestamp 10000000000000
#txid 10000000000

'''

Permission_status = {
    username many int8 | ![0]
    groupname many int8 | ![0]
    mode unit16
}
Data = {
    txid unit64
    optional length unit64
    optional inodeid unit64
    optional path many int8 | ![0]
    optional src many int8 | ![0]
    optional timestamp unit64
    optional permission_status Permission_status
}



Record = {
    opcode many unit8 | ![0]
}

    many Record



permission_status = {
    username many alnum
    groupname many alnum
    mode unit16
}

Data = {
    txid unit64
    optional length unit64
    optional inodeid unit64
    optional path many int8 | ![0]
    optional src many int8 | ![0]
    optional timestamp unit64
    optional permission_status permission_status
}

Record = {
    opcode many int8 | ![0]
    data Data
}

edits = {
    edits_version int16
    record many Record
}

# Lowercase names
permission_status = {
    username many int8 | ![0]
    groupname many int8 | ![0]
    mode unit16
}

data = {
    txid unit64
    optional length unit64
    optional inodeid unit64
    optional path many int8 | ![0]
    optional src many int8 | ![0]
    optional timestamp unit64
    optional permission_status permission_status
}

record = {
    opcode many int8 | ![0]
    data data
}

edits = {
    edits_version int16
    record many record
}




'''
