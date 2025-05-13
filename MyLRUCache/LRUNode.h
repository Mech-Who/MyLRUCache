#pragma once
#ifndef LRUNODE_H
#define LRUNODE_H

//#include <memory>
#include <cmath>

struct LRUNode {
	int _key;
	int _value;
	time_t  _expiredTime;
	LRUNode* _prev = nullptr;
	LRUNode* _next = nullptr;

	LRUNode() = delete;
	LRUNode(int key, int value, time_t  expiredTime) : _key(key), _value(value), _expiredTime(expiredTime) {}
};


#endif // LRUNODE_H
