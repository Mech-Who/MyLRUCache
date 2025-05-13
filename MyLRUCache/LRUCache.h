#ifndef LRUCACHE_H
#define LRUCACHE_H

#include"LRUNode.h"
#include <unordered_map>

class LRUCache{
private:
	using NodePtr = LRUNode*;
	using NodeMap = std::unordered_map<int, NodePtr>;
	// time to live
	const int _ttl = 10;
	// Cache 容量
	int _capacity;
	// linked list 哨兵节点，linked list按照最近访问记录节点，head指向最久未使用的节点，tail指向最近使用的节点
	LRUNode* _head = nullptr;
	LRUNode* _tail = nullptr;
	// hashmap，hashmap的 <int,Node *> 结构是为了方便与双向链表进行交互
	NodeMap _map;
public:
	LRUCache(int capacity, int ttl);
	~LRUCache();
	int get(int key);
	void put(int key, int value);

	void remove(NodePtr node);
	void insert(int key, int value);
};

#endif // LRUCACHE_H
