# [C] Simple DNS Server
DNS Server using 3 processes and Multithreading (dynamic thread pool). 

Whitelisted domains and server configuration in text files.

Project made for Operative Systems course on CS BSc. Programmed in C and written functions and comments in Portuguese.

# Requirements
* Unix terminal
* dig cli tool installed

# Configuration

* `localdns.txt`:
Provides local addresses and its' ips.

Example:
```
mywebserver.so.local 192.168.1.20
fileserver.so.local 192.168.1.10
ww5.so.local 192.168.1.5
ww6.so.local 192.168.1.6
ww7.so.local 192.168.1.7
ww8.so.local 192.168.1.8
```

* `config.txt`: 
Provides number of threads, whitelisted external domains, local domain, and a named pipe name. In order to change
both type of domains while running the server you may send a signal SIGUSR1 to the cfg_process, which PID will be shown occasionaly
as some stats are printed time to time.
Threads treat external requests using dig aswell. Local requests are processed faster.

Example:
```
Threads = 5
Domains = edu.pt; lala.pt; olx.pt; asdasdasdasd.pt; fb.com;
LocalDomain = so.local
NamedPipeEstatisticas = statistics
```


# Running



- Compilation: `$ make`

- Usage: `$ ./main <port>`

- Testing: `$ dig @127.0.0.1 -p53000 www.domain.com`

dig: -p stands for port and @ it's the server address (local machine in this case).





# ISSUES: 
* Multithreading not working as intended.

#
Shared under MIT License.
