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
	bool help;

	// common
	std::string all_args;

	CliConfig() noexcept {
		api_key = "S29FZQwAdpI8kyLRsmw9t3BkkKUxk9fKRM1YIMO7J3ne3uL0vcsJZEyI7i4pGJYT"; // TODO:
		secret_key = "yKZ0NuW7CRwByWkD8JxRaOkcwQIBQefAt7vGdEsb7EJPFVK5wUBcoTegamxStpIQ"; // TODO:
		currency_symbol = "BTC";
		trade_period_sec = 1u;
		help = false;
	}

	bool parse_args(int argc, char** argv) noexcept {

		static const char* short_options =
			"k:"  // API key
			"s:"  // secret key
			"c:"  // currency symbols to trade
			"p:"  // trade period seconds
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

				case 'p':
					result &= cli::Integer::parse(optarg, trade_period_sec);
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
		return result;
	}

	void print_usage(FILE* out, const char* bin) {
		CliConfig def;
		printf("usage %s -asc[h]\n", bin);

		fprintf(out, "Application options:\n");
		fprintf(out, "\t-a String. API key. (not an empty string)\n");
		fprintf(out, "\t-s String. Secret key. (not an empty string)\n");
		fprintf(out, "\t-c String. Symbol to trade. (not an empty string) [default value = '%s']\n", def.currency_symbol.c_str());
		fprintf(out, "\t-p Integer. Trade period. (greater than zero) [default value = %d]\n", def.trade_period_sec);
		fprintf(out, "\t-h Print this screen and exit.\n");

	}

};
