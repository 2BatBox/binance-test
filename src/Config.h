#pragma once

class Config {
public:

	static constexpr bool ColorLog = true;

	static constexpr const char* BinanceRestHost = "https://testnet.binance.vision";
	static constexpr unsigned NextAttemptSec = 5u;

	static constexpr const char* BinanceWsHost = "stream.binance.com";
	static constexpr int BinanceWsPort = 9443;

	static constexpr const char* BasicSymbol = "BNB";

	static constexpr const char* WSProtocolName = "binance-test";
	static constexpr size_t WSSessionData = 0xFFFF;
	static constexpr size_t WSRxBuffer = 0xFFFF;
	static constexpr int WSServiceTimeoutMS = 1000;
	static constexpr size_t WSProtocols_nb = 1u;

};
