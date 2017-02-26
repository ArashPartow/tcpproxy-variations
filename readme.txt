C++ TCP Proxy Server Variations

[INTRODUCTION]
The  C++ TCP  Proxy Server  Variations are  a series  of very  simple
variations made upon the baseline version of the C++ TCP Proxy  server
so as  to demonstrate  how one  can easily  add interesting and useful
functionality  to  the  proxy  server and  also  to  provide  a simple
tutorial on the  usage of the  ASIO library. The  variations presented
are as follows:

(1) Multi-threaded I/O service
(2) Limiting of upstream data flow
(3) Logging of upstream and downstream data flows
(4) Limiting the number of concurrent client connections


[COPYRIGHT NOTICE]
Free use of the C++ TCP Proxy Server variations is permitted under the
guidelines and  in accordance  with the MIT License.

http://www.opensource.org/licenses/MIT


[DOWNLOADS & UPDATES]
All updates and the  most recent version of  the C++ TCP Proxy  Server
variations can be found at:
http://www.partow.net/programming/tcpproxy/index.html

Code repository:
https://github.com/ArashPartow/tcpproxy-variations



[COMPILATION]
(1) For a complete build: make clean all
(2) To strip executables: make strip_bin


[COMPILER COMPATIBILITY]
(*) GNU Compiler Collection (4.3+)
(*) Intel® C++ Compiler (9.x+)
(*) Clang/LLVM (1.1+)
(*) Microsoft Visual Studio C++ Compiler (8.1+)


[FILES]
(00) Makefile
(01) readme.txt
(02) tcpproxy_server_01.cpp
(03) tcpproxy_server_02.cpp
(04) tcpproxy_server_03.cpp
(05) tcpproxy_server_04.cpp
