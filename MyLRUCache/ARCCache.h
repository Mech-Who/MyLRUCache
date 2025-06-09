#pragma once
#ifndef ARCCACHE_H
#define ARCCACHE_H

#include "ARCNode.h"
#include "ARCLinkList.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <map>

template<typename Key, typename Value>
class ARC_LRUCache {
	using Node = ARCNode<Key, Value>;
	using NodePtr = std::shared_ptr<Node>;
	using NodeMap = std::unordered_map<Key, NodePtr>;
	using NodeList = HashLink<Key, Value>;

	std::mutex _mtx;
	int _transformThreshold;
	// main cache
	int _capacity;
	NodeMap _nodeMap;
	NodeList _nodeList;
	// ghost list
	int _ghostCapacity;
	NodeMap _ghostMap;
	NodeList _ghostList;

	bool updateNodeAccess(NodePtr node) {
		++node->_freq;
		return node->_freq >= _transformThreshold;
	}
public:
	ARC_LRUCache(int capacity, int ghostCapacity, int transformThreshold)
		: _capacity(capacity), _ghostCapacity(ghostCapacity), _transformThreshold(transformThreshold){}
	/**
	* 将节点从所在的链表删除
	*/
	void removeFromList(NodePtr node) {
		node->_next->_prev = node->_prev;
		node->_prev.lock()->_next = node->_next;
		node->_prev.reset();
		node->_next.reset();
	}

	/**
	* 检查key对应的节点是否存在，存在则移除节点并返回true；否则返回false。
	*/
	bool checkGhost(Key key) {
		auto it = _ghostMap.find(key);
		if (it == _ghostMap.end()) {
			return false;
		}
		// Node exists in ghost list, remove it
		removeFromList(it->second);
		_ghostMap.erase(it);
		return true;
	}

	/**
	* 删除最少使用的元素
	*/
	void kickOut() {
		// 从主缓存 链表 删除
		auto removedNode = _nodeList.tailRemove().lock();
		if (!removedNode || _nodeList.isEmpty()) return;
		
		if (_ghostMap.size() >= _ghostCapacity) {
			auto removedGhost = _ghostList.tailRemove().lock();
			if (removedGhost)
				_ghostMap.erase(removedGhost->_key);
		}
		// 重置频数
		removedNode->_freq = 1;
		// 插入到 ghost 链表
		_ghostList.headInsert(removedNode);
		_ghostMap[removedNode->_key] = removedNode;
		// 从主缓存 Map 删除
		_nodeMap.erase(removedNode->_key);
	}

	/**
	* 主缓存扩容
	*/
	void expandCapacity() {
		++_capacity;
	}
	/**
	* 主缓存缩容
	*/
	bool shrinkCapacity() {
		if (_capacity <= 0) return false;
		// Cache 已满，删除最近最久未使用节点
		if (_nodeMap.size() >= _capacity) {
			kickOut();
		}
		--_capacity;
		return true;
	}

	/**
	* 从LRU读取缓存
	*/
	bool get(Key key, Value& value, bool& shouldTransform) {
		// 加锁
		std::lock_guard<std::mutex> lock(_mtx);
		// 查找 Node
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			// Node 存在，更新频数（_freq），移至链表头部
			NodePtr node = it->second;
			removeFromList(node);
			shouldTransform = updateNodeAccess(node);
			// 如果达到阈值，应该加入到LFU，用 shouldTransform 进行标识，然后从LRU删除，交给 ARCCache 处理
			if (shouldTransform) {
				_nodeMap.erase(key);
				return true;
			}
			_nodeList.headInsert(node);
			value = node->_value;
			return true;
		}
		// Node 不存在
		return false;
	}
	
	/**
	* 向LRU写入缓存
	*/
	bool put(Key key, Value value, bool& shouldTransform) {
		if (_capacity <= 0) return false;
		// 加锁
		std::lock_guard<std::mutex> lock(_mtx);
		// 查找 Node
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			// Node 存在，更新频数，添加到节点头部
			NodePtr node = _nodeMap[key];
			node->_value = value;
			removeFromList(node);
			shouldTransform = updateNodeAccess(node);
			// 如果达到阈值，应该加入到LFU，用 shouldTransform 进行标识，然后从LRU删除，交给 ARCCache 处理
			if (shouldTransform) {
				_nodeMap.erase(key);
				return true;
			}
			_nodeList.headInsert(node);
			return true;
		}
		// Cache 已满，删除最近最久未使用节点
		if (_nodeMap.size() >= _capacity) {
			kickOut();
		}
		// Node 不存在，创建新 Node 并插入链表头部，在 Map 中添加记录
		NodePtr newNode = std::make_shared<Node>(key, value);
		_nodeMap[key] = newNode;
		_nodeList.headInsert(newNode);
		return true;
	}
};

