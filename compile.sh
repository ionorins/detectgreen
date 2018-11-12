g++ -Iinclude/ -Isrc/ -I/usr/local/cuda-9.1/include/ -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -fopenmp -Ofast -DGPU -DCUDNN detect.cpp include/libdarknet.a -o detect -lm -pthread -L/usr/local/cuda-9.1/lib64 -lcuda -lcudart -lcublas -lcurand -lcudnn -lstdc++ -lzmq `pkg-config --cflags --libs opencv`

