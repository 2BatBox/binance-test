#include <csignal>

#include "CliConfig.h"
#include "binance/rest/Connector.h"
#include "binance/ws/Connector.h"
#include "app/AppSymbolTracer.h"

bool signal_abort = false;

void signal_handler(int signum) {
	if(signum == SIGINT || signum == SIGTERM) {
		LOG_DEBUG("Abort signal.\n");
		signal_abort = true;
	}
}

template <typename Application>
int run(const CliConfig& cli) noexcept {

	int err = EXIT_SUCCESS;

	auto culr_handler = curl_easy_init();
	if(culr_handler == nullptr) {
		LOG_CRITICAL("curl_easy_init() fails.\n");
		return EXIT_FAILURE;
	}

	if(cli.currency_symbol == Config::BasicSymbol) {
		LOG_ERROR("Basic symbol '%s' is not allowed to trade.\n", Config::BasicSymbol);
		return EXIT_FAILURE;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	binance::rest::Connector rest_conn(culr_handler, Config::BinanceRestHost, cli.api_key, cli.secret_key);

	binance::ws::Connector ws_conn;
	if(not ws_conn.init()) {
		LOG_CRITICAL("WebSocket initializing failure.\n");
		return EXIT_FAILURE;
	}

	Application app(rest_conn, ws_conn, cli.currency_symbol);

	while(not app.init()) {
		LOG_CRITICAL("Application initialization has failed.\n");
		return EXIT_FAILURE;
	}

	LOG_DEBUG("Entering the service loop...\n");
	while(not signal_abort){
		if(not app.service()) {
			LOG_CRITICAL("Application servicing failure.\n");
			err = EXIT_FAILURE;
			break;
		}
	}
	LOG_DEBUG("Leaving the service loop.\n");

	return err;
}

int main(int argc, char** argv) {

	CliConfig cli;
	if(not cli.parse_args(argc, argv)) {
		cli.print_usage(stderr, argv[0]);
		return EXIT_FAILURE;
	}

	if(cli.help) {
		cli.print_usage(stdout, argv[0]);
		return EXIT_SUCCESS;
	}

	if(curl_global_init(CURL_GLOBAL_DEFAULT)) {
		LOG_CRITICAL("curl_global_init() fails.\n");
		return EXIT_FAILURE;
	}

	auto err = run<AppSymbolTracer>(cli);

	curl_global_cleanup();

	return err;
}
