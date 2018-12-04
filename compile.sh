alias g++=/usr/local/Cellar/gcc/8.2.0/bin/g++-8
alias gcc=/usr/local/Cellar/gcc/8.2.0/bin/gcc-8
g++ -I`g++ -print-file-name=plugin`/include -std=c++11 -g -Wall -fno-rtti -Wno-literal-suffix -fPIC -c -o plugin1.o plugin_debugger.c
g++ -std=c++11 -dynamiclib -undefined dynamic_lookup -g -o plugin1.so plugin1.o
gcc -fplugin=./plugin1.so -fplugin-arg-plugin1-port=14857 -O0 -fdump-tree-gimple plugin1_test.c -o plugin1_test.o
# gcc -fplugin=./plugin1.so -fplugin-arg-plugin1-port=14857 -O0 -S plugin1_test.c
