#ifndef NET_TCP_SERVER_INC_TCP_SERVICE_HPP_
#define NET_TCP_SERVER_INC_TCP_SERVICE_HPP_

#include <unordered_set>
#include <functional>
#include <net/tcp_client.hpp>

namespace net
{
	class tcp_service
	{
	private:
		enum class status : unsigned int
		{
			UNBIND = 0x00,
			BINDED = 0x01
		};
		using client_set = std::unordered_set<tcp_client*>;
		using msg_cb = std::function<void(tcp_client*, const char*, std::size_t)>;
		using service_event_cb = std::function<void()>;
		using client_event_cb = std::function<void(tcp_client*)>;
	private:
		service_event_cb _on_bind;
		service_event_cb _on_unbind;
		client_event_cb _on_connected;
		client_event_cb _on_disconnected;
		msg_cb _on_msg;
		client_set _clients;
		int _sock;
		std::string _bind_addr;
		unsigned short int _bind_port;
		status _sta;
	public:
		tcp_service(unsigned short int bind_port, const std::string& bind_addr = "");
		tcp_service(tcp_service&&);
		tcp_service& operator=(tcp_service&&);
		~tcp_service();
		bool send(tcp_client* cli, const void* ptr, std::size_t size);
		void send(const void* ptr, std::size_t size);
		template <typename T>
		void send(tcp_client* cli, const T& obj)
		{
			send(cli, &obj, sizeof(T));
		}
		template <typename T>
		void send(const T& obj)
		{
			send(&obj, sizeof(T));
		}
		bool close(tcp_client* cli);
		void close();
		void set_on_msg(msg_cb&& _on_msg);
		void set_service_bind(service_event_cb&& on_bind);
		void set_service_unbind(service_event_cb&& on_unbind);
		void set_on_connect(client_event_cb&& on_connected);
		void set_on_disconnect(client_event_cb&& on_disconnected);
		void run();
		tcp_service() = delete;
		tcp_service(const tcp_client&) = delete;
		tcp_service& operator=(const tcp_client&) = delete;
	private:
		void _bind();
		void _accept();
		void _run_clients();
	};
}



#endif /* NET_TCP_SERVER_INC_TCP_SERVICE_HPP_ */
