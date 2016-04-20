/*
*
*    Dictionary Encoding and the RDF Storage Model
*       by Adam Stallard , Aranya Ajith
*          
*    COP5725 Advanced Databases
*       Spring 2016
*
*    "trie.h"
*
*     This source file
*
*
*
*	PUT THIS IN THE HEADER FILE
*
*
*
*/

#include "bplus.h"
#include "trie.h"
#include <iostream>

// B+ tree's constructor
// Sets everything to NULL and nothing is allocated
//  until the first insertion
template <typename Tkey, typename Tdat>
Bplus<Tkey, Tdat>::Bplus() 
{
	headLeaf = tailLeaf = NULL;
	root = NULL;
}

// B+ tree's destructor
// Clears the tree before dipping to dealloacte pointers
// This is done recursively in a depth first function
template <typename Tkey, typename Tdat>
Bplus<Tkey, Tdat>::~Bplus() 
{
	this->clear();
}

// Merges two inner Nodes of the tree together where
//  N is left, M is right, and P is the parent Node
// This occurs when there is an underflow among the Nodes
//  typically a series of removals
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::remove_result 
Bplus<Tkey, Tdat>::merge_iNodes(
	Bplus<Tkey, Tdat>::innerNode *N, 
	Bplus<Tkey, Tdat>::Node *M, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot) 
{
	N->keys[N->slots_used] = P->keys[p_slot];
	M->slots_used++;
	
	std::copy(M->keys, M->keys + M->slots_used,
		N->keys + N->slots_used);
	std::copy(M->pointers, M->pointers + M->slots_used + 1,
		N->pointers + N->slots_used);
		
	N->slots_used += M->slots_used;
	M->slots_used = 0;
	
	return remove_result(ok);
}

// Merges two leaf Nodes of the tree together where
//  N is left, M is right, and P is the parent Node
// This occurs when there is an underflow among the Nodes
//  typically a series of removals
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::remove_result 
Bplus<Tkey, Tdat>::merge_lNodes(
	Bplus<Tkey, Tdat>::leafNode *N, 
	Bplus<Tkey, Tdat>::leafNode *M, 
	Bplus<Tkey, Tdat>::Node *P)
{
	std::copy(M->keys, M->keys + M->slots_used,
		N->keys + N->slots_used);
	std::copy(M->pointers, M->pointers + M->slots_used,
		N->pointers + N->slots_used);
	
	N->slots_used += M->slots_used;
	
	N->next = M->next;
	if (N->next)
		N->next->prev = N;
	else
		this->tailLeaf = N;
	
	M->slots_used = 0;
	
	return remove_result(ok);
}

// Splits an inner Node (called iNode) of the tree apart where
// This occurs when there is an overflow in the Node
//  typically a series of insertions
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::split_iNodes(
	Bplus<Tkey, Tdat>::innerNode *iNode, 
	Tkey *_newKey, 
	Node **_newInner, 
	unsigned int addSlot) 
{
	unsigned int mid = (iNode->slots_used >> 1);
	innerNode *newInner = new innerNode();

	if (addSlot <= mid && mid > iNode->slots_used - (mid + 1))
		mid--;
	
	newInner->slots_used = iNode->slots_used - (mid + 1);
	
	std::copy(iNode->keys + mid + 1, iNode->keys + iNode->slots_used,
		newInner->keys);
	std::copy(iNode->pointers + mid + 1, iNode->pointers + iNode->slots_used + 1,
		newInner->pointers);
	
	iNode->slots_used = mid;
	
	*_newKey = iNode->keys[mid];
	*_newInner = newInner;
}

