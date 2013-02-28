#pragma once
#include "../os/api_wrapper.h"
#include "geometry.h"
#include "../util/memory_pool.h"

/**
* \brief Implements a lock-free singly linked list using
* atomic operations.
*/
template <typename T> class LockFreeList {
public:
	struct ListItem {
		T value;
		ListItem *next;

		inline ListItem(const T &value) :
		value(value), next(NULL) { }
	};

	inline LockFreeList() : m_head(NULL) {}

	inline const ListItem *head() const {
		return m_head;
	}

	template <typename alloc_functor>
	void append(const T &value, alloc_functor& a_functor) {
		ListItem* pNew = (ListItem*)a_functor.Alloc(sizeof(ListItem));
		ListItem *item = new(pNew) ListItem(value);
		ListItem **cur = &m_head;
		while (!atomic_compare_exchange_ptr((void**)cur, item, NULL))
			cur = &((*cur)->next);
	}


	ListItem *m_head;
};

/**
* \brief Generic multiple-reference octree.
*
* Based on the excellent implementation in PBRT. Modifications are 
* the addition of a bounding sphere query and support for multithreading.
*/
template <typename T> class Octree {
public:
	/**
	* \brief Create a new octree
	*
	* By default, the maximum tree depth is set to 16
	*/

	inline Octree(int maxDepth = 16) 
		: m_maxDepth(maxDepth) {
			mItemCnt = 0;
	}

	inline Octree(const KBBox &aabb, int maxDepth = 16) 
		: m_aabb(aabb), m_maxDepth(maxDepth) {
			mItemCnt = 0;
	}

	/// Insert an item with the specified cell coverage
	inline void insert(const T &value, const KBBox &coverage) {
		KVec3 diag = coverage.mMax - coverage.mMin;
		insert(&m_root, m_aabb, value, coverage,
			nvmath::lengthSquared(diag), 0);
	}

	/// Execute operator() of functor on all records, which potentially overlap p
	template <typename Functor> inline void lookup(const KVec3 &p, Functor &functor) const {
		if (!m_aabb.IsInside(p))
			return;
		lookup(&m_root, m_aabb, p, functor);
	}

	/// Execute operator() of functor on all records, which potentially overlap bsphere
	template <typename Functor> inline void searchSphere(const KBSphere &sphere, Functor &functor) const {
		if (!m_aabb.IsOverlapping(sphere))
			return;
		searchSphere(&m_root, m_aabb, sphere, functor);
	}

	inline const KBBox &getAABB() const { return m_aabb; }
	inline void setInitAABB(const KBBox& box) {m_aabb = box;}
private:
	struct OctreeNode {
	public:
		OctreeNode() {
			for (int i=0; i<8; ++i)
				children[i] = NULL;
			baked_cnt = 0;
		}

		OctreeNode *children[8];

		LockFreeList<T> data;
		long baked_offset;
		long baked_cnt;
	};

	/// Return the AABB for a child of the specified index
	inline KBBox childBounds(int child, const KBBox &nodeAABB, const KVec3 &center) const {
		KBBox childAABB;
		childAABB.mMin[0] = (child & 4) ? center[0] : nodeAABB.mMin[0];
		childAABB.mMax[0] = (child & 4) ? nodeAABB.mMax[0] : center[0];
		childAABB.mMin[1] = (child & 2) ? center[1] : nodeAABB.mMin[1];
		childAABB.mMax[1] = (child & 2) ? nodeAABB.mMax[1] : center[1];
		childAABB.mMin[2] = (child & 1) ? center[2] : nodeAABB.mMin[2];
		childAABB.mMax[2] = (child & 1) ? nodeAABB.mMax[2] : center[2];
		return childAABB;
	}


	void insert(OctreeNode *node, const KBBox &nodeAABB, const T &value, 
		const KBBox &coverage, float diag2, int depth) {
			/* Add the data item to the current octree node if the max. tree
			depth is reached or the data item's coverage area is smaller
			than the current node size */
			KVec3 diag = nodeAABB.mMax - nodeAABB.mMin;
			if (depth == m_maxDepth || 
				(nvmath::lengthSquared(diag) < diag2)) {
					node->data.append(value, m_allocatorLeaf);
					atomic_increment(&mItemCnt);
					return;
			}

			/* Otherwise: test for overlap */
			const KVec3 center = nodeAABB.Center();

			/* Otherwise: test for overlap */
			bool x[2] = { coverage.mMin[0] <= center[0], coverage.mMax[0] > center[0] };
			bool y[2] = { coverage.mMin[1] <= center[1], coverage.mMax[1] > center[1] };
			bool z[2] = { coverage.mMin[2] <= center[2], coverage.mMax[2] > center[2] };
			bool over[8] = { x[0] & y[0] & z[0], x[0] & y[0] & z[1],
				x[0] & y[1] & z[0], x[0] & y[1] & z[1],
				x[1] & y[0] & z[0], x[1] & y[0] & z[1],
				x[1] & y[1] & z[0], x[1] & y[1] & z[1] };

			/* Recurse */
			for (int child=0; child<8; ++child) {
				if (!over[child])
					continue;
				if (!node->children[child]) {
					OctreeNode* pNew = (OctreeNode*)m_allocatorNode.Alloc(sizeof(OctreeNode));
					OctreeNode* newNode = new(pNew) OctreeNode();
					if (!atomic_compare_exchange_ptr((void**)&node->children[child], newNode, NULL)) {
						// Should do nothing with GlowableMemPool
						//delete newNode;
					}
				}
				const KBBox childAABB(childBounds(child, nodeAABB, center));
				insert(node->children[child], childAABB,
					value, coverage, diag2, depth+1);
			}
	}
	
	template <typename Functor> inline void visit_node(const OctreeNode *node, Functor &functor) const {
		const typename LockFreeList<T>::ListItem *item = node->data.head();
		while (item) {
			functor(item->value);
			item = item->next;
		}

		if (node->baked_cnt > 0) {
			long end_idx = node->baked_offset + node->baked_cnt;
			for (long i = node->baked_offset; i < end_idx; ++i) {
				functor(mBackedElements[i]);
			}
		}
	}

	/// Internal lookup procedure - const version
	template <typename Functor> inline void lookup(const OctreeNode *node, 
		const KBBox &nodeAABB, const KVec3 &p, Functor &functor) const {
			const KVec3 center = nodeAABB.Center();

			visit_node(node, functor);

			int child = (p[0] > center[0] ? 4 : 0)
				+ (p[1] > center[1] ? 2 : 0) 
				+ (p[2] > center[2] ? 1 : 0);

			OctreeNode *childNode = node->children[child];

			if (childNode) {
				const KBBox childAABB(childBounds(child, nodeAABB, center));
				lookup(node->children[child], childAABB, p, functor);
			}
	}

	template <typename Functor> inline void searchSphere(const OctreeNode *node, 
		const KBBox &nodeAABB, const KBSphere &sphere, 
		Functor &functor) const {
			const KVec3 center = nodeAABB.Center();

			visit_node(node, functor);

			// Potential for much optimization..
			for (int child=0; child<8; ++child) { 
				if (node->children[child]) {
					const KBBox childAABB(childBounds(child, nodeAABB, center));
					if (childAABB.IsOverlapping(sphere))
						searchSphere(node->children[child], childAABB, sphere, functor);
				}
			}
	}

	template <typename Functor> inline void iterate_all(const OctreeNode *node, Functor &functor) const {
		const typename LockFreeList<T>::ListItem *item = node->data.head();
		while (item) {
			functor(item->value);
			item = item->next;
		}

		visit_node(node, functor);

		// Potential for much optimization..
		for (int child=0; child<8; ++child) { 
			if (node->children[child]) {
				iterate_all(node->children[child], functor);
			}
		}
	}

	inline void bake_node(OctreeNode *node) {
		const typename LockFreeList<T>::ListItem *item = node->data.head();
		
		
		if (node->baked_cnt > 0) {
			long end_idx = node->baked_offset + node->baked_cnt;
			long start_idx = node->baked_offset;
			node->baked_offset = (long)mTempBackedElements.size();
			for (long i = start_idx; i < end_idx; ++i) {
				mTempBackedElements.push_back(mBackedElements[i]);
			}
		}
		else 
			node->baked_offset = (long)mTempBackedElements.size();

		while (item) {
			mTempBackedElements.push_back(item->value);
			++node->baked_cnt;
			item = item->next;
		}
		node->data.m_head = NULL;

		for (int child=0; child<8; ++child) { 
			if (node->children[child]) {
				bake_node(node->children[child]);
			}
		}
	}


public:
	void reset() {
		for (int i=0; i<8; ++i)
			m_root.children[i] = NULL;

		m_root.data.m_head = NULL;

		m_aabb.SetEmpty();
		m_allocatorLeaf.Reset();
		m_allocatorNode.Reset();
		mBackedElements.clear();
		mItemCnt = 0;
	}

	template <typename Functor> inline void iterate_all(Functor &functor) const {
		iterate_all(&m_root, functor);
	}

	inline void bake_to_array() {
		mTempBackedElements.reserve(mItemCnt);
		bake_node(&m_root);
		mBackedElements.swap(mTempBackedElements);
		mTempBackedElements.clear();
		m_allocatorLeaf.Reset();
	}

private:
	OctreeNode m_root;
	KBBox m_aabb;
	int m_maxDepth;
	std::vector<T> mBackedElements;
	std::vector<T> mTempBackedElements;
	GlowableMemPool m_allocatorLeaf;
	GlowableMemPool m_allocatorNode;
	volatile long mItemCnt;
};