#include "ARCCache.h"
#include "LRUCache.h"
#include "LFUCache.h"
#include "Random.h"
#include <iostream>
#include <memory>
#include <random>
#include <unordered_map>

using namespace std;

void testCache() {
	// Test config
	const int totalData = 5000;
	const int dataMin = 1;
	const int dataMax = 200;
	// LRU cache
	const int cacheSize = 100;
	// LRU-K cache
	/*const int historySize = 5;
	const int maxAccessCount = 2;*/
	// LFU
	//const int maxAverageFreq = 5;
	// ARC
	const int transformThreshold = 2;
	// Cache types
	using Key = int;
	using Value = int;

	// Cache
	//std::shared_ptr<LRUCache<Key, Value>> cache = std::make_shared<LRUCache<Key, Value>>(cache_size);
	//std::shared_ptr<LRUKCache<Key, Value>> cache = std::make_shared<LRUKCache<Key, Value>>(cacheSize, historySize, maxAccessCount);
	//std::shared_ptr<LFUCache<Key, Value>> cache = std::make_shared<LFUCache<Key, Value>>(cacheSize);
	//std::shared_ptr<AlignLFUCache<Key, Value>> cache = std::make_shared<AlignLFUCache<Key, Value>>(cacheSize, maxAverageFreq);
	std::shared_ptr<ARCCache<Key, Value>> cache = std::make_shared<ARCCache<Key, Value>>(cacheSize, transformThreshold);
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
}

void testHashList() {
	using Key = int;
	using Value = int;
	using Node = ARCNode<Key, Value>;
	using NodePtr = shared_ptr<Node>;

	HashLink<Key, Value> list;
	unordered_map<Key, NodePtr> map;
	cout << "========== Insert ==========" << endl;
	for (int i = 0; i < 10; ++i) {
		auto node = make_shared<ARCNode<int, int>>(i, i, 1);
		cout << "Insert Node(Key=" << node->_key << ", Value=" << node->_value << ")" << endl;
		list.headInsert(node);
		map[i] = node;
	}
	cout << "========== Remove ==========" << endl;
	while (!list.isEmpty()) {
		auto node = list.tailRemove().lock();
		cout << "Remove Node(Key=" << node->_key << ", Value=" << node->_value << ")" << endl;
		map.erase(node->_key);
	}
}

int main() 
{
	//testHashList();
	testCache();
	return 0;
}
