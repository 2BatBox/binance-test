#pragma once

#include <string>

#include "../binance/rest/Connector.h"
#include "../binance/ws/Connector.h"
#include "../binance/ws/api.h"

class AppSymbolTracer {

	static constexpr unsigned AttemptCounterMax = 5u;

	binance::rest::Connector& _conn_rest;
	binance::ws::Connector& _conn_ws;
	const std::string _symbol;

	binance::rest::AccountInformation _acc_info;
	binance::ws::SymbolTicker _ticker;
	binance::ws::SymbolTicker _ticker_prev;

public:

	AppSymbolTracer(const AppSymbolTracer&) = delete;
	AppSymbolTracer& operator=(const AppSymbolTracer&) = delete;

	AppSymbolTracer(AppSymbolTracer&&) = delete;
	AppSymbolTracer& operator=(AppSymbolTracer&&) = delete;

	AppSymbolTracer(
		binance::rest::Connector& conn_rest,
		binance::ws::Connector& conn_ws,
		std::string symbol
	               ) noexcept :
		_conn_rest(conn_rest),
		_conn_ws(conn_ws),
		_symbol(std::move(symbol)) {
		LOG_DEBUG("AppSymbolTracer::AppSymbolTracer()\n");
	}

	bool init() noexcept {
		LOG_DEBUG("AppSymbolTracer::init()\n");

		// Getting the account information
		auto attempt_counter = AttemptCounterMax;
		while(not _conn_rest.account(_acc_info)) {
			LOG_ERROR("Failed to get the account information.");
			LOG_PLAIN(" Next attempt in %u seconds.", Config::NextAttemptSec);
			LOG_PLAIN(" %u attempts left.\n", attempt_counter);
			attempt_counter--;
			sleep(Config::NextAttemptSec);
			if(attempt_counter == 0) {
				return false;
			}
		}
		_acc_info.dump();

		if(not is_available_symbol(_symbol)) {
			LOG_ERROR("Symbol '%s' is not available to trade.\n", _symbol.c_str());
			return false;
		}

		// Register a price watcher callback.
		attempt_counter = AttemptCounterMax;
		while(not _conn_ws.register_ticker(cb_ticker, this, _symbol)) {
			LOG_ERROR("Fail to register the ticker listener.");
			LOG_PLAIN(" Next attempt in %u seconds.", Config::NextAttemptSec);
			LOG_PLAIN(" %u attempts left.\n", attempt_counter);

			attempt_counter--;
			sleep(Config::NextAttemptSec);
			if(attempt_counter == 0) {
				return false;
			}
		}

		return true;
	}

	inline bool service() noexcept {
		_conn_ws.service();
		return true;
	}

	~AppSymbolTracer() noexcept {
		LOG_DEBUG("AppSymbolTracer::~AppSymbolTracer()\n");
	}

private:


	static int cb_ticker(void* instance, const Json::Value& root) noexcept {
		auto obj = reinterpret_cast<AppSymbolTracer*>(instance);
		return obj->cb_ticker_instance(root);
	}

	void ticker_dump_diff(binance::ws::SymbolTicker tkr, binance::ws::SymbolTicker tkr_old) const noexcept {
		LOG_INFO("Ticker : ");
		LOG_PLAIN("'%s'", tkr.symbol.c_str());
		LOG_UD_FLOAT(lastPrice, tkr, tkr_old);
		LOG_UD_FLOAT(priceChange, tkr, tkr_old);
		LOG_UD_FLOAT(priceChangePercent, tkr, tkr_old);
		LOG_PLAIN("\n");
	}

	int cb_ticker_instance(const Json::Value& root) noexcept {
		try{
			_ticker.parse(root);
			ticker_dump_diff(_ticker, _ticker_prev);
			_ticker_prev = _ticker;
		} catch(std::exception& e) {
			LOG_ERROR("binance::ws::SymbolTicker::parse() : %s\n", e.what());
		}
		return EXIT_SUCCESS;
	}

	bool is_available_symbol(const std::string& symbol) noexcept {
		bool result = false;
		for(const auto& item : _acc_info.balances) {
			std::string tmp_str(item.asset);
			Utils::string_to_upper(tmp_str);
			if(tmp_str == symbol){
				result = true;
				break;
			}
		}
		return result;
	}


};
