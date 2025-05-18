#include "LRUCache.h"
#include <ctime>

using NodePtr = LRUNode*;
using NodeMap = std::unordered_map<int, NodePtr>;

LRUCache::LRUCache(int capacity) : LRUCache(capacity, 10) {}

LRUCache::LRUCache(int capacity, int ttl) : _capacity(capacity), _ttl(ttl){
	_head = std::make_shared<LRUNode>(-1, -1, ttl);
	_tail = std::make_shared<LRUNode>(-1, -1, ttl);
	_head->_next = _tail;
	_tail->_prev = _head;
}

LRUCache::~LRUCache(){}

int LRUCache::get(int key)
{
	if (_map.find(key) != _map.end()) {
		NodePtr node = _map[key];
		if (difftime(node->_expiredTime, time(nullptr)) <= 0)
		{
			remove(node);
			return -1;
		}
		// 下面完成的是删除双向链表及 hash 中原有的点，并将该节点加入最近使用的表尾 R 的前驱操作
		remove(node);
		// 保持节点过期时间不变
		int timeLeft = static_cast<int>(difftime(node->_expiredTime, time(nullptr)));
		insert(node->_key, node->_value, timeLeft);
		return node->_value;
	}
	else return -1;
}

void LRUCache::put(int key, int value, int ttl) 
{
	if (_map.find(key) != _map.end()) {
		// key exists, update value
		NodePtr node = _map[key];
		// 下面完成的是删除双向链表及 hash 中原有的点，并将该节点更新 value 值后加入最近使用的表尾 R 的前驱操作
		remove(node);
		insert(node->_key, node->_value, ttl);
	}
	else {
		// key does not exist, create new node
		if (_map.size() > _capacity) {
			NodeMap::iterator it;
			bool hasExpired = false;
			for (it = _map.begin(); it != _map.end(); ++it) {
				if (difftime(it->second->_expiredTime, time(nullptr)) <= 0) {
					hasExpired = true;
					break;
				}
			}
			if (hasExpired) {
				remove(it->second);
				insert(key, value, ttl);
			}
			else {
				// remove least recently used node
				NodePtr node = _head->_next;
				remove(node);
				insert(key, value, ttl);
			}
		}
		else {
			insert(key, value, ttl);
		}
	}
}

void LRUCache::remove(NodePtr node) 
{
	auto prev = node->_prev.lock(); // 不能直接使用weak_ptr，要用lock()先转换为shared_ptr
	prev->_next = node->_next;
	node->_next->_prev = node->_prev;
	node->_next = nullptr; // 清空next_指针，彻底断开节点与链表的连接
	_map.erase(node->_key);
}

void LRUCache::insert(int key, int value, time_t ttl) 
{
	NodePtr node = std::make_shared<LRUNode>(key, value, time(nullptr)+ ttl);

	node->_next = _tail;
	node->_prev = _tail->_prev;
	_tail->_prev.lock()->_next = node;
	_tail->_prev = node;

	_map[key] = node;
}
