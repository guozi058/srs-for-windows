#ifndef _MAP_H
#define _MAP_H

typedef struct Object{
	void* raw_data;
	unsigned int size;
}Object;

enum COLOR {RED,BLACK};

typedef struct Rb_Node {
	struct Rb_Node *left;
	struct Rb_Node *right;
	struct Rb_Node *parent;
	int color; 
	Object* key;
	Object* value; 
}Rb_Node;

typedef struct Rb {
	Rb_Node* root;
	Rb_Node sentinel;
}Rb;

typedef struct Map {
	Rb* root;
}Map;

#ifdef __cplusplus
extern "C"
{
#endif

Map* Map_New();
int Map_Insert(Map* pMap, void* key, unsigned int key_size, void* value,  unsigned int value_size);
int Map_Exists(Map* pMap, void* key);
int Map_Remove(Map* pMap, void* key);
int Map_Find(Map* pMap, void* key, void**value);
int Map_Delete(Map* pMap);
void Map_Foreach(Map* pMap, void (*Handler)(Rb_Node*));

#endif