// Splits a leaf Node (called lNode) of the tree apart where
// This occurs when there is an overflow in the Node
//  typically a series of insertions
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::split_lNodes(
	Bplus<Tkey, Tdat>::leafNode *lNode, 
	Tkey *_newKey, 
	Node **_newLeaf) 
{
	unsigned int mid = (lNode->slots_used >> 1);
	leafNode *newLeaf = new leafNode();
	
	newLeaf->slots_used = lNode->slots_used - mid;
	newLeaf->next = lNode->next;
	
	if (newLeaf->next == NULL) 
	{
		this->tailLeaf = newLeaf;
	}
	else 
	{
		std::cout << newLeaf->next;
		newLeaf->next->prev = newLeaf;
	}
	
	std::copy(lNode->keys + mid, lNode->keys + lNode->slots_used,
		newLeaf->keys);
	std::copy(lNode->pointers + mid, lNode->pointers + lNode->slots_used,
		newLeaf->pointers);

	lNode->slots_used = mid;
	lNode->next = newLeaf;
	newLeaf->prev = lNode;

	*_newKey = lNode->keys[lNode->slots_used - 1];
	*_newLeaf = newLeaf;
}

// A recursive solution to clearing the tree
// Simply performas a depth first search through the 
//  pointers to go to the leaves and begin deallocating
//  the templated Tdat pointers at the end recursively removing
//  the rest of the tree
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::clear_recursive(
	Bplus<Tkey, Tdat>::Node *N) 
{
	if (!N)
		return;
	else if (N->isleaf) 
	{
		Bplus::leafNode *lNode = static_cast<Bplus::leafNode *>(N);
		for (unsigned int slot = 0; slot < lNode->slots_used; ++slot) 
		{
			delete static_cast<Tdat *>(lNode->pointers[slot]);
		}
		delete static_cast<Bplus<Tkey, Tdat>::leafNode *>(N);
	}
	else 
	{
		Bplus::innerNode *iNode = static_cast<Bplus<Tkey, Tdat>::innerNode *>(N);
		for (unsigned int slot = 0; slot < iNode->slots_used; ++slot) 
		{
			clear_recursive(static_cast<Bplus<Tkey, Tdat>::Node *>(iNode->pointers[slot]));
			delete static_cast<Bplus<Tkey, Tdat>::Node *>(iNode->pointers[slot]);
		}
		delete static_cast<Bplus<Tkey, Tdat>::innerNode *>(N);
	}
}

// This is the recursive function for insertion
// Since Bplus trees dont grow very large in height,
//  but rather in width, the recursive approach is a 
// good choice here
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::Node * 
Bplus<Tkey, Tdat>::insert_recursive(
	Node *N, 
	const Tkey &key, 
	Tdat &dat, 
	Tkey *splitKey, 
	Node **splitNode) 
{

	if (N == NULL) 
		std::cerr << "Insert recursive: Null Node * found\n";
	
	if (!N->isleaf) 
	{
		innerNode *iNode = static_cast<innerNode *>(N);
		Tkey newKey = Tkey();
		Node *newChild = NULL;
		
		int slot = lower_key_idx(N, key);
		Node *rr = insert_recursive(static_cast<Node *>(iNode->pointers[slot]), key, dat, &newKey, &newChild);
		
		if (newChild) 
		{	
			if (iNode->isfull()) 
			{	
				split_iNodes(iNode, splitKey, splitNode, slot);
				if (slot == iNode->slots_used + 1 && iNode->slots_used < (*splitNode)->slots_used)
				{
					innerNode *splitInner = static_cast<innerNode *>(*splitNode);
					iNode->keys[iNode->slots_used] = *splitKey;
					iNode->pointers[iNode->slots_used + 1] = splitInner->pointers[0];
					iNode->slots_used++;
					splitInner->pointers[0] = newChild;
					*splitKey = newKey;
					return rr;
				}
				else if (slot >= iNode->slots_used + 1) 
				{
					slot -= iNode->slots_used + 1;
					iNode = static_cast<innerNode *>(*splitNode);
				}
			}
			
			std::copy_backward(iNode->keys + slot, iNode->keys + iNode->slots_used,
				iNode->keys + iNode->slots_used + 1);
			std::copy_backward(iNode->pointers + slot, iNode->pointers + iNode->slots_used + 1,
				iNode->pointers + iNode->slots_used + 2);
			
			iNode->keys[slot] = newKey;
			iNode->pointers[slot + 1] = newChild;
			iNode->slots_used++;
		}
		
		return rr;
	}
	else 
	{
		leafNode *lNode = static_cast<leafNode *>(N);
		int slot = lower_key_idx(N, key);
		
		if (slot < lNode->slots_used && key == lNode->keys[slot])
			return lNode;
		
		if (lNode->isfull()) 
		{

			// std::cout << "splitting leaf node . . .\n";

			split_lNodes(lNode, splitKey, splitNode);
			
			
			if (slot >= lNode->slots_used) 
			{
				slot -= lNode->slots_used;
				lNode = static_cast<leafNode *>(*splitNode);
			}
		}
		
		std::copy_backward(lNode->keys + slot, lNode->keys + lNode->slots_used,
				lNode->keys + lNode->slots_used + 1);
		std::copy_backward(lNode->pointers + slot, lNode->pointers + lNode->slots_used,
			lNode->pointers + lNode->slots_used + 1);

		lNode->keys[slot] = key;
		lNode->pointers[slot] = &dat;
		lNode->slots_used++;

		if (splitNode && lNode != *splitNode && slot == lNode->slots_used - 1) 
		{
			*splitKey = key;
		}
		
		return lNode;
	}
}

