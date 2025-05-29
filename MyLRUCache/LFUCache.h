#pragma once
#ifndef LFUCACHE_H
#define LFUCACHE_H

#include <memory>
#include <unordered_map>

template<typename Key, typename Value>
class FreqList;

template<typename Key, typename Value>
class LFUNode {
private:
	Key _key;
	Value _value;
	int _freqCount;
	std::weak_ptr<LFUNode<Key, Value>> _prev;
	std::shared_ptr<LFUNode<Key, Value>> _next;
public:
	LFUNode() = delete;
	LFUNode(Key key, Value value, int freqCount=1) : _key(key), _value(value), _freqCount(freqCount) {}

	inline Value getValue() { return _value; }
	inline void setValue(Value value) { _value = value; }
	inline Key getKey() { return _key; }
	inline int getFreqCount() { return _freqCount; }

	friend class FreqList<Key, Value>;
};

template<typename Key, typename Value>
class FreqList {
private:
	std::shared_ptr<LFUNode<Key, Value>> _head;
	std::shared_ptr<LFUNode<Key, Value>> _tail;
	int _freqCount;
public:
	FreqList() = delete;
	FreqList(int freqCount) : _freqCount(freqCount) {
		_head = std::make_shared<LFUNode<Key, Value>>(-1, -1);
		_tail = std::make_shared<LFUNode<Key, Value>>(-1, -1);
		_head->_next = _tail;
		_tail->_prev = _head;
	}

	void insert(std::shared_ptr<LFUNode<Key, Value>> node) {
		_head->_next->_prev.lock() = node;
		node->_next = _head->_next;
		_head->_next = node;
		node->_prev = _head;
	}
	void remove(std::shared_ptr<LFUNode<Key, Value>> node) {
		node->_prev.lock()->_next = node->_next;
		node->_next->_prev = node->_prev;
		/*node->_prev = nullptr;
		node->_next = nullptr;*/
	}

	bool empty() {
		return _head->_next == _tail;
	}

	std::weak_ptr<LFUNode<Key, Value>> getUnfrequentNode() {
		if (empty()) {
			throw std::runtime_error("FreqList is empty!");
		}
		return _tail->_prev;
	}
};

template<typename Key, typename Value>
class LFUCache {
private:
	using NodePtr = std::shared_ptr<LFUNode<Key, Value>>;
	using FreqListPtr = std::unique_ptr<FreqList<Key, Value>>;

	int _capacity;
	int _minFreqCount;
	std::unordered_map<Key, NodePtr> _nodeMap;
	std::unordered_map<int, FreqListPtr> _freqListMap;
public:
	LFUCache(int capacity) : _capacity(capacity), _minFreqCount(0) {}

	Value get(Key key) {
		auto it = _nodeMap.find(key);
		if (it == _nodeMap.end()) {
			return Value(); // Not found
		}
		NodePtr node = it->second;
		int freqCount = node->getFreqCount();
		// Remove node from current frequency list
		remove(key);
		if (_freqListMap[freqCount]->empty()) {
			++_minFreqCount;
		}
		++freqCount;
		insert(key, node->getValue(), freqCount);
		return node->getValue();
	}

	bool get(Key key, Value& value) {
		value = get(key);
		if (value == Value()) {
			return false;
		}
		return true;
	}

	void put(Key key, Value value) {
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			// found
			NodePtr node = it->second;
			node->setValue(value);
			int freqCount = node->getFreqCount();
			remove(key);
			++freqCount;
			insert(key, value, freqCount);
		}
		else {
			// not found
			if (_nodeMap.size() >= _capacity) {
				// cache is full, remove the unfrequently node
				auto node = _freqListMap[_minFreqCount]->getUnfrequentNode().lock();
				remove(node->getKey());
			}
			// insert new node
			insert(key, value, 1);
			_minFreqCount = 1;
		}
	}

	void insert(Key key, Value value, int freqCount) {
		NodePtr node = std::make_shared<LFUNode<Key, Value>>(key, value, freqCount);
		_nodeMap[key] = node;
		if (_freqListMap.find(freqCount) == _freqListMap.end()) {
			_freqListMap[freqCount] = std::make_unique<FreqList<Key, Value>>(freqCount);
		}
		_freqListMap[freqCount]->insert(node);
		if (freqCount > _maxFreqCount)
			_maxFreqCount = freqCount;
	}
	void remove(Key key) {
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			NodePtr node = it->second;
			int freqCount = node->getFreqCount();
			_freqListMap[freqCount]->remove(node);
			_nodeMap.erase(it);
		}
	}
};

#endif // LFUCACHE_H