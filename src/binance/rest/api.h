#include "../types.h"

#include <vector>
#include <string>
#include <cstdio>

#include <jsoncpp/json/json.h>

#include "../../Log.h"

namespace binance {
namespace rest {

struct AccountInformation {

	struct CommissionRates {
		Float maker;
		Float taker;
		Float buyer;
		Float seller;
	};

	struct Balance {
		String asset;
		String free;
		String locked;
	};

	using Balances_t = std::vector<Balance>;
	using Permissions_t = std::vector<std::string>;

	UInteger makerCommission;
	UInteger takerCommission;
	UInteger buyerCommission;
	UInteger sellerCommission;

	CommissionRates commissionRates;

	Bool canTrade;
	Bool canWithdraw;
	Bool canDeposit;
	Bool brokered;
	Bool requireSelfTradePrevention;

	Time updateTime;
	String accountType;

	Balances_t balances;
	Permissions_t permissions;

	bool parse(const Json::Value& root) {

		makerCommission = root["makerCommission"].asUInt64();
		takerCommission = root["takerCommission"].asUInt64();
		buyerCommission = root["buyerCommission"].asUInt64();
		sellerCommission = root["sellerCommission"].asUInt64();

		const auto comm_rates = root["commissionRates"];
		commissionRates.maker = std::stod(comm_rates["maker"].asString());
		commissionRates.taker = std::stod(comm_rates["taker"].asString());
		commissionRates.buyer = std::stod(comm_rates["buyer"].asString());
		commissionRates.seller = std::stod(comm_rates["seller"].asString());

		canTrade = root["canTrade"].asBool();
		canWithdraw = root["canWithdraw"].asBool();
		canDeposit = root["canDeposit"].asBool();
		brokered = root["brokered"].asBool();
		requireSelfTradePrevention = root["requireSelfTradePrevention"].asBool();

		updateTime = root["updateTime"].asUInt64();
		accountType = root["accountType"].asString();

		const auto bal = root["balances"];
		const auto bal_nb = bal.size();

		balances.clear();
		for(Json::ArrayIndex idx = 0; idx < bal_nb; ++idx) {
			const auto record = bal[idx];
			Balance item {record["asset"].asString(), record["free"].asString(), record["locked"].asString()};
			balances.push_back(item);
		}

		const auto perm = root["permissions"];
		const auto perm_nb = perm.size();
		permissions.clear();
		for(Json::ArrayIndex idx = 0; idx < perm_nb; ++idx) {
			permissions.emplace_back(perm[idx].asString());
		}

		return validate();
	}

	inline bool validate() const noexcept {
		return true;
	}

	void dump() const noexcept {
		LOG_INFO("==== AccountInformation ====\n");
		LOG_INFO("  makerCommission   : %zu\n", makerCommission);
		LOG_INFO("  takerCommission   : %zu\n", takerCommission);
		LOG_INFO("  buyerCommission   : %zu\n", buyerCommission);
		LOG_INFO("  sellerCommission  : %zu\n", sellerCommission);

		LOG_INFO("  commissionRates : ");
		LOG_PLAIN(" maker='%f'", commissionRates.maker);
		LOG_PLAIN(" taker='%f'", commissionRates.taker);
		LOG_PLAIN(" buyer='%f'", commissionRates.buyer);
		LOG_PLAIN(" seller='%f'\n", commissionRates.seller);

		LOG_INFO("  canTrade    : %d\n", canTrade);
		LOG_INFO("  canWithdraw : %d\n", canWithdraw);
		LOG_INFO("  canDeposit  : %d\n", canDeposit);
		LOG_INFO("  brokered    : %d\n", brokered);
		LOG_INFO("  requireSelfTradePrevention  : %d\n", requireSelfTradePrevention);

		LOG_INFO("  updateTime  : %zu\n", updateTime);
		LOG_INFO("  accountType : '%s'\n", accountType.c_str());

		LOG_INFO("  balances :\n");
		for(const auto& item : balances) {
			LOG_INFO("    asset='%s' free='%s' locked='%s'\n", item.asset.c_str(), item.free.c_str(), item.locked.c_str());
		}

		LOG_INFO("  permissions : ");
		for(const auto& item : permissions) {
			LOG_PLAIN(" '%s'", item.c_str());
		}
		LOG_PLAIN("\n");
	}

