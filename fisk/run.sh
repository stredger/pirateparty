gcc -c p242pio.c

gcc -c test.c -lpthread
gcc -c p242piotest.c -lpthread

gcc -o ourTest test.o  -lpthread
gcc -o theirTest p242piotest.o -lpthread

echo "===================OUR TEST============================="

./ourTest

echo "===================THEIR TEST============================="

./theirTest
