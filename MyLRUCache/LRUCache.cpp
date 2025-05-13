#include "LRUCache.h"

using NodePtr = LRUNode*;

LRUCache::LRUCache(int capacity) : _capacity(capacity){}

LRUCache::~LRUCache() 
{
	NodePtr node = _head;
	while (node != nullptr) {
		NodePtr nextNode = node->_next;
		delete node;
		node = nextNode;
	}
	_head = nullptr;
	_tail = nullptr;
}

int LRUCache::get(int key)
{
	if (_map.find(key) != _map.end()) {
		NodePtr node = _map[key];
		// 下面完成的是删除双向链表及 hash 中原有的点，并将该节点加入最近使用的表尾 R 的前驱操作
		remove(node);
		insert(node->_key, node->_value);
		return node->_value;
	}
	else return -1;
}

void LRUCache::put(int key, int value) 
{
	if (_map.find(key) != _map.end()) {
		// key exists, update value
		NodePtr node = _map[key];
		// 下面完成的是删除双向链表及 hash 中原有的点，并将该节点更新 value 值后加入最近使用的表尾 R 的前驱操作
		remove(node);
		insert(node->_key, node->_value);
	}
	else {
		// key does not exist, create new node
		NodePtr newNode = new LRUNode(key, value);
		_map[key] = newNode;
		if (_map.size() > _capacity) {
			// remove least recently used node
			NodePtr node = _head->_next;
			remove(node);
			insert(key, value);
		}
		else {
			insert(key, value);
		}
	}
}

void LRUCache::remove(NodePtr node) 
{
	node->_prev->_next = node->_next;
	node->_next->_prev = node->_prev;
	_map.erase(node->_key);
}

void LRUCache::insert(int key, int value) 
{
	NodePtr node = new LRUNode(key, value);
	_tail->_next = node;
	node->_prev = _tail;
	_tail = node;

	_map[key] = node;
}
