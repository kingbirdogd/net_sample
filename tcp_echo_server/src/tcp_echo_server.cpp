#include <iostream>
#include <cstdlib>

#include <net/tcp_service.hpp>


int main(int iArgc, char** pszArgv)
{
	if (2 != iArgc)
	{
		std::cout << "usage: tcp_echo_server <port[1,65535]>" << std::endl;
		return -1;
	}
	int iPort = std::atoi(pszArgv[1]);
	if (iPort < 1 || iPort > 65535)
	{
		std::cout << "usage: tcp_echo_server <port[1,65535]>" << std::endl;
		return -2;
	}
	auto sPort = static_cast<unsigned short int>(iPort);
	net::tcp_service s(sPort);
	s.set_service_bind([&]
	{
		std::cout << "Service Binded" << std::endl;
	});
	s.set_service_unbind([&]
	{
		std::cout << "Service UnBinded" << std::endl;
	});
	s.set_on_connect([&](net::tcp_client* cli)
	{
		std::cout << "client connected, IP:" << cli->get_host() << ", PORT:" << cli->get_port() << std::endl;
		cli->send("Hello Echo\n\r", 12);
	});
	s.set_on_disconnect([&](net::tcp_client* cli)
	{
		std::cout << "client disconnected, IP:" << cli->get_host() << ", PORT:" << cli->get_port() << std::endl;
	});
	s.set_on_msg([&](net::tcp_client*, const char* ptr, std::size_t size)
	{
		std::cout << "get msg, size:" << size << std::endl;
		//broadcast
		s.send(ptr, size);
	});
	while (true)
	{
		s.run();
	}
	return 0;
}





