#include "connection.h"
#include <iostream>

int main(int argc, char* argv[]) {
	try {
		if (argc == 3) {
			zabroda::TcpConnection con{ std::cin, std::cout, argv[1], argv[2] }; // tcp echo server
			con.start_protocol();
		}
		else throw std::exception("Usage: Connections <hostname> <port>");
	}
	catch (std::exception& e) {
		std::cerr << e.what();
	}
}