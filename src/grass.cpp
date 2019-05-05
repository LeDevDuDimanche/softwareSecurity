#include <grass.hpp>

#include <stdio.h>
#include <iostream>


// hijack_flow() must remain unchanged as per the TAs.
void hijack_flow(){
	printf("Method hijack: Accepted\n");
}

// For automated exploits, it's good to flush
void hijack_flow_wrapper(){
    hijack_flow();
    std::cout << std::flush;
}
