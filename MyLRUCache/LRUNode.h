#pragma once
#ifndef LRUNODE_H
#define LRUNODE_H

#include <memory>
#include <cmath>

struct LRUNode {
	int _key;
	int _value;
	time_t  _expiredTime;
	std::weak_ptr<LRUNode> _prev;
	std::shared_ptr<LRUNode> _next;

	LRUNode() = delete;
	LRUNode(int key, int value, time_t  expiredTime) : _key(key), _value(value), _expiredTime(expiredTime){}
};


#endif // LRUNODE_H
