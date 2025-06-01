#include "LRUCache.h"
#include "LFUCache.h"
#include "Random.h"
#include <iostream>
#include <memory>
#include <random>

using namespace std;

// Test config
const int totalData = 5000;
const int dataMin = 1;
const int dataMax = 200;
// LRU cache
const int cacheSize = 100;
// LRU-K cache
const int historySize = 5;
const int maxAccessCount = 2;
// Cache types
using Key = int;
using Value = int;

int main() 
{
	// Cache
	//std::shared_ptr<LRUCache<Key, Value>> cache = std::make_shared<LRUCache<Key, Value>>(cache_size);
	//std::shared_ptr<LRUKCache<Key, Value>> cache = std::make_shared<LRUKCache<Key, Value>>(cacheSize, historySize, maxAccessCount);
	//std::shared_ptr<LFUCache<Key, Value>> cache = std::make_shared<LFUCache<Key, Value>>(cacheSize);
	std::shared_ptr<AlignLFUCache<Key, Value>> cache = std::make_shared<AlignLFUCache<Key, Value>>(cacheSize, 5);
	// Data
	vector<Value> data{};
	for (Value i = 0; i < totalData; ++i) {
		data.push_back(Random::get<Value>(dataMin, dataMax));
	}
	// visit
	int total = static_cast<int>(data.size());
	int read_disk = 0;
	for (auto i : data) {
		Value read = -1;
		if ((read = cache->get(i)) == Value{}) {
			cache->put(i, i);
			read = i;
			++read_disk;
		}
		std::cout << i << " ";
	}

	std::cout << std::endl;
	std::cout << "Total: " << total << " , Read disk：" << read_disk << ", hit_rate: " << (total - read_disk) / (1.0 * total) * 100 << "%" << std::endl;
	return 0;
}
