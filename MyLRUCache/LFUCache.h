#pragma once
#ifndef LFUCACHE_H
#define LFUCACHE_H

#include <mutex>
#include <memory>
#include <unordered_map>

template<typename Key, typename Value>
class LFUCache;

template<typename Key, typename Value>
class FreqList {
public:
	struct LFUNode {
		Key _key;
		Value _value;
		int _freqCount;
		std::weak_ptr<LFUNode> _prev;
		std::shared_ptr<LFUNode> _next;

		LFUNode() : _freqCount(1), _next(nullptr) {}
		LFUNode(Key key, Value value, int freqCount = 1) : _key(key), _value(value), _freqCount(freqCount) {}
	};
	using NodePtr = std::shared_ptr<LFUNode>;

	FreqList() = delete;
	explicit FreqList(int freqCount) : _freqCount(freqCount) {
		_head = std::make_shared<LFUNode>(-1, -1);
		_tail = std::make_shared<LFUNode>(-1, -1);
		_head->_next = _tail;
		_tail->_prev = _head;
	}

	bool empty() const {
		return _head->_next == _tail;
	}

	std::weak_ptr<LFUNode> getUnfrequentNode() const {
		return _tail->_prev;
	}

	friend class LFUCache<Key, Value>;

public:
	int _freqCount;
	NodePtr _head;
	NodePtr _tail;
};

template<typename Key, typename Value>
class LFUCache {
private:
	using LFUNode = typename FreqList<Key, Value>::LFUNode;
	using NodePtr = std::shared_ptr<LFUNode>;
	using FreqListPtr = std::unique_ptr<FreqList<Key, Value>>;

	int _capacity;
	int _minFreqCount;
	std::mutex _mutex;
	std::unordered_map<Key, NodePtr> _nodeMap;
	std::unordered_map<int, FreqListPtr> _freqListMap;

	void insert(Key key, Value value, int freqCount) {
		NodePtr node = std::make_shared<LFUNode>(key, value, freqCount);
		_nodeMap[key] = node;
		if (_freqListMap.find(freqCount) == _freqListMap.end()) {
			_freqListMap[freqCount] = std::make_unique<FreqList<Key, Value>>(freqCount);
		}
		auto head = _freqListMap[freqCount]->_head;
		head->_next->_prev.lock() = node;
		node->_next = head->_next;
		head->_next = node;
		node->_prev = head;
	}

	void remove(Key key) {
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			NodePtr node = it->second;
			int freqCount = node->_freqCount;
			node->_prev.lock()->_next = node->_next;
			node->_next->_prev = node->_prev;
			node->_next = nullptr;
			_nodeMap.erase(it);
		}
	}
public:
	LFUCache(int capacity) : _capacity(capacity), _minFreqCount(0) {}

	Value get(Key key) {
		std::lock_guard<std::mutex> lock(_mutex);
		auto it = _nodeMap.find(key);
		if (it == _nodeMap.end()) {
			return Value(); // Not found
		}
		NodePtr node = it->second;
		int freqCount = node->_freqCount;
		// Remove node from current frequency list
		remove(key);
		if (_freqListMap[freqCount]->empty()) {
			++_minFreqCount;
		}
		++freqCount;
		insert(key, node->_value, freqCount);
		return node->_value;
	}

	bool get(Key key, Value& value) {
		value = get(key);
		if (value == Value()) {
			return false;
		}
		return true;
	}

	void put(Key key, Value value) {
		std::lock_guard<std::mutex> lock(_mutex);
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			// found
			NodePtr node = it->second;
			node->_value = value;
			int freqCount = node->_freqCount;
			remove(key);
			++freqCount;
			insert(key, value, freqCount);
		}
		else {
			// not found
			if (_nodeMap.size() >= _capacity) {
				// cache is full, remove the unfrequently node
				auto node = _freqListMap[_minFreqCount]->getUnfrequentNode().lock();
				remove(node->_key);
			}
			// insert new node
			insert(key, value, 1);
			_minFreqCount = 1;
		}
	}
};


