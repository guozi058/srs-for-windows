#include "Map.h"
#include "Util.h"


int Compare_Key(void* left, void* right)
{
	return xStrCmp((char *)left, (char *)right);
}


Object* Object_New(void* inObject, size_t obj_size)
{

	Object* tmp =(Object*)xMalloc(sizeof(Object));   
	if(!tmp)
		return(Object*)0;
	tmp->size = obj_size;
	tmp->raw_data =(void*)xMalloc(obj_size);
	if(!tmp->raw_data) {
		xFree(tmp);
		return(Object*)0;
	}
	memcpy(tmp->raw_data, inObject, obj_size);
	return tmp;
}

int Get_Object(Object *inObject, void**elem)
{
	*elem =(void*)xMalloc(inObject->size);
	if(! *elem)
		return -1;
	memcpy(*elem, inObject->raw_data, inObject->size);
	return 0;
}

void Delete_Object (Object* inObject) {
	if (inObject){
		xFree(inObject->raw_data);
		xFree(inObject);
	}
}

void Rb_Node_Delete(Rb* pTree, Rb_Node* x) {

	Delete_Object(x->key);

	if (x->value){
		Delete_Object(x->value);
	}
}


Rb* Rb_New()
{

	Rb* pTree =(Rb*)xMalloc(sizeof(Rb));
	if(pTree ==(Rb*)0)
		return(Rb*)0;

	pTree->root = &pTree->sentinel;
	pTree->sentinel.left = &pTree->sentinel;
	pTree->sentinel.right = &pTree->sentinel;
	pTree->sentinel.parent =(Rb_Node*)0 ;
	pTree->sentinel.color = BLACK;

	return pTree;
}

int Rb_Delete(Rb* pTree)
{
	Rb_Node* z = pTree->root;

	while (z != &pTree->sentinel) {
		if (z->left != &pTree->sentinel)
			z = z->left;
		else if (z->right != &pTree->sentinel)
			z = z->right;
		else {
			Rb_Node_Delete(pTree, z);
			if (z->parent) {
				z = z->parent;
				if (z->left != &pTree->sentinel){
					xFree(z->left);
					z->left = &pTree->sentinel;
				}else if (z->right != &pTree->sentinel){
					xFree(z->right);
					z->right = &pTree->sentinel;
				}
			} else {
				xFree(z);
				z = &pTree->sentinel;
			}
		}
	}
	xFree(pTree);
	return 0;
}


void Rb_Left_Rotate(Rb* pTree, Rb_Node* x)
{
	Rb_Node* y;
	y = x->right;
	x->right = y->left;
	if (y->left != &pTree->sentinel)
		y->left->parent = x;
	if (y != &pTree->sentinel)
		y->parent = x->parent;
	if (x->parent){
		if (x == x->parent->left)
			x->parent->left = y;
		else
			x->parent->right = y;
	}
	else
		pTree->root = y;
	y->left = x;
	if (x != &pTree->sentinel)
		x->parent = y;
}


void Rb_Right_Rotate(Rb* pTree, Rb_Node* x)
{
	Rb_Node* y = x->left;
	x->left = y->right;
	if(y->right != &pTree->sentinel)
		y->right->parent = x;
	if(y != &pTree->sentinel)
		y->parent = x->parent;
	if(x->parent) {
		if(x == x->parent->right)
			x->parent->right = y;
		else
			x->parent->left = y;
	}
	else
		pTree->root = y;
	y->right = x;
	if(x != &pTree->sentinel)
		x->parent = y;
}


