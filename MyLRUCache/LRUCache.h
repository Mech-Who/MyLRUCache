#pragma once
#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <memory>
#include <unordered_map>

// 前向声明
template<typename Key, typename Value>
class LRUCache;

/****************************************
LRUNode

记录缓存数据的节点类
****************************************/
template<typename Key, typename Value>
struct LRUNode {
private:
	Key _key;
	Value _value;
	std::weak_ptr<LRUNode> _prev;
	std::shared_ptr<LRUNode> _next;
public:
	LRUNode() = delete;
	LRUNode(Key key, Value value) : _key(key), _value(value) {}

	inline Key getKey() { return _key; }
	inline Value getValue() { return _value; }
	inline void setValue(Value value) { _value = value; }

	friend class LRUCache<Key, Value>;
};

/****************************************
LRUCache

基于LRU算法的缓存算法
****************************************/

template<typename Key, typename Value>
class LRUCache{
private:
	using NodePtr = std::shared_ptr<LRUNode<Key, Value>>;
	using NodeMap = std::unordered_map<int, NodePtr>;
	// Cache 容量
	int _capacity;
	// linked list 哨兵节点，linked list按照最近访问记录节点，head指向最久未使用的节点，tail指向最近使用的节点
	std::shared_ptr<LRUNode<Key, Value>> _head = nullptr;
	std::shared_ptr<LRUNode<Key, Value>> _tail = nullptr;
	// hashmap，hashmap的 <int,Node *> 结构是为了方便与双向链表进行交互
	NodeMap _map;
public:
	LRUCache(int capacity) : _capacity(capacity) {
		_head = std::make_shared<LRUNode<Key, Value>>(Key(), Value());
		_tail = std::make_shared<LRUNode<Key, Value>>(Key(), Value());
		_head->_next = _tail;
		_tail->_prev = _head;
	}
	~LRUCache()=default;

	Value get(Key key);
	void put(Key key, Value value);

	void remove(NodePtr node);
	void insert(Key key, Value value);
};


template<typename Key, typename Value>
Value LRUCache<Key, Value>::get(Key key)
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

template<typename Key, typename Value>
void LRUCache<Key, Value>::put(Key key, Value value)
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

template<typename Key, typename Value>
void LRUCache<Key, Value>::remove(NodePtr node)
{
	auto prev = node->_prev.lock(); // 不能直接使用weak_ptr，要用lock()先转换为shared_ptr
	prev->_next = node->_next;
	node->_next->_prev = node->_prev;
	node->_next = nullptr; // 清空next_指针，彻底断开节点与链表的连接
	_map.erase(node->_key);
}

template<typename Key, typename Value>
void LRUCache<Key, Value>::insert(Key key, Value value)
{
	NodePtr node = std::make_shared<LRUNode<Key, Value>>(key, value);

	node->_next = _tail;
	node->_prev = _tail->_prev;
	_tail->_prev.lock()->_next = node;
	_tail->_prev = node;

	_map[key] = node;
}

#endif // LRUCACHE_H
