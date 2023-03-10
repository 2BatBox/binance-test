#include <csignal>

#include "CliConfig.h"
#include "binance/rest/Connector.h"
#include "binance/ws/Connector.h"

#include "app/AppDefault.h"

bool signal_abort = false;

void signal_handler(int signum) {
	if(signum == SIGINT || signum == SIGTERM) {
		LOG_DEBUG("Abort signal.\n");
		if(not signal_abort) {
			signal_abort = true;
		} else {
			exit(EXIT_FAILURE);
		}
	}

}

template <typename Application>
int run(const CliConfig& cli, CURL* culr_handler) noexcept {

	int err = EXIT_SUCCESS;

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	binance::rest::Connector rest_conn(culr_handler, Config::BinanceRestHost, cli.api_key, cli.secret_key);

	binance::ws::Connector ws_conn;
	if(not ws_conn.init()) {
		LOG_CRITICAL("WebSocket initializing failure.\n");
		return EXIT_FAILURE;
	}

	Application app(rest_conn, ws_conn, cli);

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

	app.finit();

	return err;
}

int main(int argc, char** argv) {
	const auto bin = argv[0];

	CliConfig cli;
	if(not cli.parse_args(argc, argv)) {
		cli.print_usage(stderr, bin);
		return EXIT_FAILURE;
	}

	if(cli.help) {
		cli.print_usage(stdout, bin);
		return EXIT_SUCCESS;
	}

	if(cli.currency_symbol == Config::BasicSymbol) {
		LOG_ERROR("Basic symbol '%s' is not allowed to trade.\n", Config::BasicSymbol);
		return EXIT_FAILURE;
	}

	if(curl_global_init(CURL_GLOBAL_DEFAULT)) {
		LOG_CRITICAL("curl_global_init() fails.\n");
		return EXIT_FAILURE;
	}

	auto culr_handler = curl_easy_init();
	if(culr_handler == nullptr) {
		LOG_CRITICAL("curl_easy_init() fails.\n");
		return EXIT_FAILURE;
	}

	const auto err = run<AppDefault>(cli, culr_handler);

	curl_global_cleanup();

	return err;
}
