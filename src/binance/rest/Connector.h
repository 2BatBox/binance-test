#pragma once

#include <string>
#include <string_view>

#include <curl/curl.h>
#include <openssl/hmac.h>

#include "api.h"
#include "../../Log.h"
#include "../../Utils.h"

namespace binance {
namespace rest {

class Connector {

	using HttpHeaders = std::vector<std::string>;

	CURL* _curl;
	const std::string _host;
	const std::string _api_key;
	const std::string _secret_key;

	HttpHeaders _http_headers;
	std::string _response;

public:

	Connector(const Connector&) = delete;
	Connector& operator=(const Connector&) = delete;

	Connector(Connector&&) = delete;
	Connector& operator=(Connector&&) = delete;

	/**
	 * @param curl - A curl instance. MUST NOT be nullptr.
	 * @param api_key - MUST NOT be empty.
	 * @param secret_key - MUST NOT be empty.
	 */
	Connector(
		CURL* curl,
		std::string host,
		std::string api_key,
		std::string secret_key,
		bool verbose = false
	         ) noexcept :
		_curl(curl),
		_host(std::move(host)),
		_api_key(std::move(api_key)),
		_secret_key(std::move(secret_key)) {

		LOG_DEBUG("binance::rest::Connector()\n");
		_http_headers.emplace_back(std::string("X-MBX-APIKEY: " + _api_key));
	}

	~Connector() noexcept {
		LOG_DEBUG("binance::rest::~Connector()\n");
		if(_curl) {
			curl_easy_cleanup(_curl);
			_curl = nullptr;
		}
	}

	/**
	 * https://github.com/binance/binance-spot-api-docs/blob/master/rest-api.md#account-information-user_data
	 * @param acc_info
	 * @param recv_window
	 * @return
	 */
	bool account(AccountInformation& acc_info, const Time recv_window = 0u) noexcept {
		LOG_DEBUG("binance::rest::Connector::account()\n");

		// Request
		std::string request("timestamp=" + timestamp());

		if(recv_window) {
			request.append("&recvWindow=" + std::to_string(recv_window));
		}

		const auto signature = sign(request);

		request.append("&signature=");
		request.append(signature);

		// URL
		std::string url(_host + "/api/v3/account?");
		url.append(request);

		return do_get(url.c_str()) && parse_response(acc_info);
	}

	/**
	 * https://github.com/binance/binance-spot-api-docs/blob/master/rest-api.md#all-orders-user_data
	 * @param all_orders
	 * @param recv_window
	 * @return
	 */
	bool all_orders(AllOrders& all_orders, const String& symbol) noexcept {
		LOG_DEBUG("binance::rest::Connector::all_orders()\n");

		// Request
		std::string request("symbol=" + symbol);
		request.append("&timestamp=" + timestamp());

		const auto signature = sign(request);
		request.append("&signature=");
		request.append(signature);

		// URL
		std::string url(_host + "/api/v3/allOrders?");
		url.append(request);

		return do_get(url.c_str()) && parse_response(all_orders);
	}

	bool new_market_order(
		NewOrderResponse& response, const String& symbol, const Order::Side& side, const Float quantity
		, const Time recv_window = 0u
	                     ) noexcept {
		LOG_DEBUG("binance::rest::Connector::new_market_order()\n");

		// Request
		std::string request("symbol=" + symbol);
		request.append("&side=");

		switch(side) {
			case Order::Side::BUY:
				request.append("BUY");
				break;

			case Order::Side::SELL:
				request.append("SELL");
				break;

			default:
				LOG_CRITICAL("Unknown order side.");
				return false;
		}

		request.append("&type=MARKET");
		request.append("&quoteOrderQty=" + std::to_string(quantity));
		request.append("&timestamp=" + timestamp());

		const auto signature = sign(request);
		request.append("&signature=");
		request.append(signature);

		// URL
		std::string url(_host + "/api/v3/order?");

		return do_post(url.c_str(), request) && parse_response(response);
	}

private:

	inline static std::string timestamp() noexcept {
		timeval tv;
		gettimeofday(&tv, nullptr);
		const auto time = static_cast<Time>(tv.tv_sec * 1000u + tv.tv_usec / 1000u);
		return std::string(std::to_string(time));
	}

	inline std::string sign(const std::string& payload) const noexcept {
		const auto payload_ptr = reinterpret_cast<const unsigned char*>(payload.c_str());
		const auto payload_len = static_cast<int>(_secret_key.length());

		const auto digest = HMAC(
			EVP_sha256(),
			_secret_key.c_str(),
			payload_len,
			payload_ptr,
			payload.length(),
			NULL,
			NULL
		);

		return Utils::bin_to_hex(digest, 32u);
	}

	bool do_get(const char* url) noexcept {
		prepare(url);
		const auto err = curl_easy_perform(_curl);

		if(err != CURLE_OK) {
			LOG_ERROR("binnance::rest::Connector::do_get() '%s'\n", curl_easy_strerror(err));
		}

		return err == CURLE_OK;
	}

	bool do_post(const char* url, const std::string& post_data) {
		prepare(url);
		curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, post_data.c_str());
		const auto err = curl_easy_perform(_curl);

		if(err != CURLE_OK) {
			LOG_ERROR("binnance::rest::Connector::do_post() '%s'\n", curl_easy_strerror(err));
		}

//		LOG_INFO("%s\n", _response.c_str());

		return err == CURLE_OK;

	}

	void prepare(const char* url) noexcept {
		_response.clear();
		curl_easy_reset(_curl);

		curl_easy_setopt(_curl, CURLOPT_URL, url);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, receiver);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_response);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(_curl, CURLOPT_ENCODING, "gzip");

		if(not _http_headers.empty()) {
			struct curl_slist* item = nullptr;
			for(const auto& str : _http_headers) {
				item = curl_slist_append(item, str.c_str());
			}
			curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, item);
		}
	}


	static size_t receiver(void* content, size_t size, size_t nmemb, std::string* response) noexcept {
		response->append((char*) content, size * nmemb);
		return size * nmemb;
	}

	template <typename T>
	bool parse_response(T& struct_api) noexcept {
		bool result = false;

		Json::Value root;
		Json::Reader reader;

		if(reader.parse(_response, root)) {

//				Json::FastWriter fw;
//				LOG_INFO("response='%s'\n", fw.write(root).c_str());

			if(not root.isArray() && root.isMember("code") && root.isMember("msg")) {
				LOG_ERROR("Bad-response:");
				LOG_PLAIN(" code='%s'", root["code"].asString().c_str());
				LOG_PLAIN("msg='%s'\n", root["msg"].asString().c_str());
			} else {
				result = struct_api.parse(root);
			}

		}

		return result;
	}

};

}; // namespace rest
}; // namespace binance