void Rb_Insert_Fixup(Rb* pTree, Rb_Node* x) {
	while(x != pTree->root && x->parent->color == RED) {
		if(x->parent == x->parent->parent->left) {
			Rb_Node* y = x->parent->parent->right;
			if(y->color == RED) {
				x->parent->color         = BLACK;
				y->color                 = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {
				if(x == x->parent->right){
					x = x->parent;
					Rb_Left_Rotate(pTree, x);
				}
				x->parent->color         = BLACK;
				x->parent->parent->color = RED;
				Rb_Right_Rotate(pTree, x->parent->parent);
			}
		} else {
			Rb_Node* y = x->parent->parent->left;
			if(y->color == RED) {
				x->parent->color         = BLACK;
				y->color                 = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {
				if(x == x->parent->left) {
					x = x->parent;
					Rb_Right_Rotate(pTree, x);
				}
				x->parent->color         = BLACK;
				x->parent->parent->color = RED;
				Rb_Left_Rotate(pTree, x->parent->parent);
			}
		}
	}
	pTree->root->color = BLACK;
}

Rb_Node* Rb_Find(Rb* pTree, void* key) 
{
	int c = 0;
	void* cur_key ;
	Rb_Node* x = pTree->root;

	while (x != &pTree->sentinel) {
		Get_Object(x->key, &cur_key);
		c = Compare_Key(key, cur_key);
		xFree(cur_key);
		if (c == 0) {
			break;
		} else {
			x = c < 0 ? x->left : x->right;
		}
	}
	if (x == &pTree->sentinel)
		return (Rb_Node*)0 ;
	return x;
}


int Rb_Insert(Rb* pTree, void* k, size_t key_size, void* v, size_t value_size)
{

	Rb_Node* x;
	Rb_Node* y;
	Rb_Node* z;
	//////////////////////////////////////////////////////////////////////////
	int c = 0;
	void* cur_key;
	void* new_key;

	x =(Rb_Node*)xMalloc(sizeof(Rb_Node));
	if(x ==(Rb_Node*)0) 
		return -1;

	x->left = &pTree->sentinel;
	x->right = &pTree->sentinel;
	x->color = RED;

	x->key = Object_New(k, key_size);
	if(v) {
		x->value = Object_New(v, value_size);
	} else {
		x->value =(Object*)0;
	}

	y = pTree->root;
	z =(Rb_Node*)0 ;

	while(y != &pTree->sentinel) {

		Get_Object(y->key, &cur_key);
		Get_Object(x->key, &new_key);

		c = Compare_Key(new_key , cur_key);
		xFree(cur_key);
		xFree(new_key);
		if(c == 0) {
			return -1;
		}
		z = y;
		if(c < 0)
			y = y->left;
		else
			y = y->right;
	}    
	x->parent = z;
	if(z) {
		Get_Object(z->key, &cur_key);
		Get_Object(x->key, &new_key);

		c = Compare_Key(new_key, cur_key);
		xFree(cur_key);
		xFree(new_key);
		if(c < 0) {
			z->left = x;
		} else {
			z->right = x;
		}
	}
	else
		pTree->root = x;

	Rb_Insert_Fixup(pTree, x);
	return 0;
}

Rb_Node* Rb_Remove(Rb* pTree, void* key) {
	int c = 0;
	void* cur_key;
	Rb_Node* z = (Rb_Node*)0 ;

	z = pTree->root;
	while (z != &pTree->sentinel) {

		Get_Object(z->key, &cur_key);
		c = Compare_Key(key, cur_key);
		xFree(cur_key);
		if (c == 0) {
			break;
		}
		else {
			z = (c < 0) ? z->left : z->right;
		}
	}
	if (z == &pTree->sentinel)
		return (Rb_Node*)0 ;
	return Rb_Remove(pTree, z);
}


Map* Map_New()
{
	Map* pMap  =(Map*)xMalloc(sizeof(Map));
	if(pMap ==(Map*)0)
		return(Map*)0;

	pMap->root  = Rb_New();
	if(pMap->root ==(Rb*)0)
		return(Map*)0;

	return pMap;
}

int Map_Insert(Map* pMap, void* key, size_t key_size, void* value,  size_t value_size)
{
	if (pMap == (Map*)0)
		return -1;

	return Rb_Insert(pMap->root, key, key_size, value, value_size);
}

int Map_Exists(Map* pMap, void* key)
{
	Rb_Node* node;

	if (pMap == (Map*)0)
		return 0;

	node = Rb_Find(pMap->root, key);
	if (node != (Rb_Node*)0 ) {
		return 1;
	}
	return 0; 
}

int Map_Remove(Map* pMap, void* key)
{
	Rb_Node* node;
	void* removed_node;
	if (pMap == (Map*)0)
		return -1;

	node = Rb_Remove(pMap->root, key);
	if (node != (Rb_Node*)0 ) {
		Get_Object(node->key, &removed_node);
		xFree(removed_node);
		Delete_Object(node->key);

		Get_Object(node->value, &removed_node);
		xFree(removed_node);
		Delete_Object (node->value);

		xFree(node);
	}
	return 0;
}

int Map_Find(Map* pMap, void* key, void**value)
{
	Rb_Node* node;

	if (pMap == (Map*)0)
		return 0;

	node = Rb_Find(pMap->root, key);
	if (node == (Rb_Node*)0) 
		return 0;

	Get_Object(node->value, value);
	return 1;
}

int Map_Delete(Map* pMap)
{
	int rc = 0;
	if (pMap!= (Map*)0 ){
		rc = Rb_Delete(pMap->root);
		xFree(pMap);
	}
	return rc;
}

static void Map_Preorder(Rb_Node* node, void (*Handler)(Rb_Node*)) 
{
	if (NULL != node) {  
		Map_Preorder(node->left, Handler);  
		Handler(node);  
		Map_Preorder(node->right, Handler);  
	} 
}

void Map_Foreach(Map* pMap, void (*Handler)(Rb_Node*))
{
	Rb_Node* node = NULL;
	if (NULL == pMap || NULL == pMap->root){
		return;
	}
	node = pMap->root->root;
	Map_Preorder(node, Handler);
}  