#pragma once

#include <string>
#include <random>

#include "../binance/rest/Connector.h"
#include "../binance/ws/Connector.h"
#include "../binance/ws/api.h"

class AppDefault {

	static constexpr unsigned PriceUpdateTimeoutSec = 10u;

	// -----------------------------
	// State machine.
	// -----------------------------
	enum class State : unsigned {
		Init,          // The instance has been created.
		WaitForPrice,  // Wait for the first price update via WS.
		Trading,       // Wait for any of two events; price changing or the trading time out.
		Wait,          // Take a break before trading.
		Stopped        // The trading process is stopped.
	};

	enum class Event : unsigned {
		Start,
		Stop,
		PriceUpdated,
		Timeout
	};

	// ---------------------------------
	// The connectors.
	// ---------------------------------
	binance::rest::Connector& _conn_rest;
	binance::ws::Connector& _conn_ws;

	// ---------------------------------
	// The user defined trader parameters.
	// ---------------------------------
	const std::string _symbol;
	const std::string _sym_pair;
	const double _price_trigger_percent;
	const unsigned _trade_period_sec;
	const unsigned _wait_period_sec;
	const double  _quantity;

	// ---------------------------------
	// The state.
	// ---------------------------------
	State _state;

	binance::rest::AccountInformation _acc_info_init;  // The account state before trading process is started.
	binance::rest::AccountInformation _acc_info_last;  // The account state just after the last trade is done.

	double _price_last;      // The recent obtained price of the symbol.
	double _price_start;     // The symbol price before the last trade.
	std::time_t _next_event; // The time in the future that the Event::Timeout will be generated.

public:

	AppDefault(const AppDefault&) = delete;
	AppDefault& operator=(const AppDefault&) = delete;

	AppDefault(AppDefault&&) = delete;
	AppDefault& operator=(AppDefault&&) = delete;

	AppDefault(binance::rest::Connector& conn_rest, binance::ws::Connector& conn_ws, const CliConfig& cli) noexcept :
		_conn_rest(conn_rest),
		_conn_ws(conn_ws),
		_symbol(cli.currency_symbol),
		_sym_pair(Config::BasicSymbol + cli.currency_symbol),
		_price_trigger_percent(cli.price_trigger_percent),
		_trade_period_sec(cli.trade_period_sec),
		_wait_period_sec(cli.wait_period_sec),
		_quantity(cli.quantity),
		_state(State::Init),
		_next_event(Utils::time_now_sec() + PriceUpdateTimeoutSec) {

		LOG_DEBUG("AppDefault::AppDefault()\n");
		srand(Utils::time_now_sec()); // TODO: std::random would be a better way to do that
	}

	~AppDefault() noexcept {
		LOG_DEBUG("AppDefault::~AppDefault()\n");
	}

	bool init() noexcept {
		LOG_DEBUG("AppDefault::init()\n");

		// Getting the account information
		if(not _conn_rest.account(_acc_info_init)) {
			LOG_ERROR("Failed to get the account information.\n");
			return false;
		}
		_acc_info_init.dump();
		_acc_info_last = _acc_info_init;

		if(not _acc_info_init.get_balance(_symbol, _price_last)) {
			LOG_ERROR("Symbol '%s' is not available to trade.\n", _sym_pair.c_str());
			return false;
		}

		// Register a price watcher callback.
		if(not _conn_ws.register_ticker(cb_ticker, this, _sym_pair)) {
			LOG_ERROR("Fail to register the ticker listener.");
			return false;
		}

		handle_event(Event::Start);
		return true;
	}

	inline bool service() noexcept {
		_conn_ws.service();
		if(Utils::time_now_sec() > _next_event) {
			handle_event(Event::Timeout);
		}
		return _state != State::Stopped;
	}

	void finit() noexcept {
		handle_event(Event::Stop);
	}

private:

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

	inline void price_update(double price) noexcept {
		_price_last = price;
		handle_event(Event::PriceUpdated);
	}

