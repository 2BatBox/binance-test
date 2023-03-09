#pragma once

#include <string>
#include <ctime>
#include <random>

#include "../binance/rest/Connector.h"
#include "../binance/ws/Connector.h"
#include "../binance/ws/api.h"

class AppDefault {

	static constexpr unsigned PriceUpdateTimeoutSec = 10u;
	static constexpr unsigned AttemptCounterMax = 5u;

	// -----------------------------
	// State machine.
	// -----------------------------
	enum class State : unsigned {
		WaitForPrice,  // Wait for the first price update via WS.
		Trading,       // Wait for any of two events; price changing or the trading time out.
		Wait,          // Take a break before trading.
		Stopped,       // The trading process is stopped.
		Inconsistent
	};

	enum class Event : unsigned {
		PriceUpdated,
		Timeout,
		Stop
	};

	binance::rest::Connector& _conn_rest;
	binance::ws::Connector& _conn_ws;

	const std::string _symbol;
	const double _price_trigger_percent;
	const unsigned _trade_period_sec;
	const unsigned _wait_period_sec;
	const double  _quantity;

	State _state;

	binance::rest::AccountInformation _acc_info;
	double _price_start;
	double _price_last;
	std::time_t _next_event;

public:

	AppDefault(const AppDefault&) = delete;
	AppDefault& operator=(const AppDefault&) = delete;

	AppDefault(AppDefault&&) = delete;
	AppDefault& operator=(AppDefault&&) = delete;

	AppDefault(binance::rest::Connector& conn_rest, binance::ws::Connector& conn_ws, const CliConfig& cli) noexcept :
		_conn_rest(conn_rest),
		_conn_ws(conn_ws),
		_symbol(cli.currency_symbol),
		_price_trigger_percent(cli.price_trigger_percent),
		_trade_period_sec(cli.trade_period_sec),
		_wait_period_sec(cli.wait_period_sec),
		_quantity(cli.quantity),
		_state(State::WaitForPrice),
		_next_event(time_now_sec() + PriceUpdateTimeoutSec) {

		LOG_DEBUG("AppDefault::AppDefault()\n");
		srand(time_now_sec()); // TODO: std::random would be a better way to do that
	}

	~AppDefault() noexcept {
		LOG_DEBUG("AppDefault::~AppDefault()\n");
	}

	bool init() noexcept {
		LOG_DEBUG("AppDefault::init()\n");

		// Getting the account information
		if(not get_account_info(_acc_info, AttemptCounterMax)) {
			LOG_ERROR("Failed to get the account information.\n");
			return false;
		}
		_acc_info.dump();

		if(not is_available_symbol()) {
			LOG_ERROR("Symbol '%s' is not available to trade.\n", _symbol.c_str());
			return false;
		}

		// Register a price watcher callback.
		if(not _conn_ws.register_ticker(cb_ticker, this, _symbol)) {
			LOG_ERROR("Fail to register the ticker listener.");
			return false;
		}

		_state = State::WaitForPrice;
		return true;
	}

	inline bool service() noexcept {
		_conn_ws.service();
		if(time_now_sec() > _next_event) {
			handle_event(Event::Timeout);
		}
		return _state != State::Inconsistent;
	}

	void finit() noexcept {
		handle_event(Event::Stop);
	}

private:

	bool get_account_info(binance::rest::AccountInformation& info, unsigned attempts) noexcept {
		bool result = true;
		while(not _conn_rest.account(info)) {
			LOG_ERROR("Failed to get the account information.");
			LOG_PLAIN(" Next attempt in %u seconds.", Config::NextAttemptSec);
			LOG_PLAIN(" %u attempts left.\n", attempts);
			attempts--;
			sleep(Config::NextAttemptSec);
			if(attempts == 0) {
				return false;
			}
		}
		return result;
	}

	static int cb_ticker(void* instance, const Json::Value& root) noexcept {
		int err = EXIT_FAILURE;
		binance::ws::SymbolTicker ticker;
		try{
			ticker.parse(root);
			auto obj = reinterpret_cast<AppDefault*>(instance);
			obj->price_update(ticker.lastPrice);
			err = EXIT_SUCCESS;
		} catch(std::exception& e) {
			LOG_ERROR("AppDefault::cb_ticker() : %s\n", e.what());
		}

		return err;
	}

	static inline std::time_t time_now_sec() noexcept {
		return std::time(nullptr);
	}

	bool is_available_symbol() noexcept {
		bool result = false;
		for(const auto& item : _acc_info.balances) {
			std::string tmp_str(item.asset);
			Utils::string_to_upper(tmp_str);
			if(tmp_str == _symbol){
				result = true;
				break;
			}
		}
		return result;
	}

