#include "connection.h"
#include <iostream>

int main() {
	try {
		zabroda::TcpConnection con{ std::cin, std::cout, "tcpbin.com", "4242" }; // tcp echo server
		con.start_protocol();
	}
	catch (std::exception& e) {
		std::cerr << e.what();
	}
}