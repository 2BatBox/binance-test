#pragma once

#include <map>
#include <jsoncpp/json/json.h>
#include <libwebsockets.h>

#include "../../Log.h"
#include "../../Config.h"
#include "../../Utils.h"

namespace binance {
namespace ws {

class Connector {

	// The event consumer callback.
	using CallBack_t = int (*)(void* instance, const Json::Value& value);

	struct CallBackRecord {
		CallBack_t callback;
		void* instance;
	};

	lws_protocols _protocols[Config::WSProtocols_nb + 1u /*Termination item.*/];
	lws_context* _context;
	std::map<lws*, CallBackRecord> _handles;

public:

	/**
	 * Neither moving nor copping are allowed since the instance pointer is used to arrange calling back from LWS.
	 */
	Connector(const Connector&) = delete;
	Connector& operator=(const Connector&) = delete;

	Connector(Connector&&) = delete;
	Connector& operator=(Connector&&) = delete;

	Connector() noexcept : _context(nullptr) {

		size_t idx;
		for(idx = 0; idx < Config::WSProtocols_nb; ++idx) {
			memset(_protocols + idx, 0, sizeof(*_protocols));

			_protocols[idx] = {
				.name = Config::WSProtocolName,
				.callback = ws_callback,
				.per_session_data_size = Config::WSSessionData,
				.rx_buffer_size = Config::WSRxBuffer,
				.user = this
			};
		}

		// The termination item in the list.
		memset(_protocols + idx, 0, sizeof(*_protocols));

		LOG_DEBUG("binance::ws::Connector()\n");
	}


	~Connector() noexcept {
		lws_context_destroy(_context);
		LOG_DEBUG("binance::ws::~Connector()\n");
	}


	/**
	 * Opens the socket instance. No one method is allowed to call before getting the instance opened.
	 * @return false - in case of any errors.
	 */
	bool init() noexcept {
		LOG_DEBUG("binance::ws::Connector::init()\n");
		bool result = true;

		lws_context_creation_info info;
		memset(&info, 0, sizeof(info));

		info = {
			.port = CONTEXT_PORT_NO_LISTEN,
			.iface = nullptr,
			.protocols = _protocols,
			.gid = -1,
			.uid = -1,
			.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT
		};

		_context = lws_create_context(&info);

		if(_context == nullptr) {
			LOG_ERROR("Unable to create LWS context.");
			result = false;
		}

		return result;
	}

	bool register_ticker(CallBack_t callback, void* instance, const std::string& pair) noexcept {
		std::string url = "/ws/" + pair + std::string("@ticker");
		Utils::string_to_lower(url);

		LOG_DEBUG("binance::ws::Connector::register_ticker(url='%s')\n", url.c_str());

		return register_callback(callback, instance, url.c_str());
	}

	/**
	 * Enters the LWS service loop.
	 */
	inline void service() noexcept {
		lws_service(_context, Config::WSServiceTimeoutMS);
	}

private:

	bool register_callback(CallBack_t callback, void* instance, const char* path) noexcept {
		bool result = true;

		lws_client_connect_info ccinfo;
		memset(&ccinfo, 0, sizeof(ccinfo));

		ccinfo = {
			.context = _context,
			.address = Config::BinanceWsHost,
			.port = Config::BinanceWsPort,
			.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK,
			.path = path,
			.host = lws_canonical_hostname(_context),
			.origin = "origin",
			.protocol = _protocols[0].name,
			.opaque_user_data = this
		};

		lws* connection = lws_client_connect_via_info(&ccinfo);
		if(connection) {
			_handles.insert(std::make_pair(connection, CallBackRecord{callback, instance}));
		} else {
			LOG_ERROR("Unable to create an LWS connection.");
			result = false;
		}
		return result;
	}


	static int ws_callback(lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) noexcept {
		auto instance = reinterpret_cast<Connector*>(lws_get_opaque_user_data(wsi));
		return instance->ws_callback_instance(wsi, reason, in, len);
	}

	int ws_callback_instance(lws* ws, enum lws_callback_reasons reason, void* in, size_t len) noexcept {

		switch(reason) {

			case LWS_CALLBACK_CLIENT_ESTABLISHED:
				lws_callback_on_writable(ws);
				break;

			case LWS_CALLBACK_CLIENT_RECEIVE: {

				const auto input_string = reinterpret_cast<const char*>(in);
				Json::Reader reader;
				Json::Value json;

				if(reader.parse(std::string(input_string), json)) {
					const auto it = _handles.find(ws);
					if(it != _handles.end()) {
						const auto& record = it->second;
						const auto err = record.callback(record.instance, json);
						if(err) {
							LOG_ERROR("The consumer rejects the event '%s' err=%d", input_string, err);
						}
					} else {
						LOG_ERROR("A LWS event with no consumer. '%s'", input_string);
					}
				} else {
					LOG_ERROR("JSON parsing failure. '%s'", input_string);
				}

			}
				break;

			case LWS_CALLBACK_CLOSED:
			case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:{
				const auto it = _handles.find(ws);
				if(it != _handles.end()) {
					_handles.erase(it);
				} else {
					LOG_ERROR("An unknown connection closing event.");
				}
			}
				break;

			default:
				break;
		}

		return EXIT_SUCCESS;
	}

};

}; // namespace ws
}; // namespace binance
