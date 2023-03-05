#pragma once

#include <string>
#include <string_view>

#include <curl/curl.h>

#include "Api.h"
#include "../../Log.h"

namespace binance {
namespace rest {

class Connector {

	using HttpHeaders = std::vector<std::string>;

	CURL* _curl;
	const std::string _host;
	const std::string _api_key;
	const std::string _secret_key;

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
		_secret_key(std::move(secret_key)) {}

	~Connector() noexcept {
		if(_curl) {
			curl_easy_cleanup(_curl);
			_curl = nullptr;
		}
	}
	/**
	 * https://github.com/binance/binance-spot-api-docs/blob/master/rest-api.md#account-information-user_data
	 * @param recvWindow
	 * @return
	 */
	bool account(AccountInformation& acc_info, Time recvWindow = 0u) noexcept {
		// Request
		std::string request("timestamp=" + timestamp());

		if(recvWindow) {
			request.append("&recvWindow=" + std::to_string(recvWindow));
		}

		const auto signature = sign(request);

		request.append("&signature=");
		request.append(signature);

		// URL
		std::string url(_host + "/api/v3/account?");
		url.append(request);

		// HTTP headers
		HttpHeaders http_headers;
		http_headers.emplace_back(std::string("X-MBX-APIKEY: " + _api_key));

		return do_get(url.c_str(), http_headers) && parse_response(acc_info);
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

	bool do_get(const char* url, const HttpHeaders& headers) noexcept {

		curl_easy_setopt(_curl, CURLOPT_URL, url);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, receiver);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_response);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(_curl, CURLOPT_ENCODING, "gzip");

		if(not headers.empty()) {
			struct curl_slist* item = nullptr;
			for(const auto str : headers) {
				item = curl_slist_append(item, str.c_str());
			}
			curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, item);
		}

		const auto err = curl_easy_perform(_curl);

		if(err != CURLE_OK) {
			LOG_ERROR("Connector::do_get() [FAIL]");
		}

		return err == CURLE_OK;
	}


	static size_t receiver(void* content, size_t size, size_t nmemb, std::string* response) noexcept {
		response->append((char*) content, size * nmemb);
		return size * nmemb;
	}

	template <typename T>
	bool parse_response(T& struct_api) noexcept {
		bool result = false;
		try {
			Json::Value root;
			Json::Reader reader;
			if(reader.parse(_response, root)) {

				if(root.isMember("code") && root.isMember("msg")) {
					LOG_ERROR("Bad-response:");
					LOG_ERROR("\tcode='%s'", root["code"].asString().c_str());
					LOG_ERROR("\tmsg='%s'", root["msg"].asString().c_str());
				} else {
					result = struct_api.parse(root);
				}
			}

		} catch (std::exception& e) {
			LOG_INFO("JSON parsing error: %s", e.what());
		}

		return result;
	}

};

}; // namespace rest
}; // namespace binance