	void price_update(double price) noexcept {
		_price_last = price;
		handle_event(Event::PriceUpdated);
	}

	inline void handle_event(const Event event) noexcept {

		switch(_state) {

			// ------------------
			// WaitForPrice
			// ------------------
			case State::WaitForPrice:

				switch(event) {
					case Event::PriceUpdated: {
						const unsigned timeout_sec =
							std::abs(std::rand()) % _wait_period_sec; // TODO: is not uniform distributed
						LOG_DEBUG("The price for symbol '%s' is obtained %f.\n", _symbol.c_str(), _price_last);
						LOG_DEBUG("Waiting for %u seconds before start trading...\n", timeout_sec)
						state_transition(State::Wait, timeout_sec);
					}
						break;

					case Event::Stop:
						LOG_DEBUG("Stop waiting by user.\n");
						state_transition(State::Stopped, 0);
						break;

					default:
						break;
				}
				break;


			// ------------------
			// Wait
			// ------------------
			case State::Wait:

				switch(event) {
					case Event::Timeout:
						_price_start = _price_last;
						LOG_DEBUG("Start trading...\n");
						if(action_buy()) {
							state_transition(State::Trading, _trade_period_sec);
						} else {
							state_transition(State::Inconsistent, 0u);
						}
						break;

					case Event::Stop:
						LOG_DEBUG("Stop waiting by user.\n");
						state_transition(State::Stopped, 0);
						break;

					default:
						break;
				}
				break;

			// ------------------
			// Trading
			// ------------------
			case State::Trading:

				switch(event) {
					case Event::Timeout:
						LOG_DEBUG("Stop trading by timeout.\n");
						if(action_sell()) {
							state_transition(State::Trading, _trade_period_sec);
						} else {
							state_transition(State::Inconsistent, 0u);
						}
						state_transition(State::Wait, _wait_period_sec);
						break;

					case Event::PriceUpdated: {
						const double price_delta = _price_last - _price_start;
						const double price_delta_percent = (price_delta / _price_start) * 100.;

						action_print_price(price_delta, price_delta_percent);

						if(std::abs(price_delta_percent) > _price_trigger_percent) {
							LOG_DEBUG("Stop trading by price trigger.\n");
							action_sell();
							state_transition(State::Wait, _wait_period_sec);
						}
					}
						break;

					case Event::Stop:
						LOG_DEBUG("Stop trading by user.\n");
						state_transition(State::Stopped, 0);
						break;

				}
				break;

			// ------------------
			// Stopped
			// ------------------
			case State::Stopped:
				break;

			// ------------------
			// Inconsistent
			// ------------------
			case State::Inconsistent:
				break;

		}

	}

	inline void state_transition(const State state_new, unsigned timeout) noexcept {
		if(state_new == State::Inconsistent) {
			LOG_CRITICAL("Inconsistent state has been achieved. Giving up...\n");
		}
		_next_event = time_now_sec() + timeout;
		_state = state_new;
	}

	void action_print_price(const double price_delta, const double price_delta_percent) noexcept {
		LOG_DEBUG("price=");
		LOG_LESS_GREATER_FLOAT(_price_last, _price_start);
		LOG_PLAIN("  price-delta=");
		LOG_LESS_GREATER_FLOAT(price_delta, .0);
		LOG_PLAIN("  price-delta-percent=");
		LOG_LESS_GREATER_FLOAT(price_delta_percent, .0);
		LOG_PLAIN("\n");
	}

	bool action_print_stats() noexcept {
		binance::rest::AccountInformation info;
		bool result = get_account_info(info, AttemptCounterMax);
		if(result) {
			result = false;
			double balance = .0;
			for(const auto& sym : info.balances) {
				if(sym.asset == _symbol) {
					balance = std::stod(sym.free);
					result = true;
				}
			}
			if(result) {
				LOG_DEBUG("Balance change : ");
				LOG_LESS_GREATER_FLOAT(balance, balance);
				LOG_PLAIN("\n")
			}
		}
		return result;
	}

	bool action_buy() noexcept {
		LOG_DEBUG("buy('%s' %f)\n", _symbol.c_str(), _quantity);
		return true;
	}

	bool action_sell() noexcept {
		LOG_DEBUG("sell('%s' %f)\n", _symbol.c_str(), _quantity);
		action_print_stats();
		LOG_DEBUG("Waiting for %u seconds before start trading again...\n", _wait_period_sec)
		return true;
	}


};
