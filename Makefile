cc = g++

all: server client

log_test:
	${cc} ./src/test/log.cpp -DDEBUG -std=c++11 -I . -o ./bin/log_test -g
log_test_E:
	${cc} ./src/test/log.cpp -DDEBUG -std=c++11 -I . -E > ./bin/1.cpp -g
util_test:
	${cc} ./src/test/util.cpp -DDEBUG -std=c++11 -I . -o ./bin/util_test -g
thread_pool_test:
	${cc} ./src/test/thread_pool.cpp -DDEBUG -lpthread -std=c++11 -I . -o ./bin/thread_pool_test -g
config_test:
	${cc} ./src/test/config.cpp -DDEBUG -std=c++11 -I . -o ./bin/config_test -g
file_test:
	${cc} ./src/fd/*.cpp ./src/test/fd.cpp -DDEBUG -std=c++11 -I . -o ./bin/file_test -g
serverd:
	${cc} ./src/fd/*.cpp ./src/app/server/controller/*.cpp ./src/app/server/main.cpp -lpthread -std=c++11 -I . -DDEBUG -o ./bin/server -g
clientd:
	${cc} ./src/fd/*.cpp ./src/app/client/controller/*.cpp ./src/app/client/main.cpp -lpthread -std=c++11 -I . -DDEBUG -o ./bin/client -g
server:
	${cc} ./src/fd/*.cpp ./src/app/server/controller/*.cpp ./src/app/server/main.cpp -lpthread -std=c++11 -I . -o ./bin/server -g
client:
	${cc} ./src/fd/*.cpp ./src/app/client/controller/*.cpp ./src/app/client/main.cpp -lpthread -std=c++11 -I . -o ./bin/client -g