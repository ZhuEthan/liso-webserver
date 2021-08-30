#!/usr/bin/python

from socket import *
import sys
import random
import os
import time

if len(sys.argv) < 6:
    sys.stderr.write('Usage: %s <ip> <port> <#trials>\
            <#writes and reads per trial>\
            <#connections> \n' % (sys.argv[0]))
    sys.exit(1)

serverHost = gethostbyname(sys.argv[1])
serverPort = int(sys.argv[2])
numTrials = int(sys.argv[3])
numWritesReads = int(sys.argv[4])
numConnections = int(sys.argv[5])

if numConnections < numWritesReads:
    sys.stderr.write('<#connections> should be greater than or equal to <#writes and reads per trial>\n')
    sys.exit(1)

socketList = []

RECV_TOTAL_TIMEOUT = 0.1
RECV_EACH_TIMEOUT = 0.01

for i in xrange(numConnections):
    s = socket(AF_INET, SOCK_STREAM)
    s.connect((serverHost, serverPort))
    socketList.append(s)

#'GET / HTTP/1.1\r\nHost: 127.0.0.1:9999\r\nConnection: keep-alive\r\nCache-Control: max-age=0\r\n\r\nmessage'
GOOD_REQUESTS = [
    'GET /text.txt HTTP/1.1\r\n\r\n',
    'GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\nContent-Length: 8\r\n\r\nmessage\nGET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\nContent-Length: 8\r\n\r\nmessage\n', # fo the message, we should use readn instead of readlinen
    'GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\nContent-Length: 8\r\n\r\nmessage\n', # fo the message, we should use readn instead of readlinen
    'GET / HTTP/1.1\r\n\r\n'
]
BAD_REQUESTS = [
    'GET\r / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n', # Extra CR
    'GET / HTTP/1.1\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n',     # Missing CR
    'GET / HTTP/1.1\rUser-Agent: 441UserAgent/1.0.0\r\n\r\n',     # Missing LF
]

BAD_REQUEST_RESPONSE = 'HTTP/1.1 400 Bad Request\r\n\r\n'

for i in xrange(numTrials):
    socketSubset = []
    randomData = []
    randomLen = []
    socketSubset = random.sample(socketList, numConnections)
    random_string = ""
    for j in xrange(numWritesReads):
        random_index = 0 #random.randrange(len(GOOD_REQUESTS) + len(BAD_REQUESTS))
        print("random_index")
        print(random_index)
        if random_index < len(GOOD_REQUESTS):
            random_string = GOOD_REQUESTS[random_index]
            randomLen.append(len(random_string))
            randomData.append(random_string)
        else:
            random_string = BAD_REQUESTS[random_index - len(GOOD_REQUESTS)]
            randomLen.append(len(BAD_REQUEST_RESPONSE))
            randomData.append(BAD_REQUEST_RESPONSE)
        socketSubset[j].send(random_string)
        #socketSubset[j].send(random_string)

    for j in xrange(numWritesReads):
        data = socketSubset[j].recv(100)#randomLen[j])
        print("receive data:\n")
        print(data)
        #start_time = time.time()
        #while True:
            #print("sent data: ")
            #print(random_string)
            #print("expected data with len ", randomLen[j])
            #print(randomData[j])
            #if len(data) == randomLen[j]:
                #break
            #socketSubset[j].settimeout(RECV_EACH_TIMEOUT)
            #data += socketSubset[j].recv(randomLen[j])
            #print("receive data:\n")
            #print(data)
            #if time.time() - start_time > RECV_TOTAL_TIMEOUT:
                #break
        #if data != randomData[j]:
            #print("rece data: ")
            #print(data)
            #sys.stderr.write("Error: Data received is not the same as sent! \n")
            #sys.exit(1)

for i in xrange(numConnections):
    socketList[i].close()

print("Success!")