template<typename Key, typename Value>
class ARC_LFUCache {
	using Node = ARCNode<Key, Value>;
	using NodePtr = std::shared_ptr<Node>;
	using NodeMap = std::unordered_map<Key, NodePtr>;
	using List = HashLink<Key, Value>;
	using FreqPtr = std::unique_ptr<List>;
	using FreqMap = std::map<Key, FreqPtr>;

	std::mutex _mtx;
	int _transformThreshold;
	// main cache
	int _capacity;
	int _minFreqCount;
	NodeMap _nodeMap;
	FreqMap _freqListMap;
	// ghost list
	int _ghostCapacity;
	NodeMap _ghostMap;
	List _ghostList;

	void insertToFreqList(NodePtr node) {
		if (_freqListMap.find(node->_freq) == _freqListMap.end()) {
			_freqListMap[node->_freq] = std::make_unique<List>(node->_freq);
		}
		_freqListMap[node->_freq]->headInsert(node);
	}

	void removeFromFreqList(NodePtr node) {
		// Check if the frequency list exists
		if (_freqListMap.find(node->_freq) == _freqListMap.end()) {
			return;
		}
		// Remove node
		_freqListMap[node->_freq]->nodeRemove(node);
		// Update _minFreqCount if the list is empty after remove the node
		if (_freqListMap[_minFreqCount]->isEmpty()) {
			++_minFreqCount;
		}
	}

	void kickOut() {
		// 判空
		if (_freqListMap.empty()) return;

		if (_freqListMap[_minFreqCount]->isEmpty()) return;

		// 从主缓存 链表 删除最少使用节点
		auto removedNode = _freqListMap[_minFreqCount]->tailRemove().lock();
		if (_freqListMap[_minFreqCount]->isEmpty()) {
			++_minFreqCount;
		}

		// 节点添加到 ghost
		if (_ghostMap.size() >= _ghostCapacity) {
			// ghost 满了先删除
			auto removedGhost = _ghostList.tailRemove().lock();
			if(removedGhost){
				_ghostMap.erase(removedGhost->_key);
			}
		}
		// 更新频数
		removedNode->_freq = 1;
		// 添加到 ghost
		_ghostList.headInsert(removedNode);
		_ghostMap[removedNode->_key] = removedNode;
		// 从主缓存 Map 删除
		_nodeMap.erase(removedNode->_key);
	}

	void removeFromList(NodePtr node) {
		node->_next->_prev = node->_prev;
		node->_prev.lock()->_next = node->_next;
		node->_next.reset();
		node->_prev.reset();
	}
public:
	ARC_LFUCache(int capacity, int ghostCapacity, int transformThreshold)
		: _capacity(capacity)
		, _ghostCapacity(ghostCapacity)
		, _transformThreshold(transformThreshold)
		, _minFreqCount(0) {}

	/**
	* 检查 key 是否在 ghost 中，在的话删除并返回true，否则返回false
	*/
	bool checkGhost(Key key) {
		auto it = _ghostMap.find(key);
		if (it == _ghostMap.end()) {
			return false;
		}
		NodePtr node = it->second;
		removeFromList(node);
		_ghostMap.erase(it);
		return true;
	}