template<typename Key, typename Value>
class AlignLFUCache {
private:
	using Node = FreqList<Key, Value>::LFUNode;
	using NodePtr = std::shared_ptr<Node>;
	using NodeMap = std::unordered_map<Key, NodePtr>;
	using FreqListPtr = std::unique_ptr<FreqList<Key, Value>>;
	using FreqListMap = std::unordered_map<int, FreqListPtr>;

	std::mutex _mutex;
	NodeMap _nodeMap;
	FreqListMap _freqListMap;
	int _capacity;
	int _minFreqCount;
	// align
	int _maxAverageFreq;
	int _curAverageFreq;
	int _curTotalFreq;

	void insert(NodePtr node);
	void remove(NodePtr node);

	void insertToFreqList(NodePtr node);
	void removeFromFreqList(NodePtr node);

	void kickOut();

	void addFreqCount();
	void decFreqCount(int num=1);
	void updateAllNodeFreq();
	void updateMinFreq();
public:
	AlignLFUCache(int capacity, int maxAverageFreq) : _capacity(capacity), _maxAverageFreq(maxAverageFreq), _curAverageFreq(0), _curTotalFreq(0), _minFreqCount(0) {}

	Value get(Key key);
	bool get(Key key, Value& value);
	void put(Key key, Value value);
};

template<typename Key, typename Value>
Value AlignLFUCache<Key, Value>::get(Key key) {
	// 0. 加锁，线程安全
	std::lock_guard<std::mutex> lock(_mutex);
	// 1. 判断key是否存在
	auto it = _nodeMap.find(key);
	// 2. 如果不存在，返回默认值
	if (it == _nodeMap.end()) {
		return Value(); // Not found
	}
	// 3. 如果存在，获取节点
	NodePtr node = it->second;
	// 4. 更新节点的频率
	remove(node);
	++ node->_freqCount;
	insert(node);
	// 5. 更新平均频率
	addFreqCount();
	// 6. 返回节点的值
	return node->_value;
}

