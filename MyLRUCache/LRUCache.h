#pragma once
#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <memory>
#include <unordered_map>
#include <mutex>

// 前向声明
template<typename Key, typename Value>
class LRUCache;

/****************************************
LRUNode

记录缓存数据的节点类
****************************************/
template<typename Key, typename Value>
struct LRUNode {
protected:
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
protected:
	using NodePtr = std::shared_ptr<LRUNode<Key, Value>>;
	using NodeMap = std::unordered_map<int, NodePtr>;
	// mutex 互斥量 TODO
	std::mutex _mutex;
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
	bool get(Key key, Value& value);
	void put(Key key, Value value);

	void remove(NodePtr node);
	void remove(Key key);
	void insert(Key key, Value value);
};


template<typename Key, typename Value>
Value LRUCache<Key, Value>::get(Key key)
{
	std::lock_guard<std::mutex> lock(_mutex);
	if (_map.find(key) != _map.end()) {
		NodePtr node = _map[key];
		// 下面完成的是删除双向链表及 hash 中原有的点，并将该节点加入最近使用的表尾 R 的前驱操作
		remove(node);
		insert(node->_key, node->_value);
		return node->_value;
	}
	else return Value{};
}

template<typename Key, typename Value>
bool LRUCache<Key, Value>::get(Key key, Value& value) {
	value = get(key);
	if (value == -1) {
		return false;
	}
	return true;
}

template<typename Key, typename Value>
void LRUCache<Key, Value>::put(Key key, Value value)
{
	std::lock_guard<std::mutex> lock(_mutex);
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
void LRUCache<Key, Value>::remove(Key key)
{
	auto node = _map[key];
	remove(node);
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

/****************************************
LRUKCache

基于LRU-K算法的缓存算法
****************************************/

template<typename Key, typename Value>
class LRUKCache : public LRUCache<Key, Value>
{
private:
	using NodePtr = std::shared_ptr<LRUNode<Key, Value>>;

	const int _k;
	std::unique_ptr<LRUCache<Key, size_t>> _historyTimesMap;
	std::unordered_map<Key, Value> _historyValueMap;
public:
	LRUKCache(int capacity, int historyCapacity, int k) :LRUCache<Key, Value>(capacity),
		_historyTimesMap(std::make_unique<LRUCache<Key, size_t>>(historyCapacity)),
		_k(k) {
	}
	Value get(Key key);
	void put(Key key, Value value);
};

template<typename Key, typename Value>
Value LRUKCache<Key, Value>::get(Key key) 
{
	// 先在主缓存中查找
	Value value{};
	bool inMain = LRUCache<Key, Value>::get(key, value);
	// 更新访问计数
	size_t accessCount = _historyTimesMap->get(key);
	++accessCount;
	_historyTimesMap->put(key, accessCount);
	// 如果找到了，直接返回
	if (inMain) {
		return value;
	}
	// 如果没有找到，检查访问次数是否达到k次
	if (accessCount >= _k) {
		// 如果达到k次，则检查是否记录历史值
		auto it = _historyValueMap.find(key);
		if (it != _historyValueMap.end())
		{
			// 有历史值，则加入主缓存并返回
			Value storedValue = it->second;
			// 需要先从历史记录中删除
			_historyTimesMap->remove(key);
			_historyValueMap.erase(key);
			// 加入主缓存并返回
			LRUCache<Key, Value>::put(key, storedValue);
			return storedValue;
		}
	}
	// 没有历史值或者访问次数没有达到k次，则返回默认值;
	return value;
}

template<typename Key, typename Value>
void LRUKCache<Key, Value>::put(Key key, Value value)
{
	// 如果在主缓存，直接更新
	Value existingValue{};
	bool inMain = LRUCache<Key, Value>::get(key, existingValue);
	if (inMain) {
		LRUCache<Key, Value>::put(key, value);
		return;
	}
	// 如果不在，更新访问次数
	size_t accessCount = _historyTimesMap->get(key);
	++accessCount;
	_historyTimesMap->put(key, accessCount);

	// 保存值到历史记录
	_historyValueMap[key] = value;

	// 如果访问次数达到k次，则加入主缓存并清楚历史记录
	if (accessCount >= _k) {
		_historyTimesMap->remove(key);
		_historyValueMap.erase(key);
		LRUCache<Key, Value>::put(key, value);
	}
}


#endif // LRUCACHE_H