	/**
	* Cache 扩容
	*/
	void expandCapacity() {
		++_capacity;
	}
	/**
	* Cache 缩容
	*/
	bool shrinkCapacity() {
		if (_capacity <= 0) return false;
		if (_nodeMap.size() >= _capacity) {
			kickOut();
		}
		// Cache 已满，删除最近最少被使用节点
		--_capacity;
		return true;
	}
	
	/**
	* 读取缓存
	*/
	bool get(Key key, Value& value) {
		std::lock_guard<std::mutex> lock(_mtx);
		auto it = _nodeMap.find(key);
		if (it == _nodeMap.end()) {
			return false;
		}
		NodePtr node = it->second;
		value = node->_value;
		// Remove from current frequency list
		removeFromFreqList(node);
		// Increment frequency
		++node->_freq;
		// Insert into new frequency list
		insertToFreqList(node);
		return true;
	}
	
	/**
	* 写入缓存
	*/
	bool put(Key key, Value value) {
		if (_capacity <= 0) return false;

		std::lock_guard<std::mutex> lock(_mtx);
		auto it = _nodeMap.find(key);
		if (it != _nodeMap.end()) {
			// Node 存在更新值
			auto node = it->second;
			node->_value = value;
			removeFromFreqList(node);
			++node->_freq;
			insertToFreqList(node);
			return true;
		}
		// Node 存在
		if (_nodeMap.size() >= _capacity) {
			// Cache 已满，删除最近最少被使用节点
			kickOut();
		}
		// 创建新节点并插入缓存
		NodePtr newNode = std::make_shared<ARCNode<Key, Value>>(key, value);
		newNode->_freq = 1;
		_nodeMap[key] = newNode;
		insertToFreqList(newNode);
		_minFreqCount = 1; // Reset min frequency count
		return true;
	}
};

template<typename Key, typename Value>
class ARCCache {
	int _capacity;
	int _transformThreshold;
	std::unique_ptr<ARC_LRUCache<Key, Value>> _LRU;
	std::unique_ptr<ARC_LFUCache<Key, Value>> _LFU;
	
	/**
	* 检查所查值是否在 Ghost 中，在的话扩容对应缓存部分
	*/
	bool checkGhostCaches(Key key) {
		bool inGhost = false;
		// 检查是否在 Ghost
		if (_LRU->checkGhost(key)) {
			// 先缩容再扩容
			if (_LFU->shrinkCapacity()) {
				_LRU->expandCapacity();
			}
			inGhost = true;
		}
		else if (_LFU->checkGhost(key)) {
			if (_LRU->shrinkCapacity()) {
				_LFU->expandCapacity();
			}
			inGhost = true;
		}
		return inGhost;
	}

public:
	ARCCache(int capacity, int transformThreshold) 
		: _capacity(capacity), 
		_transformThreshold(transformThreshold),
		_LRU(std::make_unique<ARC_LRUCache<Key, Value>>(static_cast<int>(capacity / 2), static_cast<int>(capacity / 2), transformThreshold)),
		_LFU(std::make_unique<ARC_LFUCache<Key, Value>>(_capacity - static_cast<int>(capacity/2), _capacity - static_cast<int>(capacity / 2), transformThreshold))
	{}

	~ARCCache() = default;

	bool get(Key key, Value& value) {
		checkGhostCaches(key);

		bool shouldTransform = false;
		if (_LRU->get(key, value, shouldTransform)) {
			if (shouldTransform) {
				_LFU->put(key, value);
			}
			// Found in LRU cache
			return true;
		}
		return _LFU->get(key, value);
	}
	Value get(Key key) {
		Value v{};
		get(key, v);
		return v;
	}

	void put(Key key, Value value) {
		bool shouldTransform = false;
		if (_LRU->checkGhost(key)) {
			if (_LRU->put(key, value, shouldTransform)) {
				if (shouldTransform) {
					_LFU->put(key, value);
				}
			}
		}else if(_LFU->checkGhost(key)){
			_LFU->put(key, value);
		}
		else {
			_LRU->put(key, value);
		}
	}
};

#endif // ARCCACHE_H
