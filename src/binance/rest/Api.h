#include "../types.h"

#include <vector>
#include <string>
#include <cstdio>

#include <jsoncpp/json/json.h>

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

		return vaidate();
	}

	inline bool vaidate() const noexcept {
		return true;
	}

	void dump() const noexcept {
		LOG_INFO("==== AccountInformation ====");
		LOG_INFO("  makerCommission   : %zu", makerCommission);
		LOG_INFO("  takerCommission   : %zu", takerCommission);
		LOG_INFO("  buyerCommission   : %zu", buyerCommission);
		LOG_INFO("  sellerCommission  : %zu", sellerCommission);

		LOG_INFO("  commissionRates");
		LOG_INFO("    maker  : %f", commissionRates.maker);
		LOG_INFO("    taker  : %f", commissionRates.taker);
		LOG_INFO("    buyer  : %f", commissionRates.buyer);
		LOG_INFO("    seller : %f", commissionRates.seller);

		LOG_INFO("  canTrade    : %d", canTrade);
		LOG_INFO("  canWithdraw : %d", canWithdraw);
		LOG_INFO("  canDeposit  : %d", canDeposit);
		LOG_INFO("  brokered    : %d", brokered);
		LOG_INFO("  requireSelfTradePrevention  : %d", requireSelfTradePrevention);

		LOG_INFO("  updateTime  : %zu", updateTime);
		LOG_INFO("  accountType : '%s'", accountType.c_str());

		LOG_INFO("  balances :");
		for(const auto& item : balances) {
			LOG_INFO("    asset='%s' free='%s' locked='%s'", item.asset.c_str(), item.free.c_str(), item.locked.c_str());
		}

		LOG_INFO("  permissions :");
		for(const auto& item : permissions) {
			LOG_INFO("    '%s'", item.c_str());
		}
	}

};

}; // namespace rest
}; // namespace binance
