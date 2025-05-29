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
private:
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

	int _freqCount;
	NodePtr _head;
	NodePtr _tail;
public:
	FreqList() = delete;
	explicit FreqList(int freqCount) : _freqCount(freqCount) {
		_head = std::make_shared<LFUNode>(-1, -1);
		_tail = std::make_shared<LFUNode>(-1, -1);
		_head->_next = _tail;
		_tail->_prev = _head;
	}

	void insert(NodePtr node) {
		_head->_next->_prev.lock() = node;
		node->_next = _head->_next;
		_head->_next = node;
		node->_prev = _head;
	}
	void remove(NodePtr node) {
		node->_prev.lock()->_next = node->_next;
		node->_next->_prev = node->_prev;
		node->_next = nullptr;
	}

	bool empty() const {
		return _head->_next == _tail;
	}

	std::weak_ptr<LFUNode> getUnfrequentNode() const {
		return _tail->_prev;
	}

	friend class LFUCache<Key, Value>;
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
		_freqListMap[freqCount]->insert(node);
	}

	void remove(Key key) {
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			NodePtr node = it->second;
			int freqCount = node->_freqCount;
			_freqListMap[freqCount]->remove(node);
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

#endif // LFUCACHE_H