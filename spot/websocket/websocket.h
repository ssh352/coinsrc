

#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__


#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include<functional>
#include <spot/utility/gzipwrapper.h>
#include <spot/utility/okzlibwrapper.h>
// #include "spot/utility/Logging.h"


extern string gWebsocketProxyAddr;

//typedef void(*websocketpp_callbak_open)();
//typedef void(*websocketpp_callbak_close)();
//typedef void(*websocketpp_callbak_message)(const char *message);

typedef std::function<void()> websocketpp_callbak_open;
typedef std::function<void()> websocketpp_callbak_close;
typedef std::function<void(const char *msg)> websocketpp_callbak_message;


typedef websocketpp::client<websocketpp::config::asio_client> http_client;
typedef websocketpp::client<websocketpp::config::asio_tls> client;
//typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

// typedef websocketpp::server<websocketpp::config::asio> serverConfig;

#define MAX_RETRY_COUNT		10000

enum CONNECTION_STATE
{
	CONNECTION_STATE_UNKONWN,
	CONNECTION_STATE_CONNECTING,
	CONNECTION_STATE_DISCONNECT,
};

class WebSocket
{
private:
    client m_https_endpoint;
	// serverConfig server;
	http_client m_http_endpoint;
	websocketpp::connection_hdl m_hdl;
	std::string m_uri;
	CONNECTION_STATE m_con_state;
public:
	websocketpp_callbak_open callbak_open;
	websocketpp_callbak_close  callbak_close;
	websocketpp_callbak_message callbak_message;

	bool m_isHttps;//�Ƿ�ʹ��httpsЭ��
	bool m_manual_close;//�Ƿ�Ϊ�����ر����ӣ���������û������رգ����ӵ��Ͽ����ӻص�ʱ���Զ�ִ���������ӻ��ơ�
	bool m_isCompress = false;//�Ƿ���ܵ���ѹ������
	bool m_isOkCompress = false;//Okex�Ƿ���ܵ���ѹ������

    typedef WebSocket type;

    WebSocket() :  
	m_isHttps(true),
	m_manual_close(false),
	m_con_state(CONNECTION_STATE_UNKONWN)
	//callback_open(0),
	//callbak_close(0),
	//callbak_message(0)
	{
		m_https_endpoint.set_access_channels(websocketpp::log::alevel::all);
		m_https_endpoint.set_error_channels(websocketpp::log::elevel::all);

		// this will turn off console output for frame header and payload
		m_https_endpoint.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload); 
		
		// this will turn off everything in console output
		// m_https_endpoint.clear_access_channels(websocketpp::log::alevel::all); 

