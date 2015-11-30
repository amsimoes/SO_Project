# [C] Simple DNS Server
DNS Server using 3 processes and a thread pool. Whitelisted domains in text files.

- Compilation: $ make 

- Usage: $ ./main <port>

- Testing: $ dig @127.0.0.1 -p53000 www.domain.com

dig: -p stands for port and @ it's the server address (local machine in this case).

localdns.txt provides local addresses and its ips.
config.txt provides number of threads, whitelisted external domains, local domain, and a named pipe name. In order to change
both type of domains while running the server you may send a signal SIGUSR1 to the cfg_process, which PID will be shown occasionaly
as some stats printing time to time.
Threads treat external requests using dig aswell. Local requests are processed faster.

Programmed in C and written functions and comments in Portuguese.

#
Shared under MIT License.