// The traversal function makes an attempt to print each Node in a
//  depth first recursive ordering. It is'nt very pretty for large 
//  trees but useful for debugging on small ones
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::traverse(
	Bplus<Tkey, Tdat>::Node *N)
{
	this->printNode(N);
	for (int key = 0; key < N->slots_used; ++key)
		traverse(static_cast<Bplus<Tkey, Tdat>::Node *>(N->pointers[key]));
}

// Simple print utility to show contents of a single Node
// Good for debugging and called by traverse
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::printNode(
	Bplus<Tkey, Tdat>::Node *N)
{
	if (!N) return;
	std::cout << "----------------------------------------------\n|";
	for (int key = 0; key < this->max_slots; ++key) 
	{
		if (key < N->slots_used)
		{
			std::cout << std::fixed << std::setprecision(8) << N->keys[key] << "|";
		}
		else
		{
			std::cout << std::fixed << std::setprecision(8) << -1 << "|";
		}
	}
	std::cout << "\n----------------------------------------------\n";
	for (int ptr = 0; ptr < this->max_slots; ++ptr) 
	{
		if (ptr < N->slots_used)
		{
			std::cout << "    x   |";
		}
		else
		{
			std::cout << "        |";
		}
	}
	std::cout << "\n----------------------------------------------\n";
}

// Simple find command which dives into the tree
//  by searching on key slots
//  and choosing pointer slots to travsere into
//  Once at the leaf Node, it will find the correct index for 
//  the key maps to. This does not guarentee the pointer
//  is valid however so our dictionary object must make
//  a NULL ptr check
template <typename Tkey, typename Tdat>
Tdat * 
Bplus<Tkey, Tdat>::find(
	Tkey key) 
{
	Node *N = this->root;
	if (!N) return NULL;
	
	while (!N->isleaf) 
	{
		int slot = lower_key_idx(N, key);
		N = static_cast<Bplus<Tkey, Tdat>::Node *>(N->pointers[slot]);
	}
	
	int slot = lower_key_idx(N, key);
	
	return static_cast<Tdat *>(N->pointers[slot]);
}