template<typename Key, typename Value>
bool AlignLFUCache<Key, Value>::get(Key key, Value& value)
{
	Value v = get(key);
	if (v == Value()) {
		return false; // Not found
	}
	value = v;
	return true;
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::put(Key key, Value value)
{
	if (_capacity == 0) return;
	// 0. 加锁，线程安全
	std::lock_guard<std::mutex> lock(_mutex);
	// 1. 判断key是否存在
	auto it = _nodeMap.find(key);
	// 2. 如果存在，更新节点的值
	if (it != _nodeMap.end()) {
		// 3. 获得当前节点
		auto node = _nodeMap[key];
		// 4. 更新节点的值与频率
		node->_value = value;
		++ node->_freqCount;
		removeFromFreqList(node);
		insertToFreqList(node);
		// 5. 更新平均频率
		addFreqCount();
	}
	// 6. 如果不存在，判断缓存是否已满
	else {
		// 7. 删除最不常用节点
		if (_nodeMap.size() >= _capacity) {
			kickOut();
		}
		// 8. 创建新节点
		NodePtr node = std::make_shared<Node>(key, value);
		insert(node);
		// 9. 更新平均频率
		addFreqCount();
	}
}


template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::insert(NodePtr node)
{
	// 0. 判断节点是否为空
	if (!node) {
		return;
	}
	// 1. 插入节点到哈希表
	_nodeMap[node->_key] = node;
	// 2. 判断频率列表是否存在
	if (_freqListMap.find(node->_freqCount) == _freqListMap.end()) {
		_freqListMap[node->_freqCount] = std::make_unique<FreqList<Key, Value>>(node->_freqCount);
	}
	// 3. 插入节点到频率列表
	auto& freqList = _freqListMap[node->_freqCount];
	node->_next = freqList->_head->_next;
	node->_prev = freqList->_head;
	freqList->_head->_next = node;
	node->_next->_prev = node;
	// 4. 更新最小频率
	if (_minFreqCount == 0 || node->_freqCount < _minFreqCount) {
		_minFreqCount = node->_freqCount;
	}
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::remove(NodePtr node) 
{
	// 0. 判断节点是否为空
	if (!node) {
		return;
	}
	// 1. 从哈希表中删除节点
	_nodeMap.erase(node->_key);
	// 2. 从频率列表中删除节点
	if (_freqListMap.find(node->_freqCount) != _freqListMap.end()) {
		//auto& freqList = _freqListMap[node->_freqCount];
		node->_prev.lock()->_next = node->_next;
		node->_next->_prev = node->_prev;
		node->_next = nullptr;
	}
	// 3. 更新最小频率
	if (_freqListMap[_minFreqCount]->empty()) {
		++_minFreqCount;
	}
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::insertToFreqList(NodePtr node) {
	if (!node)
		return;
	// 添加进入相应的频次链表前需要判断该频次链表是否存在
	auto freq = node->_freqCount;
	if (_freqListMap.find(freq) == _freqListMap.end())
	{
		// 不存在则创建
		_freqListMap[freq] = std::make_unique<FreqList<Key, Value>>(freq);
	}

	auto head = _freqListMap[freq]->_head;
	auto tail = _freqListMap[freq]->_tail;

	node->_next = head->_next;
	node->_prev = head;
	head->_next->_prev = node;
	head->_next = node;
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::removeFromFreqList(NodePtr node) {
	// 检查结点是否为空
	if (!node || node->_prev.expired() || !node->_next)
		return;

	auto pre = node->_prev.lock(); // 使用lock()获取shared_ptr
	pre->_next = node->_next;
	node->_next->_prev = pre;
	node->_next = nullptr; // 确保显式置空next指针，彻底断开节点与链表的连接
}


template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::kickOut() {
	auto node = _freqListMap[_minFreqCount]->getUnfrequentNode().lock();
	node->_prev.lock()->_next = node->_next;
	node->_next->_prev = node->_prev;
	node->_next = nullptr;
	_nodeMap.erase(node->_key);
	decFreqCount(node->_freqCount);
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::addFreqCount() {
	// 1. 更新当前总频率
	++ _curTotalFreq;
	// 2. 更新当前平均频率
	if (_curTotalFreq > 0) {
		_curAverageFreq = static_cast<int>(_curTotalFreq / _nodeMap.size());
	}
	// 3. 如果当前平均频率超过最大平均频率，更新所有节点的频率
	if (_curAverageFreq > _maxAverageFreq) {
		updateAllNodeFreq();
	}
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::decFreqCount(int num) {
	// 1. 更新当前总频率
	if (_curTotalFreq > 0) {
		_curTotalFreq -= num;
	}
	// 2. 更新当前平均频率
	if (_nodeMap.size() > 0) {
		_curAverageFreq = static_cast<int>(_curTotalFreq / _nodeMap.size());
	}
	else {
		_curAverageFreq = 0;
	}
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::updateAllNodeFreq() 
{
	if (_nodeMap.empty()) return;

	// 1. 遍历所有节点，更新频率
	for (auto& pair : _nodeMap) {
		NodePtr node = pair.second;
		if (!node) continue;
		// 2. 更新节点的频率
		removeFromFreqList(node);
		node->_freqCount -= _maxAverageFreq / 2; // 简单的平均化处理
		if (node->_freqCount < 1) {
			node->_freqCount = 1;
		}
		insertToFreqList(node);
	}
	// 3. 更新最小频率
	updateMinFreq();
}

template<typename Key, typename Value>
void AlignLFUCache<Key, Value>::updateMinFreq() 
{
	_minFreqCount = INT8_MAX;
	for (const auto& pair : _freqListMap) {
		if (pair.second && !pair.second->empty()) {
			_minFreqCount = std::min(_minFreqCount, pair.first);
		}
	}
	if (_minFreqCount == INT8_MAX) {
		_minFreqCount = 1;
	}
}

#endif // LFUCACHE_H