	inline void handle_event(const Event event) noexcept {

		switch(_state) {

			// ------------------
			// Init
			// ------------------
			case State::Init:

				switch(event) {
					case Event::Start:
						LOG_DEBUG("The trading state machine is starting...\n");
						LOG_DEBUG("symbol='%s'", _sym_pair.c_str());
						LOG_PLAIN(" price_trigger_percent=%f", _price_trigger_percent);
						LOG_PLAIN(" trade_period_sec=%u", _trade_period_sec);
						LOG_PLAIN(" wait_period_sec=%u", _wait_period_sec);
						LOG_PLAIN(" quantity=%f\n", _quantity);
						state_transition(State::WaitForPrice, PriceUpdateTimeoutSec);
						break;

					case Event::PriceUpdated:
						break;

					default:
						LOG_DEBUG("Starting the trading machine has been canceled.\n");
						state_transition(State::Stopped, 0);
						break;
				}
				break;


			// ------------------
			// WaitForPrice
			// ------------------
			case State::WaitForPrice:

				switch(event) {
					case Event::PriceUpdated: {

						// TODO: 'timeout_sec' is not uniformly distributed.
						const unsigned timeout_sec = std::abs(std::rand()) % _wait_period_sec;
						LOG_DEBUG("The price for symbol '%s' is obtained %f.\n", _sym_pair.c_str(), _price_last);
						LOG_DEBUG("Waiting for %u seconds before start trading...\n", timeout_sec)
						state_transition(State::Wait, timeout_sec);
					}
						break;

					case Event::Timeout:
						LOG_DEBUG("Stop waiting for the first price updated by timeout.\n");
						state_transition(State::Stopped, 0);
						break;

					case Event::Stop:
						LOG_DEBUG("Stop waiting for the first price updated by user.\n");
						state_transition(State::Stopped, 0);
						break;

					default:
						LOG_CRITICAL("Inconsistent state transition.\n");
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
							state_transition(State::Stopped, 0u);
						}
						break;

					case Event::Stop:
						LOG_DEBUG("Stop waiting by user.\n");
						state_transition(State::Stopped, 0);
						break;

					case Event::PriceUpdated:
						break;

					default:
						LOG_CRITICAL("Inconsistent state transition.\n");
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
							state_transition(State::Wait, _wait_period_sec);
						} else {
							state_transition(State::Stopped, 0u);
						}
						break;

					case Event::PriceUpdated: {
						const double price_delta = _price_last - _price_start;
						const double price_delta_percent = (price_delta / _price_start) * 100.;

						print_price_stats(price_delta, price_delta_percent);

						if(std::abs(price_delta_percent) > _price_trigger_percent) {
							LOG_DEBUG("Stop trading by price trigger.\n");
							if(action_sell()) {
								state_transition(State::Wait, _wait_period_sec);
							} else {
								state_transition(State::Stopped, 0u);
							}
						}
					}
						break;

					case Event::Stop:
						LOG_DEBUG("Stop trading by user.\n");
						state_transition(State::Stopped, 0);
						break;

					default:
						LOG_CRITICAL("Inconsistent state transition.\n");
						break;

				}
				break;

			// ------------------
			// Stopped
			// ------------------
			case State::Stopped:
				break;
		}

	}


	inline void state_transition(const State state_new, unsigned timeout) noexcept {
		_next_event = Utils::time_now_sec() + timeout;
		_state = state_new;
	}

	bool action_buy() noexcept {
		LOG_DEBUG("buying %f of '%s'...\n", _quantity, _sym_pair.c_str());

		binance::rest::NewOrderResponse response;
		bool result = _conn_rest.new_market_order(response, _sym_pair, binance::rest::Order::Side::SELL, _quantity);
		if(result) {
			response.dump();
		}

		return result;
	}

	bool action_sell() noexcept {
		LOG_DEBUG("selling %f of '%s'...\n", _quantity, _sym_pair.c_str());

		binance::rest::NewOrderResponse response;
		bool result = _conn_rest.new_market_order(response, _sym_pair, binance::rest::Order::Side::BUY, _quantity);
		if(result) {
			response.dump();

			binance::rest::AccountInformation info;
			if(not _conn_rest.account(info)) {
				LOG_ERROR("Failed to get the account information.\n");
				return false;
			}

			LOG_DEBUG("Last trade balance delta ");
			print_trade_stats(Config::BasicSymbol, _acc_info_last, info);
			print_trade_stats(_symbol, _acc_info_last, info);
			LOG_PLAIN("\n");
			_acc_info_last = info;

			LOG_DEBUG("Last trade balance delta ");
			print_trade_stats(Config::BasicSymbol, _acc_info_last, info);
			print_trade_stats(_symbol, _acc_info_init, _acc_info_last);
			LOG_PLAIN("\n");

			LOG_DEBUG("Waiting for %u seconds before start trading again...\n", _wait_period_sec)
		}

		return result;
	}


	// ------------------------------
	// Printing stuff
	// ------------------------------

	static void print_trade_stats(
		const std::string symbol,
		const binance::rest::AccountInformation& prev,
		const binance::rest::AccountInformation& last
	                             ) noexcept {
		double balance_prev;
		double balance_last;
		if(prev.get_balance(symbol, balance_prev) && last.get_balance(symbol, balance_last)) {
			LOG_LESS_GREATER_FLOAT(balance_last - balance_prev, .0f);
			LOG_PLAIN(" %s  ", symbol.c_str());
		} else {
			LOG_CRITICAL("The balance records are missed!\n");
		}
	}

	void print_price_stats(const double price_delta, const double price_delta_percent) noexcept {
		LOG_DEBUG("'%s' price=", _sym_pair.c_str());
		LOG_LESS_GREATER_FLOAT(_price_last, _price_start);
		LOG_PLAIN("  price-delta=");
		LOG_LESS_GREATER_FLOAT(price_delta, .0);
		LOG_PLAIN("  price-delta-percent=");
		LOG_LESS_GREATER_FLOAT(price_delta_percent, .0);
		LOG_PLAIN("\n");
	}

};