		//Register our handlers
		//m_https_endpoint.set_socket_init_handler(bind(&type::on_socket_init,this,::_1));
		m_https_endpoint.set_tls_init_handler(bind(&type::on_tls_init, this, ::_1));
		m_https_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
		m_https_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
		m_https_endpoint.set_close_handler(bind(&type::on_close, this, ::_1));
		m_https_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));

		// Initialize ASIO
		m_https_endpoint.init_asio(); 
	//	m_http_endpoint.start_perpetual();   
    }

    ~WebSocket()
	{
	}

	void Reset_WebSocket()
	{
		doclose();
		m_isHttps = true;
		m_manual_close = false;
		m_con_state = CONNECTION_STATE_UNKONWN;
		m_https_endpoint.set_access_channels(websocketpp::log::alevel::all);
		m_https_endpoint.set_error_channels(websocketpp::log::elevel::all);

		m_https_endpoint.set_tls_init_handler(bind(&type::on_tls_init, this, ::_1));
		m_https_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
		m_https_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
		m_https_endpoint.set_close_handler(bind(&type::on_close, this, ::_1));
		m_https_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));

		m_https_endpoint.init_asio(); 
	}

	void Set_Protocol_Mode(bool isHttps)
	{
		m_isHttps = isHttps;
		if (!m_isHttps)
		{
			m_http_endpoint.set_access_channels(websocketpp::log::alevel::all);
			m_http_endpoint.set_error_channels(websocketpp::log::elevel::all);

			//Register our handlers
			m_http_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
			m_http_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
			m_http_endpoint.set_close_handler(bind(&type::on_close, this, ::_1));
			m_http_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));

			// Initialize ASIO
			m_http_endpoint.init_asio();
		//	m_http_endpoint.start_perpetual();
		}
	}

	void SetCompress(bool isCompress) 
	{
		m_isCompress = isCompress;
	}

	void SetOkCompress(bool isCompress)
	{
		m_isOkCompress = isCompress;
	}

	void start()
	{
        websocketpp::lib::error_code ec;

		if (m_isHttps)
		{
			client::connection_ptr con = m_https_endpoint.get_connection(m_uri, ec);

			if (ec) {
				m_https_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
			}

			m_https_endpoint.set_open_handshake_timeout(5000);
			m_https_endpoint.set_close_handshake_timeout(5000);

			if (gWebsocketProxyAddr.length() != 0)
			{
				std::cout << "websocket set proxy:" << gWebsocketProxyAddr << std::endl;
				con->set_proxy(gWebsocketProxyAddr);
			}

			m_https_endpoint.connect(con);

			// Start the ASIO io_service run loop
			m_https_endpoint.run();
		}
		else
		{
			http_client::connection_ptr con = m_http_endpoint.get_connection(m_uri, ec);

			if (ec) 
			{
				m_https_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
			}

			if (gWebsocketProxyAddr.length() != 0)
			{
				std::cout << "websocket set proxy:" << gWebsocketProxyAddr << std::endl;
				con->set_proxy(gWebsocketProxyAddr);
			}

			m_http_endpoint.connect(con);

			// Start the ASIO io_service run loop
			m_http_endpoint.run();

		}
        
		if(callbak_close != 0)callbak_close();
    }

    void on_socket_init(websocketpp::connection_hdl) 
	{

    }

    context_ptr on_tls_init(websocketpp::connection_hdl)
	{
        context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);

        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
			// LOG_ERROR << "on_tls_init on_tls_init: " << e.what() << __FILE__ << " " << __LINE__;
        }
        return ctx;
    }

    void on_fail(websocketpp::connection_hdl hdl)
	{
		
		if (m_isHttps)
		{
			client::connection_ptr con = m_https_endpoint.get_con_from_hdl(hdl);

			std::cout << "on_fail" << std::endl;
			std::cout << con->get_state() << std::endl;
			std::cout << con->get_local_close_code() << std::endl;
			std::cout << con->get_local_close_reason() << std::endl;
			std::cout << con->get_remote_close_code() << std::endl;
			std::cout << con->get_remote_close_reason() << std::endl;
			std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
			// LOG_INFO << "on_fail m_isHttps state: " << con->get_state() << ", get_local_close_code: " << con->get_local_close_code()
			// 	<< ", get_local_close_reason: " << con->get_local_close_reason() << ", get_remote_close_code: " << con->get_remote_close_code()
			// 	<< ", get_remote_close_reason: " << con->get_remote_close_reason()
			// 	<< ", message: " << con->get_ec().message();
		}
		else
		{
			http_client::connection_ptr con = m_http_endpoint.get_con_from_hdl(hdl);

			std::cout << "on_fail" << std::endl;
			std::cout << con->get_state() << std::endl;
			std::cout << con->get_local_close_code() << std::endl;
			std::cout << con->get_local_close_reason() << std::endl;
			std::cout << con->get_remote_close_code() << std::endl;
			std::cout << con->get_remote_close_reason() << std::endl;
			std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
			// LOG_INFO << "on_fail m_isHttp state: " << con->get_state() << ", get_local_close_code: " << con->get_local_close_code()
			// 	<< ", get_local_close_reason: " << con->get_local_close_reason() << ", get_remote_close_code: " << con->get_remote_close_code()
			// 	<< ", get_remote_close_reason: " << con->get_remote_close_reason()
			// 	<< ", message: " << con->get_ec().message();
		}
		
    }

    void on_close_handshake_timeout(websocketpp::connection_hdl hdl)
	{
	}
	
    void on_open(websocketpp::connection_hdl hdl) 
	{
		m_hdl = hdl;
		m_manual_close = false;
		if(callbak_open != 0)
			callbak_open();
    }
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) 
	{
		
		//std::cout << "Message: " << msg->get_payload().c_str() << std::endl;
		if (callbak_message != 0)
		{
			if (m_isCompress)
			{
				callbak_message(GzipWrapper::decompress(msg->get_payload()).c_str());
			}
			else if (m_isOkCompress)
			{
				callbak_message(OkZlibWrapper::decompress(msg->get_payload()).c_str());
			}
			else
			{
				callbak_message(msg->get_payload().c_str());
			}
		}
    }

    void on_close(websocketpp::connection_hdl hdl) 
	{
		if (m_isHttps)
		{
			client::connection_ptr con = m_https_endpoint.get_con_from_hdl(hdl);

			std::cout << "on_close handler" << std::endl;
			std::cout << con->get_state() << std::endl;
			std::cout << con->get_local_close_code() << std::endl;
			std::cout << con->get_local_close_reason() << std::endl;
			std::cout << con->get_remote_close_code() << std::endl;
			std::cout << con->get_remote_close_reason() << std::endl;
			std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
			// LOG_INFO << "on_close m_isHttps state: " << con->get_state() << ", get_local_close_code: " << con->get_local_close_code()
			// 	<< ", get_local_close_reason: " << con->get_local_close_reason() << ", get_remote_close_code: " << con->get_remote_close_code()
			// 	<< ", get_remote_close_reason: " << con->get_remote_close_reason()
			// 	<< ", message: " << con->get_ec().message();
		}
		else
		{
			http_client::connection_ptr con = m_http_endpoint.get_con_from_hdl(hdl);

			std::cout << "on_close handler" << std::endl;
			std::cout << con->get_state() << std::endl;
			std::cout << con->get_local_close_code() << std::endl;
			std::cout << con->get_local_close_reason() << std::endl;
			std::cout << con->get_remote_close_code() << std::endl;
			std::cout << con->get_remote_close_reason() << std::endl;
			std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
			// LOG_INFO << "on_close m_isHttp state: " << con->get_state() << ", get_local_close_code: " << con->get_local_close_code()
			// 	<< ", get_local_close_reason: " << con->get_local_close_reason() << ", get_remote_close_code: " << con->get_remote_close_code()
			// 	<< ", get_remote_close_reason: " << con->get_remote_close_reason()
			// 	<< ", message: " << con->get_ec().message();
		}
		
    }

    void doclose() 
	{
		m_manual_close = true;
		if (m_isHttps)
		{
			m_https_endpoint.close(m_hdl, websocketpp::close::status::going_away, "");
		}
		else
		{
			m_http_endpoint.close(m_hdl, websocketpp::close::status::going_away, "");
		}
    }

	void run(std::string &uri)
	{
		try {
			m_uri = uri;
			start();
		} catch (const websocketpp::exception &e) {
			std::cout << e.what() << std::endl;
			// LOG_ERROR << e.what() << __FILE__ << " " << __LINE__;
		} catch (...) {
			std::cout << "other exception" << std::endl;
			// LOG_ERROR << "other exception " << __FILE__ << " " << __LINE__;
		}
	}
	void emit(std::string channel)
	{
		string cmd = "{'event':'addChannel','channel':'";
		cmd += channel;
		cmd += "'}"; 
		if (m_isHttps)
		{
			m_https_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);
		}
		else
		{
			m_http_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);
		}
	}

	void emit(std::string channel,string &parameter)
	{
		string cmd = "{'event':'addChannel','channel':'";
		cmd += channel;
		cmd += "','parameters':{";
		cmd += parameter;
		cmd += "}}";
		if (m_isHttps)
		{
			m_https_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);

		}
		else
		{
			m_http_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);
		}
	}
	void send(string &msg)
	{
		if (m_isHttps)
		{
			try {
				m_https_endpoint.send(m_hdl, msg, websocketpp::frame::opcode::text);
			} catch (const websocketpp::exception &e) {
				std::cout << e.what() << std::endl;
				// LOG_ERROR << e.what() << __FILE__ << " " << __LINE__;
			} catch (...) {
				std::cout << "other exception" << std::endl;
				// LOG_ERROR << "other exception " << __FILE__ << " " << __LINE__;
			}
		}
		else
		{
			try {
				m_http_endpoint.send(m_hdl, msg, websocketpp::frame::opcode::text);
			} catch (const websocketpp::exception &e) {
				std::cout << e.what() << std::endl;
				// LOG_ERROR << e.what() << __FILE__ << " " << __LINE__;
				return;
			} catch (...) {
				std::cout << "other exception" << std::endl;
				// LOG_ERROR << "other exception " << __FILE__ << " " << __LINE__;
			}
		}
		return;
	}
	void remove(std::string channel)
	{
		string cmd = "{'event':'removeChannel','channel':'";
		cmd += channel;
		cmd += "'}";
		if (m_isHttps)
		{
			m_https_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);
		}
		else
		{
			m_http_endpoint.send(m_hdl, cmd, websocketpp::frame::opcode::text);
		}
	}

};


#endif /* __WEBSOCKET_H__ */