// Wrapper for the recursive insertion function to handle things
//  like null ptrs, starting at the root, and creating a new 
//  root if a original was split
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::Node *
Bplus<Tkey, Tdat>::insert(
	Tkey &key, 
	Tdat &dat) 
{
	Node *newChild = NULL;
	Tkey newKey = Tkey();
	
	if (this->root == NULL) 
	{
		this->root = new leafNode();
		this->headLeaf = this->tailLeaf = this->root;
	}
	
	Node *return_val = insert_recursive(this->root, key, dat, &newKey, &newChild);
	
	if (newChild) 
	{	
		innerNode *newRoot = new innerNode();
		newRoot->keys[0] = newKey;
		newRoot->pointers[0] = this->root;
		newRoot->pointers[1] = newChild;
		newRoot->slots_used = 1;
		this->root = newRoot;	
	}
	
	return return_val;
}

// Wrapper for the removal function to handle things
//  like null ptrs, and starting at the root
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::remove_result 
Bplus<Tkey, Tdat>::remove(
	Tkey key)
{
	if (!this->root) 
		return remove_result(ok);
	remove_result rr = remove_recursive(key, this->root, NULL, NULL, NULL, NULL, NULL, 0);
	return rr;
}

// The recursive removal function which searches down the 
//  tree by iterating over pointer slots and choosing the correct 
//  traversal path. If the key is found, the corresponding 
//  pointer is removed. The dictionary does not use this functionality
//  for our test however in that situation it would have to handle 
//  removing the corresponding string from the Trie and reallocating
//  ID values for new strings
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::remove_result 
Bplus<Tkey, Tdat>::remove_recursive(
	Tkey key, 
	Bplus<Tkey, Tdat>::Node *N, 
	Bplus<Tkey, Tdat>::Node *L, 
	Bplus<Tkey, Tdat>::Node *R, 
	Bplus<Tkey, Tdat>::Node *LP, 
	Bplus<Tkey, Tdat>::Node *RP, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot)
{
	if (N->isleaf)
	{
		leafNode *lNodeN = static_cast<leafNode *>(N);
		leafNode *lNodeL = static_cast<leafNode *>(L);
		leafNode *lNodeR = static_cast<leafNode *>(R);
		
		int slot = lower_key_idx(lNodeN, key);
		
		if (slot >= lNodeN->slots_used || (key != lNodeN->keys[slot]))
		{
			return remove_result(key_not_found);
		}
		
		std::copy(lNodeN->keys + slot + 1, lNodeN->keys + lNodeN->slots_used,
			lNodeN->keys + slot);
		std::copy(lNodeN->pointers + slot + 1, lNodeN->pointers + lNodeN->slots_used,
			lNodeN->pointers + slot);
		
		lNodeN->slots_used--;
		
		remove_result rr = remove_result(ok);
		
		if (slot == lNodeN->slots_used)
		{
			if (P && p_slot < P->slots_used)
			{
				P->keys[p_slot] = lNodeN->keys[lNodeN->slots_used - 1];
			}
			else
			{
				if (lNodeN->slots_used >= 1)
				{
					rr |= remove_result(update_last_key, lNodeN->keys[lNodeN->slots_used - 1]);
				}
			}
		}
		
		if (lNodeN->isunderflow() && !(lNodeN == this->root && lNodeN->slots_used >= 1))
		{
			if (lNodeL == NULL && lNodeR == NULL) 
			{
				delete this->root;
				this->root = lNodeN = NULL;
				this->tailLeaf = this->headLeaf = NULL;
				
				return remove_result(ok);
			}
			else if ((lNodeL == NULL || lNodeL->isfew()) && (lNodeR == NULL || lNodeR->isfew()))
			{
				if (LP == P)
					rr |= merge_lNodes(lNodeL, lNodeN, LP);
				else
					rr |= merge_lNodes(lNodeN, lNodeR, RP);
			}
			else if ((lNodeL != NULL && lNodeL->isfew()) && (lNodeR != NULL && lNodeR->isfew()))
			{
				if (RP == P)
					rr |= shift_lNodeL(lNodeN, lNodeR, RP, p_slot);
				else
					rr |= merge_lNodes(lNodeL, lNodeN, LP);
			}			
			else if ((lNodeN != NULL && !lNodeN->isfew()) && (lNodeR != NULL && lNodeR->isfew()))
			{
				if (LP == P)
					shift_lNodeR(lNodeL, lNodeN, LP, p_slot - 1);
				else
					rr |= merge_lNodes(lNodeN, lNodeR, RP);
			}
			else if (LP == RP)
			{
				if (lNodeL->slots_used <= lNodeR->slots_used)
					rr |= shift_lNodeL(lNodeN, lNodeR, RP, p_slot);
				else
					shift_lNodeR(lNodeL, lNodeN, LP, p_slot - 1);
			}
			else
			{
				if (LP == P)
					shift_lNodeR(lNodeL, lNodeN, LP, p_slot - 1);
				else
					rr |= shift_lNodeL(lNodeN, lNodeR, RP, p_slot);
			}
		}
		
		return remove_result(rr);
	}
	else
	{
		innerNode *iNodeN = static_cast<innerNode *>(N);
		innerNode *iNodeL = static_cast<innerNode *>(L);
		innerNode *iNodeR = static_cast<innerNode *>(R);
		
		Node *_L, *_R;
		innerNode *_LP, *_RP;
		
		int slot = lower_key_idx(iNodeN, key);
		
		if (slot == 0)
		{
			_L = (L == NULL) ? NULL 
				: static_cast< Bplus<Tkey, Tdat>::Node * >((static_cast<innerNode *>(L))->pointers[L->slots_used - 1]);
			_LP = static_cast<Bplus<Tkey, Tdat>::innerNode *>(LP);
		}
		else
		{
			_L = static_cast<Bplus<Tkey, Tdat>::Node *>(iNodeN->pointers[slot - 1]);
			_LP = iNodeN;
		}
		
		if (slot == iNodeN->slots_used)
		{
			_R = (R == NULL) ? NULL 
				: static_cast< Bplus<Tkey, Tdat>::Node * >((static_cast<innerNode *>(R))->pointers[0]);
			_RP = static_cast<Bplus<Tkey, Tdat>::innerNode *>(RP);;
		}
		else
		{
			_R = static_cast< Bplus<Tkey, Tdat>::Node * >(iNodeN->pointers[slot + 1]);
			_RP = iNodeN;
		}
		
		remove_result result = remove_recursive(key, static_cast<Bplus<Tkey, Tdat>::Node *>(iNodeN->pointers[slot]), _L, _R, _LP, _RP, iNodeN, slot);
		
		remove_result rr = remove_result(ok);
		
		if (result.has(key_not_found))
		{
			return remove_result(result);
		}
		
		if (result.has(update_last_key))
		{
			if (P && p_slot < P->slots_used)
			{
				P->keys[p_slot] = result.last_key;
			}
			else
			{
				rr |= remove_result(update_last_key, result.last_key);
			}
		}
		
		if (result.has(fix_merge))
		{
			if (static_cast< Bplus<Tkey, Tdat>::Node * >(iNodeN->pointers[slot])->slots_used != 0)
				slot++;

			delete static_cast< Bplus<Tkey, Tdat>::Node * >(iNodeN->pointers[slot]);

			std::copy(iNodeN->keys + slot, iNodeN->keys + iNodeN->slots_used,
					  iNodeN->keys + slot - 1);
			std::copy(iNodeN->pointers + slot + 1, iNodeN->pointers + iNodeN->slots_used + 1,
					  iNodeN->pointers + slot);

			iNodeN->slots_used--;

			// trying to fake (iNodeN->level == 1) // i.e. children are leaves
			if (iNodeN->slots_used && 
				static_cast< Bplus<Tkey, Tdat>::Node * >(iNodeN->pointers[0])->isleaf)
			{
				slot--;
				leafNode* child = static_cast<leafNode *>(iNodeN->pointers[slot]);
				iNodeN->keys[slot] = child->keys[child->slots_used - 1];
			}
			
		}
		
		if (iNodeN->isunderflow() && !(iNodeN == this->root && iNodeN->slots_used >= 1))
		{
			if (iNodeL == NULL && iNodeR == NULL)
			{
				this->root = static_cast< Bplus<Tkey, Tdat>::Node * >(iNodeN->pointers[0]);

				iNodeN->slots_used = 0;
				delete iNodeN;

				return remove_result(ok);
			}
			else if ((iNodeL == NULL || iNodeL->isfew()) && (iNodeR == NULL || iNodeR->isfew()))
			{
				if (LP == P)
					rr |= merge_iNodes(iNodeL, iNodeN, LP, p_slot - 1);
				else
					rr |= merge_iNodes(iNodeN, iNodeR, RP, p_slot);
			}
			else if ((iNodeL != NULL && iNodeL->isfew()) && (iNodeR != NULL && !iNodeR->isfew()))
			{
				if (RP == P)
					shift_iNodeL(iNodeN, iNodeR, RP, p_slot);
				else
					rr |= merge_iNodes(iNodeL, iNodeN, LP, p_slot - 1);
			}
			else if ((iNodeL != NULL && !iNodeL->isfew()) && (iNodeR != NULL && iNodeR->isfew()))
			{
				if (LP == P)
					shift_iNodeR(iNodeL, iNodeN, LP, p_slot - 1);
				else
					rr |= merge_iNodes(iNodeN, iNodeR, RP, p_slot);
			}
			else if (LP == RP)
			{
				if (iNodeL->slots_used <= iNodeR->slots_used)
					shift_iNodeL(iNodeN, iNodeR, RP, p_slot);
				else
					shift_iNodeR(iNodeL, iNodeN, LP, p_slot - 1);
			}
			else
			{
				if (LP == P)
					shift_iNodeR(iNodeL, iNodeN, LP, p_slot - 1);
				else
					shift_iNodeL(iNodeN, iNodeR, RP, p_slot);
			}
		}
		
		return remove_result(rr);
	}
}

