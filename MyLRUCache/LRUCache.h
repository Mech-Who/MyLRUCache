#ifndef LRUCACHE_H
#define LRUCACHE_H

#include"LRUNode.h"
#include <memory>
#include <unordered_map>

class LRUCache{
private:
	using NodePtr = std::shared_ptr<LRUNode>;
	using NodeMap = std::unordered_map<int, NodePtr>;
	// time to live
	const int _ttl;
	// Cache 容量
	int _capacity;
	// linked list 哨兵节点，linked list按照最近访问记录节点，head指向最久未使用的节点，tail指向最近使用的节点
	std::shared_ptr<LRUNode> _head = nullptr;
	std::shared_ptr<LRUNode> _tail = nullptr;
	// hashmap，hashmap的 <int,Node *> 结构是为了方便与双向链表进行交互
	NodeMap _map;
public:
	LRUCache(int capacity);
	LRUCache(int capacity, int ttl);
	~LRUCache();
	int get(int key);
	void put(int key, int value, int ttl);

	void remove(NodePtr node);
	void insert(int key, int value, time_t ttl);
};

#endif // LRUCACHE_H
