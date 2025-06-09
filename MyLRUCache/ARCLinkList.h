#pragma once
#ifndef ARCLINKLIST_H
#define ARCLINKLIST_H

#include "ARCNode.h"
#include <memory>

template<typename Key, typename Value>
class HashLink {
	using Node = ARCNode<Key, Value>;
	using NodePtr = std::shared_ptr<Node>;
	using WeakNodePtr = std::weak_ptr<Node>;
	std::shared_ptr<Node> _head;
	std::shared_ptr<Node> _tail;
	int _freq;
public:
	HashLink() : HashLink(0) {}
	HashLink(int freq) : _freq(freq) {
		_head = std::make_shared<Node>(Key(), Value(), -1);
		_tail = std::make_shared<Node>(Key(), Value(), -1);
		_head->_next = _tail;
		_tail->_prev = _head;
	}
	void headInsert(NodePtr node) {
		node->_next = _head->_next;
		node->_prev = _head;
		_head->_next->_prev = node;
		_head->_next = node;
	}
	void tailInsert(NodePtr node) {
		node->_prev = _tail->_prev;
		node->_next = _tail;
		_tail->_prev.lock()->_next = node;
		_tail->_prev = node;
	}
	void nodeRemove(NodePtr node) {
		node->_next->_prev = node->_prev;
		node->_prev.lock()->_next = node->_next;
		node->_next.reset();
		node->_prev.reset();
	}
	WeakNodePtr headRemove() {
		auto node_ptr = _head->_next;
		node_ptr->_next->_prev = _head;
		_head->_next = node_ptr->_next;
		node_ptr->_prev.reset();
		node_ptr->_next.reset();
		return node_ptr;
	}
	WeakNodePtr tailRemove() {
		auto node_ptr = _tail->_prev.lock();
		node_ptr->_prev.lock()->_next = _tail;
		_tail->_prev = node_ptr->_prev;
		node_ptr->_prev.reset();
		node_ptr->_next.reset();
		return node_ptr;
	}
	bool isEmpty() {
		return _head->_next == _tail;
	}
};

#endif // ARCLINKLIST_H
