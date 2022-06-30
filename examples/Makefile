


default: general_test

general_test: general_test.c ../confparse.h
	gcc -Wall general_test.c -o test -DDEBUG
	./test

cpp_test: cpp_test.cpp ../confparse.h
	g++ cpp_test.cpp -o cpp_test
	./cpp_test

clean:
	rm -f test
