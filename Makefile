main:
	g++ PizzaFastCGI.cpp -lmongoclient -lboost_thread -lboost_system -lboost_regex -lboost_filesystem -lboost_program_options -pthread  -O2 -fPIC -lfastcgi-daemon2 -shared -o libPizzaFastCGI.so 
