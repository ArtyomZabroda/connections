#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <vector>
namespace zabroda {

	// 
	class AddrinfoResults {
	public:
		class iterator {
		public:
			iterator(addrinfo* ptr) : ptr_{ ptr } {}
			addrinfo operator*() { return *ptr_; }
			iterator& operator++() { ptr_ = ptr_->ai_next; return *this; }
			addrinfo* operator->() { return ptr_; }
			operator addrinfo* () { return ptr_; }
		private:
			addrinfo* ptr_;
		};

		iterator begin() { return raw_results_; }
		iterator end() { return nullptr; }

		AddrinfoResults(const std::string& hostname, const std::string& port, const addrinfo& hints);
		~AddrinfoResults();

		AddrinfoResults(const AddrinfoResults&) = delete;
		AddrinfoResults& operator=(const AddrinfoResults&) = delete;
	private:
		addrinfo* raw_results_;
	};

	class TcpConnection {
	public:
		TcpConnection(std::istream& is, std::ostream& os, const std::string& hostname, const std::string& port);
		~TcpConnection();

		std::string hostname() { return hostname_; }
		std::string port() { return port_; }

		static unsigned long long connections_amount;
	private:

		void initialize_winsock();
		void initialize_hints();
		addrinfo hints_;
		std::string hostname_;
		std::string port_;
	};

	class TcpSocket {
	public:
		TcpSocket(AddrinfoResults::iterator begin);
		~TcpSocket();
		void send(std::string sendbuf, int flags = 0);
		std::string recv(int flags = 0);

	private:
		void create_socket_(AddrinfoResults::iterator it);
		int connect(AddrinfoResults::iterator it);

		AddrinfoResults::iterator chosen_result_;
		SOCKET connect_socket_ = INVALID_SOCKET;
		
		constexpr static int default_buflen = 512;
	};

	class StartupError : public std::exception { using std::exception::exception; };
	class GetAddrInfoError : public std::exception { using std::exception::exception; };
	class SocketError : public std::exception { using std::exception::exception; };
	class ConnectionError : public std::exception { using std::exception::exception; };
	class SendError : public std::exception { using std::exception::exception; };
	class RecvError : public std::exception { using std::exception::exception; };
}

#endif