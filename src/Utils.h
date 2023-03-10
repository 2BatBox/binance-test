#pragma once

#include <string>
#include <algorithm>
#include <ctime>
#include "Config.h"

class Utils {
public:

	static inline void string_to_lower(std::string& str) noexcept {
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	}

	static inline void string_to_upper(std::string& str) noexcept {
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	}

	static std::string bin_to_hex(const void* bin, const size_t bin_bytes_nb) noexcept {
		static constexpr char base_16 [] = "0123456789ABCDEF";
		auto u8ptr = static_cast<const uint8_t*>(bin);
		std::string result;
		result.resize(bin_bytes_nb * 2u);
		for(size_t i = 0; i < bin_bytes_nb; ++i) {
			result[i * 2u] = base_16[(u8ptr[i] >> 4u) & 0x0F];
			result[i * 2u + 1u] = base_16[u8ptr[i] & 0x0F];
		}
		return result;
	}

	static inline std::time_t time_now_sec() noexcept {
		return std::time(nullptr);
	}

};
