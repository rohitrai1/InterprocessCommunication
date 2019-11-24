compile : 
	sudo g++ -o berkley berkley.cpp -lpthread -std=c++11
	sudo g++ -o causal_ordering causal_ordering.cpp -lpthread -std=c++11
	sudo g++ -o non_causal_ordering non_causal_ordering.cpp -lpthread -std=c++11	

clean :
	sudo rm berkley causal_ordering non_causal_ordering locking
