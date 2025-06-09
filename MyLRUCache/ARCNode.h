#pragma once
#ifndef ARCNODE_H
#define ARCNODE_H

#include <memory>

template<typename Key, typename Value>
struct ARCNode {
	Key _key;
	Value _value;
	int _freq;
	std::weak_ptr<ARCNode<Key, Value>> _prev;
	std::shared_ptr<ARCNode<Key, Value>> _next;

	ARCNode(Key key, Value value, int freq = 1) 
		: _key(key), _value(value), _freq(freq), _next(nullptr) {}
};


#endif // ARCNODE_H