// This is a utility function which essentially shifts
//  key and pointer data over from one leaf to its left neighbor
//  using std copy functions
template <typename Tkey, typename Tdat>
typename Bplus<Tkey, Tdat>::remove_result 
Bplus<Tkey, Tdat>::shift_lNodeL(
	Bplus<Tkey, Tdat>::Node *N, 
	Bplus<Tkey, Tdat>::Node *M, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot)
{
	unsigned int shiftnum = (M->slots_used - N->slots_used) >> 1;

	std::copy(M->keys, M->keys + shiftnum,
		N->keys + N->slots_used);
	std::copy(M->pointers, M->pointers + shiftnum,
		N->pointers + N->slots_used);

	N->slots_used += shiftnum;

	std::copy(M->keys + shiftnum, M->keys + M->slots_used,
		M->keys);
	std::copy(M->pointers + shiftnum, M->pointers + M->slots_used,
		M->pointers);

	M->slots_used -= shiftnum;

	if (p_slot < P->slots_used) 
	{
		P->keys[p_slot] = N->keys[N->slots_used - 1];
		return remove_result(ok);
	}
	else
	{
		return remove_result(update_last_key, N->keys[N->slots_used - 1]);
	}
}

// This is a utility function which essentially shifts 
//  key and pointer data over from one leaf to its right neighbor
//  using std copy functions
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::shift_lNodeR(
	Bplus<Tkey, Tdat>::Node *N, 
	Bplus<Tkey, Tdat>::Node *M, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot)
{
	unsigned int shiftnum = (N->slots_used - M->slots_used) >> 1;

	std::copy(M->keys, M->keys + shiftnum,
		N->keys + N->slots_used);
	std::copy(M->pointers, M->pointers + shiftnum,
		N->pointers + N->slots_used);

	N->slots_used += shiftnum;

	std::copy_backward(M->keys, M->keys + M->slots_used,
		M->keys + M->slots_used + shiftnum);
	std::copy_backward(M->pointers, M->pointers + M->slots_used,
		M->pointers + M->slots_used + shiftnum);

	M->slots_used += shiftnum;

	std::copy(N->keys + N->slots_used - shiftnum, N->keys + N->slots_used,
		M->keys);
	std::copy(N->pointers + N->slots_used - shiftnum, N->pointers + N->slots_used,
		M->pointers);
		
	N->slots_used -= shiftnum;
	
	P->keys[p_slot] = N->keys[N->slots_used - 1];
}

