#pragma once

#include <cstdio>
#include <cstring>
#include <getopt.h>
#include <vector>
#include <sstream>

#include "cli/types/Integer.h"
#include "cli/types/Float.h"
#include "Utils.h"

struct CliConfig {


	// application
	std::string api_key;
	std::string secret_key;
	std::string currency_symbol;
	unsigned trade_period_sec;
	unsigned wait_period_sec;
	double price_trigger_percent;
	double quantity;
	bool help;

	// common
	std::string all_args;

	CliConfig() noexcept {
		api_key = "S29FZQwAdpI8kyLRsmw9t3BkkKUxk9fKRM1YIMO7J3ne3uL0vcsJZEyI7i4pGJYT"; // TODO:
		secret_key = "yKZ0NuW7CRwByWkD8JxRaOkcwQIBQefAt7vGdEsb7EJPFVK5wUBcoTegamxStpIQ"; // TODO:
		currency_symbol = "BTC";
		trade_period_sec = 30u;
		wait_period_sec = 15u;
		price_trigger_percent = .25;
		quantity = .001;
		help = false;
	}

	bool parse_args(int argc, char** argv) noexcept {

		static const char* short_options =
			"k:"  // API key
			"s:"  // secret key
			"c:"  // currency symbols to trade
			"t:"  // trade period seconds
			"w:"  // wait period seconds
			"p:"  // price rigger in percents.
			"q:"  // quantity to trade at once.
			"h"  // help
		;

		// collecting all the arguments
		all_args.clear();
		for(int i = 0; i < argc; ++i) {
			all_args += argv[i];
			all_args += " ";
		}

		bool result = true;
		int opt;

		while((opt = getopt(argc, argv, short_options)) != EOF) {
			switch(opt) {
				case 'a':
					api_key = std::string(optarg);
					break;

				case 's':
					secret_key = std::string(optarg);
					break;

				case 'c':
					currency_symbol = std::string(optarg);
					break;

				case 't':
					result &= cli::Integer::parse(optarg, trade_period_sec);
					break;

				case 'w':
					result &= cli::Integer::parse(optarg, wait_period_sec);
					break;

				case 'p':
					result &= cli::Float::parse(optarg, price_trigger_percent);
					break;

				case 'q':
					result &= cli::Float::parse(optarg, quantity);
					break;

				case 'h':
					help = true;
					break;

				default:
					result = false;
					break;
			}
		}

		Utils::string_to_upper(currency_symbol);

		const bool dry_run = help;
		result &= dry_run || validate();
		return result;
	}

	bool validate() const noexcept {
		bool result = true;
		result &= (not api_key.empty());
		result &= (not secret_key.empty());
		result &= (not currency_symbol.empty());
		result &= (trade_period_sec > 0u);
		result &= (wait_period_sec > 0u);
		result &= (price_trigger_percent > .0);
		result &= (quantity > .0);
		return result;
	}

	void print_usage(FILE* out, const char* bin) {
		CliConfig def;
		printf("usage %s -as[cth]\n", bin);

		fprintf(out, "Application options:\n");
		fprintf(out, "\t-a String. API key. (not an empty string)\n");
		fprintf(out, "\t-s String. Secret key. (not an empty string)\n");
		fprintf(out, "\t-c String. Symbol to trade. (not an empty string) [default value = '%s']\n", def.currency_symbol.c_str());
		fprintf(out, "\t-t Integer. Trade period seconds. (greater than zero) [default value = %d]\n", def.trade_period_sec);
		fprintf(out, "\t-t Integer. Wait period seconds. (greater than zero) [default value = %d]\n", def.wait_period_sec);
		fprintf(out, "\t-p Float. Price trigger percent. (greater than zero) [default value = %f]\n", def.price_trigger_percent);
		fprintf(out, "\t-q Float. Quantity to trade. (greater than zero) [default value = %f]\n", def.quantity);
		fprintf(out, "\t-h Print this screen and exit.\n");

	}

};
