#include "connection.h"
#include <thread>
unsigned long long zabroda::TcpConnection::connections_amount = 0;

zabroda::TcpConnection::TcpConnection(std::istream& is, std::ostream& os, const std::string& hostname, const std::string& port)
	: hostname_(hostname), port_(port)
{
	if (connections_amount == 0)
		initialize_winsock();
	++connections_amount;
	initialize_hints();
	AddrinfoResults results_{ hostname, port, hints_ }; // TODO: May be I should move it into TcpSocket
	TcpSocket sock{ results_.begin() }; // TODO: Need to find better way to manage infinite loop in thread
	std::string input;
	std::jthread th{ [&os, &sock]() {
		try {
			while (true) os << sock.recv();
		}
		catch (RecvError& e) {  }
	} };
	while (std::getline(is, input)) {
		sock.send(input + '\n');
	}
	sock.~TcpSocket();
 }

zabroda::TcpConnection::~TcpConnection()
{
	--connections_amount;

	if (connections_amount == 0)
		WSACleanup();
}

void zabroda::TcpConnection::initialize_hints()
{
	ZeroMemory(&hints_, sizeof(hints_));
	hints_.ai_family = AF_UNSPEC;
	hints_.ai_socktype = SOCK_STREAM;
	hints_.ai_protocol = IPPROTO_TCP;
}

void zabroda::TcpConnection::initialize_winsock()
{
	WSADATA wsaData;

	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		throw StartupError{ "WSAStartup failed" };
	}
}

zabroda::TcpSocket::TcpSocket(AddrinfoResults::iterator begin)
	: chosen_result_(begin)
{
	int iResult;
	while (chosen_result_) {
		create_socket_(chosen_result_);
		if (connect(chosen_result_) != SOCKET_ERROR) break;
	}
}

void zabroda::TcpSocket::create_socket_(AddrinfoResults::iterator it)
{
	connect_socket_ = socket(it->ai_family, it->ai_socktype,
		it->ai_protocol);

	if (connect_socket_ == INVALID_SOCKET)
		throw SocketError("Error at socket()");
}

int zabroda::TcpSocket::connect(AddrinfoResults::iterator it)
{
	return ::connect(connect_socket_, chosen_result_->ai_addr, static_cast<int>(chosen_result_->ai_addrlen));
}


zabroda::TcpSocket::~TcpSocket()
{
	closesocket(connect_socket_);
}

void zabroda::TcpSocket::send(std::string sendbuf, int flags)
{
	if (::send(connect_socket_, sendbuf.c_str(), sendbuf.length(), flags) == SOCKET_ERROR)
		throw SendError{"send failed"};
}

std::string zabroda::TcpSocket::recv(int flags)
{
	char buf[default_buflen];
	int iResult;
	iResult = ::recv(connect_socket_, buf, default_buflen, flags);
	if (iResult < 0) throw RecvError{ "recv failed" };
	std::string result_str{ buf, static_cast<size_t>(iResult) };
	return result_str;
}

zabroda::AddrinfoResults::AddrinfoResults(const std::string& hostname, const std::string& port, const addrinfo& hints)
{
	if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &raw_results_)) {
		throw GetAddrInfoError{ "Couldn't resolve given hostname and port" };
	}
}

zabroda::AddrinfoResults::~AddrinfoResults()
{
	freeaddrinfo(raw_results_);
}
