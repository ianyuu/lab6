# Make file

net367: host.o packet.o man.o main.o net.o switch.o dns.o
	gcc -o net367 host.o man.o main.o net.o packet.o switch.o dns.o

main.o: main.c
	gcc -c main.c

switch.o: switch.c
	gcc -c switch.c

dns.o: dns.c
	gcc -c dns.c

host.o: host.c 
	gcc -c host.c  

man.o:  man.c
	gcc -c man.c

net.o:  net.c
	gcc -c net.c

packet.o:  packet.c
	gcc -c packet.c

clean:
	rm *.o

