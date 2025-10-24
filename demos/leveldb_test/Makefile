LEVELDB = leveldb
LEVELDB_URL = <BENCHMARK_URL>


leveldb:
	wget ${LEVELDB_URL} -O leveldb.tar.gz
	tar xvf leveldb.tar.gz
	cd leveldb*/leveldb*/
	mkdir -p build && cd build
	cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .


compile:
	gcc leveldb_test.c -o leveldb_test -lleveldb -lpthread -fsanitize=thread

run:
	./../../build/lotto record leveldb_test > output/record.txt
	./../../build/lotto replay > output/replay.txt
	diff  output/record.txt output/replay.txt

clean:
	rm -rf leveldb*/
	rm -rf output/
	rm leveldb.tar.gz
