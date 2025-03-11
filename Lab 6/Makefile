all:
	gcc -o cook cook.c
	gcc -o waiter waiter.c
	gcc -o customer customer.c
run:
	./cook &
	./waiter &
	./customer
db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

clean:
	rm -f cook waiter customer gencustomers customers.txt
