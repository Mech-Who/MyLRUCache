#include "LRUCache.h"
#include <iostream>

using namespace std;

int main() 
{
	LRUCache* cache = new LRUCache(10);
	vector<int> data{ 3,1,4,1,5,9,2,6,5,3,5,8,9,7,9,3,2,3 };
	int total = data.size();
	int read_disk = 0;
	for (auto i : data) {
		int read = -1;
		if ((read = cache->get(i)) == -1) {
			cache->put(i, i);
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
