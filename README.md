# myip
Simple utility for getting our IPv4 address

Compile:
```
g++ -o myip main.cpp -lcurl
```

Using:
```
myip [OPNIONS]
myip - A mini utility for getting your ip address

Options:
        -l/--local              - print local IP address
        -g/--global             - print global IP address and location
        -a/--all                - print local and global IP addresses and location (default)
        -h/--help               - print this message
```