// This is a utility function which essentially shifts 
//  key and pointer data over from one inner Node to its 
// left neighbor using std copy functions
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::shift_iNodeL(
	Bplus<Tkey, Tdat>::Node *N, 
	Bplus<Tkey, Tdat>::Node *M, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot)
{
	unsigned int shiftnum = (M->slots_used - N->slots_used) >> 1;
	
	N->keys[N->slots_used] = P->keys[p_slot];
	N->slots_used++;
	
	std::copy(M->keys, M->keys + shiftnum - 1,
		N->keys + N->slots_used);
	std::copy(M->pointers, M->pointers + shiftnum,
		N->pointers + N->slots_used);
		
	N->slots_used += shiftnum - 1;
	
	P->keys[p_slot] = M->keys[shiftnum - 1];
	
	std::copy(M->keys + shiftnum, M->keys + M->slots_used,
		M->keys);
	std::copy(M->pointers + shiftnum, M->pointers + M->slots_used + 1,
		M->pointers);
		
	M->slots_used -= shiftnum;
}

// This is a utility function which essentially shifts 
//  key and pointer data over from one inner Node to its 
// right neighbor using std copy functions
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::shift_iNodeR(
	Bplus<Tkey, Tdat>::Node *N, 
	Bplus<Tkey, Tdat>::Node *M, 
	Bplus<Tkey, Tdat>::Node *P, 
	unsigned int p_slot)
{
	unsigned int shiftnum = (M->slots_used - N->slots_used) >> 1;
	
	std::copy_backward(M->keys, M->keys + M->slots_used,
		M->keys + M->slots_used + shiftnum);
	std::copy_backward(M->pointers, M->pointers + M->slots_used + 1,
		M->pointers + M->slots_used + 1 + shiftnum);
	
	M->slots_used += shiftnum;
	
	M->keys[shiftnum - 1] = P->keys[p_slot];
	
	std::copy(N->keys + N->slots_used - shiftnum + 1, N->keys + N->slots_used,
		M->keys);
	std::copy(N->pointers + N->slots_used - shiftnum + 1, N->pointers + N->slots_used,
		M->pointers);
	
	P->keys[p_slot] = N->keys[N->slots_used - shiftnum];
	
	N->slots_used -= shiftnum;
}

// Simple wrapper for public use which always starts
// the clear at the root
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::clear() 
{
	if (this->root)
	{
		clear_recursive(this->root);
	}
}

// Simple wrapper for public use which always starts
// the traverse at the root
template <typename Tkey, typename Tdat>
void 
Bplus<Tkey, Tdat>::show() 
{
	if (this->root)
	{
		this->traverse(this->root);
	}
}




// Instantiate templates for the dictionary object
// We need to have the Bplus tree index on the Trie Node IDs
//  which represent the mapping for a string i.e. a (S,O,P) triple
// So we instantiate our key as a simple unsigned int for simple
//  counting, but we must set the data pointer type at the leaf Nodes
//  to point to the Nodes in our trie structure.
// The dictionary object constructs this for us using this template
template class Bplus<unsigned int, Trie::Node>;

// Testing
// template class Bplus<int, int>;
// template class Bplus<int, Trie::Node>;
