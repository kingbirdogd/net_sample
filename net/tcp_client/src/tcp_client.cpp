#include <cstring>
#include <net/tcp_client.hpp>
#ifdef __APPLE__
#include <sys/errno.h>
#endif
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#ifndef MSG_MORE
#define MSG_MORE 0
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#ifdef SO_NOSIGPIPE
#define CEPH_USE_SO_NOSIGPIPE
#else
#error "Cannot block SIGPIPE!"
#endif
#endif

using namespace net;

void tcp_client::_connect()
{
	if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		close();
		return;
	}
	int flags = fcntl(_sock, F_GETFL, 0);
	if (flags == -1)
	{
		close();
		return;
	}
	flags = fcntl(_sock, F_SETFL, flags | O_NONBLOCK);
	if (flags == -1)
	{
		close();
		return;
	}
	int optval = 1;
	std::size_t optlen = sizeof(optval);
	if(setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
	{
		close();
		return;
	}
	struct sockaddr_in serv_addr;
	std::memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(_port);
	if (inet_pton(AF_INET, _host.c_str(), &serv_addr.sin_addr) <= 0)
	{
		close();
		return;
	}
	if (::connect(_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		if (errno != EINPROGRESS)
		{
			close();
		}
		else
		{
			_sta = status::CONNECTING;
		}
		return;
	}
	_sta = status::CONNECTED;
	if (_on_connected)
		_on_connected();
}

void tcp_client::_check_connect()
{
	struct pollfd fd;
	fd.fd = _sock;
	fd.events = POLLOUT;
	auto poll_rt = ::poll(&fd, 1, 0);
	if (1 == poll_rt)
	{
		int so_error = 0;
		socklen_t len = sizeof(so_error);
		if (0 == ::getsockopt(_sock, SOL_SOCKET, SO_ERROR, &so_error, &len))
		{
			if (0 == so_error)
			{
				_sta = status::CONNECTED;
				if (_on_connected)
					_on_connected();
			}
			else if (so_error != EINPROGRESS)
			{
				close();
			}
		}
		else
		{
			close();
		}
	}
}

void tcp_client::_do_send()
{
	while (0 != _buffer.size())
	{
		auto& first = _buffer.front();
		while (first.snd < first.data.size())
		{
			auto rt = ::send(_sock, &first.data[first.snd], first.data.size() - first.snd, MSG_NOSIGNAL);
			if (rt > 0)
			{
				first.snd += rt;
			}
			else
			{
				if (0 == rt)
				{
					close();
				}
				else if (errno != EAGAIN)
				{
					close();
				}
				return;
			}
		}
		if (first.data.size() == first.snd)
			_buffer.pop_front();
	}
}

void tcp_client::_do_recv()
{
	char rcv_buffer[1024];
	while (true)
	{
		ssize_t size_received = ::recv(_sock, rcv_buffer, 1024, 0);
		if (size_received > 0)
		{
			if (_msg_cb)
				_msg_cb(rcv_buffer, size_received);
		}
		else if (0 == size_received)
		{
			close();
		}
		else if (errno != EAGAIN)
		{
			close();
		}
		return;
	}
}

tcp_client::tcp_client(const std::string& host, unsigned short int port):
		_buffer(),
		_msg_cb(),
		_on_connected(),
		_on_disconnected(),
		_sock(0),
		_sta(status::CLOSED),
		_host(host),
		_port(port)
{
}

tcp_client::tcp_client(int sock,const std::string& host, unsigned short int port):
		_buffer(),
		_msg_cb(),
		_on_connected(),
		_on_disconnected(),
		_sock(sock),
		_sta(status::ACCEPTED),
		_host(host),
		_port(port)
{
}

tcp_client::tcp_client(tcp_client&& client):
	_buffer(std::move(client._buffer)),
	_msg_cb(std::move(client._msg_cb)),
	_on_connected(std::move(client._on_connected)),
	_on_disconnected(std::move(client._on_disconnected)),
	_sock(client._sock),
	_sta(client._sta),
	_host(std::move(client._host)),
	_port(client._port)
{
	client._sock = 0;
}

tcp_client& tcp_client::operator=(tcp_client&& client)
{
	_buffer = std::move(client._buffer);
	_msg_cb = std::move(client._msg_cb);
	_on_connected = std::move(client._on_connected);
	_on_disconnected = std::move(client._on_disconnected);
	_sock = client._sock;
	_sta = client._sta;
	_host = std::move(client._host);
	_port = client._port;
	client._sock = 0;
	return *this;
}

tcp_client::~tcp_client()
{
	close();
}

bool tcp_client::run()
{
	switch(_sta)
	{
		case status::CLOSED:
		{
			_connect();
			break;
		}
		case status::CONNECTING:
		{
			_check_connect();
			break;
		}
		case status::CONNECTED:
		case status::ACCEPTED:
		{
			_do_send();
			_do_recv();
			break;
		}
		default:
		{
			break;
		}
	}
	if (status::CLOSED == _sta || status::NONE == _sta)
		return false;
	else
		return true;
}

void tcp_client::send(const void* ptr, std::size_t size)
{
	if (nullptr == ptr || 0 == size)
		return;
	if (!_buffer.empty() || (_sta != status::CONNECTED && _sta != status::ACCEPTED))
	{
		snd_node node;
		node.data.resize(size);
		std::memcpy(&node.data[0], ptr, size);
		node.snd = 0;
		_buffer.push_back(std::move(node));
	}
	else
	{
		const char* psz = static_cast<const char*>(ptr);
		while (size > 0)
		{
			auto rt = ::send(_sock, psz, size, MSG_NOSIGNAL);
			if (rt > 0)
			{
				psz += rt;
				size -= rt;
			}
			else
			{
				if (0 == rt)
				{
					close();
				}
				else if (errno != EAGAIN)
				{
					close();
				}
				break;
			}
		}
		if (size > 0)
		{
			snd_node node;
			node.data.resize(size);
			std::memcpy(&node.data[0], psz, size);
			node.snd = 0;
			_buffer.push_back(std::move(node));
		}
	}
}

void tcp_client::close()
{
	if (status::CONNECTED == _sta || status::CONNECTING == _sta)
	{
		_sta = status::CLOSED;
		if (_on_disconnected)
		{
			_on_disconnected();
		}
		if (0 != _sock)
		{
			::close(_sock);
			_sock = 0;
		}
		_buffer.clear();
	}
	else if (status::ACCEPTED == _sta)
	{
		_sta = status::NONE;
		if (_on_disconnected)
		{
			_on_disconnected();
		}
		if (0 != _sock)
		{
			::close(_sock);
			_sock = 0;
		}
		_buffer.clear();
	}
}

void tcp_client::set_on_msg(msg_cb&& msg_cb)
{
	_msg_cb = std::move(msg_cb);
}

void tcp_client::set_connected(event_cb&& on_connected)
{
	_on_connected = std::move(on_connected);
}

void tcp_client::set_disconnected(event_cb&& on_disconnected)
{
	_on_disconnected = std::move(on_disconnected);
}

const std::string& tcp_client::get_host()
{
	return _host;
}

unsigned short int tcp_client::get_port()
{
	return _port;
}





