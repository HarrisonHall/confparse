


default: general_test

general_test: general_test.c ../confparse.h
	gcc general_test.c -o test -DDEBUG
	./test

clean:
	rm -f test
