#include "LRUCache.h"
#include "Random.h"
#include <iostream>
#include <random>

using namespace std;

int main() 
{
	LRUCache* cache = new LRUCache(100);
	vector<int> data{};
	for (int i = 0; i < 1000; ++i) {
		data.push_back(Random::get<int>(1, 100));
	}
	int total = static_cast<int>(data.size());
	int read_disk = 0;
	for (auto i : data) {
		int read = -1;
		if ((read = cache->get(i)) == -1) {
			cache->put(i, i, 13);
			read = i;
			++read_disk;
		}
		std::cout << i << " ";
	}
	std::cout << std::endl;
	std::cout << "Total: " << total << " , Read disk：" << read_disk << ", hit_rate: " << (total - read_disk) / (1.0 * total) * 100 << "%" << std::endl;
	delete cache;
	return 0;
}
