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
Provides thread pool size, whitelisted external Domains addresses, LocalDomain address, and a statistics named pipe name. 
Live Reloading: It's possible to update both domain types list, while running the server, by sending a SIGUSR1 signal to the cfg_process PID (which is shown occasionally on the server cli window or by locating it on your system) after updating and saving the file.
Threads handle external requests using dig aswell but local requests are processed faster.

Example:
```
Threads = 5
Domains = edu.pt; lala.pt; olx.pt; google.com; fb.com;
LocalDomain = so.local
NamedPipeEstatisticas = statistics
```


# Running

* Compilation: `$ make`

* Usage: `$ ./main <port>`

* Testing: `$ dig @127.0.0.1 -p53000 www.domain.com`

dig usage: -p stands for port and @ it's the server address (always localhost in this project).


# Issues
* Multithreading not working as intended.

#
Shared under MIT License.
