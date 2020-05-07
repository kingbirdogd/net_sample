#ifndef TCP_CLIENT_INC_TCP_CLIENT_HPP_
#define TCP_CLIENT_INC_TCP_CLIENT_HPP_
#include <list>
#include <vector>
#include <string>
#include <functional>

namespace net
{
	class tcp_client
	{
	private:
		enum class status : unsigned int
		{
			NONE = 0x00,
			CLOSED = 0x01,
			CONNECTING = 0x02,
			CONNECTED = 0x03,
			ACCEPTED = 0x04
		};
		struct snd_node
		{
			std::vector<char> data;
			std::size_t snd;
		};
		using buffer_list = std::list<snd_node>;
		using msg_cb = std::function<void(const char*, std::size_t)>;
		using event_cb = std::function<void()>;
	private:
		buffer_list _buffer;
		msg_cb _msg_cb;
		event_cb _on_connected;
		event_cb _on_disconnected;
		int _sock;
		status _sta;
		std::string _host;
		unsigned short int _port;
	private:
		void _connect();
		void _check_connect();
		void _do_send();
		void _do_recv();
	public:
		tcp_client(const std::string& host, unsigned short int port);
		tcp_client(int sock, const std::string& host, unsigned short int port);
		tcp_client(tcp_client&&);
		tcp_client& operator=(tcp_client&&);
		~tcp_client();
		bool run();
		void send(const void* ptr, std::size_t size);
		void close();
		void set_on_msg(msg_cb&& msg_cb);
		void set_connected(event_cb&& on_connected);
		void set_disconnected(event_cb&& on_disconnected);
		template <typename T>
		void send(const T& obj)
		{
			send(&obj, sizeof(T));
		}
		const std::string& get_host();
		unsigned short int get_port();
		tcp_client() = delete;
		tcp_client(const tcp_client&) = delete;
		tcp_client& operator=(const tcp_client&) = delete;
	};
}




#endif /* TCP_CLIENT_INC_TCP_CLIENT_HPP_ */
