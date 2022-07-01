a : *.o
	g++ -g *.o -o a

*.o : *.cpp
	g++ -c *.cpp -o *.o

.PHONY : clean

clean : 
	rm -f *.o
