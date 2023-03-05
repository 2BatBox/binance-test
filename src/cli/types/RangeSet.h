#pragma once

#include "Integer.h"

#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <set>

namespace cli {

/**
 * [item[-item][,item[-item][,...]]]
 */
template <typename T>
struct RangeSet {

	static constexpr char DELIM = ',';
	static constexpr char RANGE = '-';

	std::vector<T> _items;

	void print(FILE* out = stdout) const noexcept {
		fprintf(out, "[");
		for(const auto item : _items) {
			fprintf(out, "%u ", item);
		}
		fprintf(out, "]");
	}

	inline const std::vector<T>& range() const noexcept {
		return _items;
	}

	/*
	 * @return the number of symbols which have been read or -1 in case of any errors.
	 */
	ssize_t parse(const char* arg) noexcept {

		_items.clear();

		enum State : unsigned {
			WF_PAIR, WF_FIRST, WF_SECOND, WF_DELIM, END, ERR
		};

		State state = WF_PAIR;
		size_t offset = 0;
		T item;
		T item_range;

		while(1) {

			switch(state) {
				case WF_PAIR: {
					const size_t read = Integer::parse_offset(arg + offset, item, 10);
					if(read) {
						offset += read;
						state = WF_FIRST;
					} else {
						state = END;
					}
				}
					break;

				case WF_FIRST: {
					const char ch = arg[offset++];
					switch(ch) {
						case DELIM:
							_items.push_back(item);
							state = WF_PAIR;
							break;
						case RANGE:
							state = WF_SECOND;
							break;
						default:
							_items.push_back(item);
							state = END;
							break;
					}
				}
					break;

				case WF_SECOND: {
					const size_t read = Integer::parse_offset(arg + offset, item_range, 10);
					if(read && item_range >= item) {
						for(T i = item; i <= item_range; ++i) {
							_items.push_back(i);
						}
						offset += read;
						state = WF_DELIM;
					} else {
						state = ERR;
					}
				}
					break;

				case WF_DELIM:
					if(arg[offset++] == DELIM){
						state = WF_PAIR;
					} else {
						state = END;
					}
					break;

				case END:
					return offset;

				case ERR:
					fprintf(stderr, "Bad range list '%s' +%zu\n", arg, offset);
					fprintf(stderr, "> '%s'\n", arg + offset);
					return -1;
			}
		}
		assert(false);
	}

};

}; // namespace cli
