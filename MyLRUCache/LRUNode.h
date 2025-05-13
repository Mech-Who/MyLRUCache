#pragma once
#ifndef LRUNODE_H
#define LRUNODE_H

//#include <memory>

struct LRUNode {
	int _key;
	int _value;
	LRUNode* _prev = nullptr;
	LRUNode* _next = nullptr;

	LRUNode() = delete;
	LRUNode(int key, int value) : _key(key), _value(value) {}
};


#endif // LRUNODE_H