	bool get_balance(std::string symbol, double& balance) const noexcept {
		Utils::string_to_lower(symbol);
		for(const auto& sym : balances) {
			std::string asset = sym.asset;
			Utils::string_to_lower(asset);
			if(asset == symbol) {
				balance = std::stod(sym.free);
				return true;
			}
		}
		return false;
	}

};


struct Order {
	String	 symbol;
	SInteger orderId;
	SInteger orderListId;
	String	 clientOrderId;
	Float	 price;
	Float	 origQty;
	Float	 executedQty;
	Float	 cummulativeQuoteQty;
	String	 status;
	String	 timeInForce;
	String	 type;
	String	 side;
	Float	 stopPrice;
	Float	 icebergQty;
	Time	 time;
	Time	 updateTime;
	Bool	 isWorking;
	Float	 origQuoteOrderQty;
	Time	 workingTime;
	String	 selfTradePreventionMode;
	SInteger preventedMatchId;
	Float	 preventedQuantity;

	enum class Side {
		BUY,
		SELL
	};

	bool parse(const Json::Value& root) {
		symbol = root["symbol"].asString();
		orderId = root["orderId"].asLargestInt();
		orderListId = root["orderListId"].asLargestInt();
		clientOrderId = root["clientOrderId"].asString();
		price = std::stod(root["price"].asString());
		origQty = std::stod(root["origQty"].asString());
		executedQty = std::stod(root["executedQty"].asString());
		cummulativeQuoteQty = std::stod(root["cummulativeQuoteQty"].asString());
		status = root["status"].asString();
		timeInForce = root["timeInForce"].asString();
		symbol = root["symbol"].asString();
		side = root["side"].asString();
		stopPrice = std::stod(root["stopPrice"].asString());
		icebergQty = std::stod(root["icebergQty"].asString());
		time = root["icebergQty"].asLargestUInt();
		isWorking = root["isWorking"].asBool();
		origQuoteOrderQty = std::stod(root["origQuoteOrderQty"].asString());
		workingTime = root["workingTime"].asLargestUInt();
		selfTradePreventionMode = root["symbol"].asString();
		preventedMatchId = root["preventedMatchId"].asLargestInt();
		preventedQuantity = std::stod(root["preventedQuantity"].asString());

		return validate();
	}

	inline bool validate() const noexcept {
		return true;
	}

	void dump() const noexcept {
		LOG_INFO("==== Order ====\n");
		LOG_INFO("symbol = '%s'", symbol.c_str());
	}
};

struct AllOrders {
	std::vector<Order> orders;

	bool parse(const Json::Value& root) {
		const auto size = root.size();
		orders.resize(size);
		for(Json::ArrayIndex idx = 0; idx < size; ++idx) {
			if(orders[idx].parse(root[idx])) {
				return false;
			}
		}
		return true;
	}

	void dump() const noexcept {
		for(const auto& item : orders) {
			item.dump();
		}
	}
};

struct NewOrderResponse {

	String	 symbol;
	SInteger orderId;
	String	 clientOrderId;

	bool parse(const Json::Value& root) {
		symbol = root["symbol"].asString();
		orderId = root["orderId"].asLargestInt();
		clientOrderId = root["clientOrderId"].asString();
		return true;
	}

	void dump() const noexcept {
		LOG_INFO("NewOrderResponse :");
		LOG_PLAIN(" symbol='%s'", symbol.c_str());
		LOG_PLAIN(" orderId=%zu", orderId);
		LOG_PLAIN(" clientOrderId='%s'", clientOrderId.c_str());
		LOG_PLAIN("\n");
	}

};

}; // namespace rest
}; // namespace binance
