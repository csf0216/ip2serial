#!/usr/bin/python

import sys,time,threading
from socket import *

class RecvThread(threading.Thread):
    def __init__(self,sock,fdOutput):
        threading.Thread.__init__(self)
        self.sock = sock
        self.fd = fdOutput
        self.stopped = False
    def run(self):
        while not self.stopped:
            buf = self.sock.recv(200)
            self.fd.write(buf)
    def stop(self):
        self.stopped = True

def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    fdInput = open(sys.argv[3], "r+")
    fdOutput = open(sys.argv[3]+".bak", "w+")
    s = socket(AF_INET,SOCK_STREAM)
    s.connect((host,port))
    rxThread = RecvThread(s,fdOutput)
    rxThread.start()
    for line in fdInput.readlines():
        s.send(line)
        print len(line)
    time.sleep(2)
    rxThread.stop()
    rxThread.join()
    s.close()
    fdInput.close()
    fdOutput.close()

if __name__ == "__main__":
    main()
