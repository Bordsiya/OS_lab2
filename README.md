# OS_lab2
Operation Systems, ITMO, 3 year

1) make clean
2) make
3) sudo insmod driver.ko
4) gcc user.c -o user
5) ./user [args]
6) pid search: lsof | grep "mem" | head
7) unregister module: sudo rmmod driver.ko
8) check journal: dmesg
