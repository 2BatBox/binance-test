#pragma once

#include <jsoncpp/json/json.h>
#include "../types.h"

namespace binance {
namespace ws {


/**
 * https://github.com/binance/binance-spot-api-docs/blob/master/web-socket-streams.md#individual-symbol-ticker-streams
 */
struct SymbolTicker {
	Time eventTime = 0;
	String symbol;
	Float priceChange = 0.0;
	Float priceChangePercent = 0.0;
	Float weightedAveragePrice = 0.0;
	Float firstTrade = 0.0;
	Float lastPrice = 0.0;
	Float lastQuantity = 0.0;
	Float bestBidPrice = 0.0;
	Float bestBidQuantity = 0.0;
	Float bestAskPrice = 0.0;
	Float bestAskQuantity = 0.0;
	Float openPrice = 0.0;
	Float highPrice = 0.0;
	Float lowPrice = 0.0;
	Float totalTradedBase = 0.0;
	Float totalTradedQuote = 0.0;
	Time statisticsPpenTime = 0u;
	Time statisticsCloseTime = 0u;
	UInteger firstTradeID = 0u;
	UInteger lastTradeID = 0u;
	UInteger totalNumberOfTrades = 0u;

	bool parse(const Json::Value& root) {
		eventTime = root["E"].asUInt64();
		symbol = root["s"].asString();
		priceChange = std::stod(root["p"].asString());
		priceChangePercent = std::stod(root["P"].asString());
		weightedAveragePrice = std::stod(root["w"].asString());
		firstTrade = std::stod(root["x"].asString());
		lastPrice = std::stod(root["c"].asString());
		lastQuantity = std::stod(root["Q"].asString());
		bestBidPrice = std::stod(root["b"].asString());
		bestBidQuantity = std::stod(root["B"].asString());
		bestAskPrice = std::stod(root["a"].asString());
		bestAskQuantity = std::stod(root["A"].asString());
		openPrice = std::stod(root["o"].asString());
		highPrice = std::stod(root["h"].asString());
		lowPrice = std::stod(root["l"].asString());
		totalTradedBase = std::stod(root["v"].asString());
		totalTradedQuote = std::stod(root["q"].asString());
		statisticsPpenTime = root["O"].asUInt64();
		statisticsCloseTime = root["C"].asUInt64();
		firstTradeID = root["C"].asUInt64();
		firstTradeID = root["L"].asUInt64();
		totalNumberOfTrades = root["n"].asUInt64();

		return validate();
	}

	inline bool validate() const noexcept {
		return true;
	}

	void dump() const noexcept {}

};

}; // namespace ws
}; // namespace binance
