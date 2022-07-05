#include "connection.h"
#include <thread>
unsigned long long zabroda::TcpConnection::connections_amount = 0;

zabroda::TcpConnection::TcpConnection(std::istream& is, std::ostream& os, const std::string& hostname, const std::string& port)
	: hostname_(hostname), port_(port), is_(is), os_(os)
{
	if (connections_amount == 0)
		initialize_winsock_();
	++connections_amount;
	initialize_hints_();
	results_ = AddrinfoResults{ hostname, port, hints_ };
	sock_ = TcpSocket{ results_.begin() };
 }

zabroda::TcpConnection::~TcpConnection()
{
	--connections_amount;

	if (connections_amount == 0)
		WSACleanup();
}

void zabroda::TcpConnection::initialize_hints_()
{
	ZeroMemory(&hints_, sizeof(hints_));
	hints_.ai_family = AF_UNSPEC;
	hints_.ai_socktype = SOCK_STREAM;
	hints_.ai_protocol = IPPROTO_TCP;
}

void zabroda::TcpConnection::start_protocol()
{
	std::string input;
	std::jthread th{ [this]() {
		try {
			while (true) os_ << sock_.recv();
		}
		catch (RecvError& e) {}
	} };
	while (std::getline(is_, input)) {
		sock_.send(input + "\r\n");
	}
	sock_.~TcpSocket();
}

void zabroda::TcpConnection::initialize_winsock_()
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

void zabroda::TcpSocket::send(std::string sendbuf, int flags) const
{
	if (::send(connect_socket_, sendbuf.c_str(), sendbuf.length(), flags) == SOCKET_ERROR)
		throw SendError{"send failed"};
}

std::string zabroda::TcpSocket::recv(int flags) const
{
	char buf[default_buflen];
	int iResult;
	iResult = ::recv(connect_socket_, buf, default_buflen, flags);
	if (iResult < 0) throw RecvError{ "recv failed" };
	std::string result_str{ buf, static_cast<size_t>(iResult) };
	return result_str;
}

zabroda::TcpSocket::TcpSocket(TcpSocket&& right) noexcept
	: connect_socket_(right.connect_socket_), chosen_result_(right.chosen_result_)
{
	right.connect_socket_ = INVALID_SOCKET;
	right.chosen_result_ = nullptr;
}

zabroda::TcpSocket & zabroda::TcpSocket::operator=(TcpSocket && right) noexcept
{
	closesocket(connect_socket_);
	connect_socket_ = right.connect_socket_;
	chosen_result_ = right.chosen_result_;
	right.connect_socket_ = INVALID_SOCKET;
	right.chosen_result_ = nullptr;
	return *this;
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

zabroda::AddrinfoResults::AddrinfoResults(AddrinfoResults&& right) noexcept
{
	raw_results_ = right.raw_results_;
	right.raw_results_ = nullptr;
}

zabroda::AddrinfoResults& zabroda::AddrinfoResults::operator=(AddrinfoResults&& right) noexcept
{
	freeaddrinfo(raw_results_);
	raw_results_ = right.raw_results_;
	right.raw_results_ = nullptr;
	return *this;
}
