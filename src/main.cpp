#include <map>
#include <vector>
#include <string>
#include <thread>
#include <csignal>

#include "binance/rest/Connector.h"
#include "binance/ws/Connector.h"
#include "CliConfig.h"

struct Context {
	binance::rest::AccountInformation acc_info;
	bool signal_abort = false;
};

Context context;

bool is_available_symbol(const Context& ctx, const std::string& symbol) noexcept {
	bool result = false;
	for(const auto& item : ctx.acc_info.balances) {
		std::string tmp_str(item.asset);
		Utils::string_to_upper(tmp_str);
		if(tmp_str == symbol){
			result = true;
			break;
		}
	}
	return result;
}

int cb_ticker(const Json::Value& root) noexcept {
	LOG_INFO("cb_ticker...");
	return EXIT_SUCCESS;
}

void signal_handler(int signum) {
	if(signum == SIGINT || signum == SIGTERM) {
		LOG_INFO("Abort signal.");
		context.signal_abort = true;
	}
}


int application(const CliConfig& cli) noexcept {

	auto culr_handler = curl_easy_init();
	if(culr_handler == nullptr) {
		LOG_ERROR("curl_easy_init() fails.");
		return EXIT_FAILURE;
	}

	if(cli.currency_symbol == Config::BasicSymbol) {
		LOG_ERROR("Basic symbol '%s' is not allowed to trade.", Config::BasicSymbol);
		return EXIT_FAILURE;
	}

	binance::rest::Connector rest_conn(culr_handler, Config::BinanceRestHost, cli.api_key, cli.secret_key);

	// Getting
	while(not rest_conn.account(context.acc_info)) {
		LOG_INFO("Failed to get the account information. Next attempt in %u seconds.", Config::NextAttemptSec);
		sleep(Config::NextAttemptSec);
	}
	context.acc_info.dump();

	if(not is_available_symbol(context, cli.currency_symbol)) {
		LOG_ERROR("Symbol '%s' is not available to trade.", cli.currency_symbol.c_str());
		return EXIT_FAILURE;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	binance::ws::Connector ws_conn;
	if(not ws_conn.init()) {
		LOG_ERROR("WebSocket initializing failure.");
		return EXIT_FAILURE;
	}

	while(not ws_conn.register_ticker(cb_ticker, cli.currency_symbol)) {
		LOG_INFO("Failed to get the register the ticker listener. Next attempt in %u seconds.", Config::NextAttemptSec);
		sleep(Config::NextAttemptSec);
	}

	LOG_INFO("Entering the service loop...");
	while(not context.signal_abort){
		ws_conn.service();
	}
	LOG_INFO("Leaving the service loop.");

	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	int err = EXIT_FAILURE;

	CliConfig cli;

	if(cli.parse_args(argc, argv)) {

		if(cli.help) {
			cli.print_usage(stdout, argv[0]);
			err = EXIT_SUCCESS;
		} else {

			// Initialize all the dependencies.
			err = curl_global_init(CURL_GLOBAL_DEFAULT);
			if(err == EXIT_SUCCESS) {
				err = application(cli);
				curl_global_cleanup();
			} else {
				LOG_CRITICAL("curl_global_init() fails.");
			}
		}

	} else {
		cli.print_usage(stderr, argv[0]);
	}

	return err;
}
