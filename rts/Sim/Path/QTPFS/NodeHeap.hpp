/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODEHEAP_HDR
#define QTPFS_NODEHEAP_HDR

#include <limits>
#include <vector>
#include "PathDefines.hpp"

#define NODE_CMP_EQ(a, b) (a->operator==(b))
#define NODE_CMP_LT(a, b) (a->operator< (b))
#define NODE_CMP_LE(a, b) (a->operator<=(b))
#define NODE_CMP_GT(a, b) (a->operator> (b))
#define NODE_CMP_GE(a, b) (a->operator>=(b))

namespace QTPFS {
	template<class TNode> class binary_heap {
	public:
		binary_heap() { clear(); }
		binary_heap(size_t n) { reserve(n); }
		~binary_heap() { clear(); }

		// interface functions
		void push(TNode n) {
			#ifdef QTPFS_DEBUG_NODE_HEAP
			check_heap_property(0);
			#endif

			assert(cur_idx < nodes.size());

			// park new node at first free spot
			nodes[cur_idx] = n;
			nodes[cur_idx]->SetHeapIndex(cur_idx);

			if (cur_idx == max_idx)
				nodes.resize(nodes.size() * 2);

			cur_idx += 1;
			max_idx = nodes.size() - 1;

			// move new node up if necessary
			if (size() > 1)
				inc_heap(cur_idx - 1);

			#ifdef QTPFS_DEBUG_NODE_HEAP
			check_heap_property(0);
			#endif
		}

		void pop() {
			#ifdef QTPFS_DEBUG_NODE_HEAP
			check_heap_property(0);
			#endif

			assert(cur_idx <= max_idx);

			// exchange root and last node
			if (size() > 1)
				swap_nodes(0, cur_idx - 1);

			// former position of last node is now free
			cur_idx -= 1;
			assert(cur_idx <= max_idx);

			// kill old root
			nodes[cur_idx]->SetHeapIndex(-1u);
			nodes[cur_idx] = NULL;

			// move new root (former last node) down if necessary
			if (size() > 1)
				dec_heap(0);

			#ifdef QTPFS_DEBUG_NODE_HEAP
			check_heap_property(0);
			#endif
		}

		TNode top() {
			assert(!empty());
			return nodes[0];
		}


		// utility functions
		bool empty() const { return (size() == 0); }
//		bool full() const { return (size() >= capacity()); }
		size_t size() const { return (cur_idx); }
		size_t capacity() const { return (max_idx + 1); }

		void clear() {
			nodes.clear();

			cur_idx =  0;
			max_idx = -1u;
		}

		void reserve(size_t size) {
			nodes.clear();
			nodes.resize(size, NULL);

			cur_idx =        0;
			max_idx = size - 1;
		}

		// acts like reserve(), but without re-allocating
		void reset() {
			assert(!nodes.empty());

			cur_idx =                0;
			max_idx = nodes.size() - 1;
		}

		void resort(TNode n) {
			assert(n != NULL);
			assert(valid_idx(n->GetHeapIndex()));

			const size_t n_idx = n->GetHeapIndex();
			const size_t n_rel = is_sorted(n_idx);

			// bail if <n> does not actually break the heap-property
			if (n_rel == 0) {
				return;
			} else {
				if (n_rel == 1) {
					// parent of <n> is larger, move <n> further up
					inc_heap(n_idx);
				} else {
					// parent of <n> is smaller, move <n> further down
					dec_heap(n_idx);
				}
			}

			#ifdef QTPFS_DEBUG_NODE_HEAP
			check_heap_property(0);
			#endif
		}


		void check_heap_property(size_t idx) const {
			#ifdef QTPFS_DEBUG_NODE_HEAP
			if (valid_idx(idx)) {
				assert(is_sorted(idx) == 0);

				check_heap_property(l_child_idx(idx));
				check_heap_property(r_child_idx(idx));
			}
			#endif
		}

		#ifdef QTPFS_DEBUG_PRINT_NODE_HEAP
		void debug_print(size_t idx, size_t calls, const std::string& tabs) const {
			if (calls == 0)
				return;

			if (valid_idx(idx)) {
				debug_print(r_child_idx(idx), calls - 1, tabs + "\t");
				printf("%shi=%u :: hp=%.2f\n", tabs.c_str(), nodes[idx]->GetHeapIndex(), nodes[idx]->GetHeapPriority());
				debug_print(l_child_idx(idx), calls - 1, tabs + "\t");
			}
		}
		#endif

	private:
		size_t  parent_idx(size_t n_idx) const { return ((n_idx - 1) >> 1); }
		size_t l_child_idx(size_t n_idx) const { return ((n_idx << 1) + 1); }
		size_t r_child_idx(size_t n_idx) const { return ((n_idx << 1) + 2); }

		// tests if <idx> belongs to the tree built over [0, cur_idx)
		bool valid_idx(size_t idx) const { return (idx < cur_idx); }

		// test whether the node at <nidx> is in the right place
		// parent must have smaller / equal value, children must
		// have larger / equal value
		//
		size_t is_sorted(size_t n_idx) const {
			const size_t   p_idx =  parent_idx(n_idx);
			const size_t l_c_idx = l_child_idx(n_idx);
			const size_t r_c_idx = r_child_idx(n_idx);

			if (n_idx != 0) {
				// check if parent > node
				assert(valid_idx(p_idx));
				assert(nodes[p_idx] != NULL);
				assert(nodes[n_idx] != NULL);

				if (NODE_CMP_GT(nodes[p_idx], nodes[n_idx])) {
					return 1;
				}
			}

			if (valid_idx(l_c_idx)) {
				// check if l_child < node
				assert(nodes[l_c_idx] != NULL);
				assert(nodes[  n_idx] != NULL);

				if (NODE_CMP_LT(nodes[l_c_idx], nodes[n_idx])) {
					return 2;
				}
			}
			if (valid_idx(r_c_idx)) {
				// check if r_child < node
				assert(nodes[r_c_idx] != NULL);
				assert(nodes[  n_idx] != NULL);

				if (NODE_CMP_LT(nodes[r_c_idx], nodes[n_idx])) {
					return 2;
				}
			}

			return 0;
		}


		void inc_heap(size_t c_idx) {
			size_t p_idx = 0;

			while (c_idx >= 1) {
				p_idx = parent_idx(c_idx);

				assert(valid_idx(p_idx));
				assert(valid_idx(c_idx));

				if (NODE_CMP_LE(nodes[p_idx], nodes[c_idx])) {
					break;
				}

				swap_nodes(p_idx, c_idx);

				// move up one level
				c_idx = p_idx;
			}
		}

		void dec_heap(size_t p_idx) {
			size_t   c_idx = -1u;
			size_t l_c_idx = l_child_idx(p_idx);
			size_t r_c_idx = r_child_idx(p_idx);

			while (valid_idx(l_c_idx) || valid_idx(r_c_idx)) {
				if (!valid_idx(r_c_idx)) {
					// node only has a left child
					c_idx = l_c_idx;
				} else {
					if (NODE_CMP_LE(nodes[l_c_idx], nodes[r_c_idx])) {
						// left child is smaller or equal to right child
						// according to the node total ordering, pick it
						c_idx = l_c_idx;
					} else {
						// pick the right child
						c_idx = r_c_idx;
					}
				}

				if (NODE_CMP_LE(nodes[p_idx], nodes[c_idx])) {
					// parent is smaller (according to the
					// node total ordering) than the child
					break;
				}

				swap_nodes(p_idx, c_idx);

				  p_idx = c_idx;
				l_c_idx = l_child_idx(p_idx);
				r_c_idx = r_child_idx(p_idx);
			}
		}


		void swap_nodes(size_t idx1, size_t idx2) {
			assert(idx1 != idx2);
			assert(idx1 < cur_idx);
			assert(idx2 < cur_idx);

			TNode n1 = nodes[idx1];
			TNode n2 = nodes[idx2];

			assert(n1 != n2);
			assert(n1->GetHeapIndex() == idx1);
			assert(n2->GetHeapIndex() == idx2);

			nodes[idx1] = n2;
			nodes[idx2] = n1;

			n2->SetHeapIndex(idx1);
			n1->SetHeapIndex(idx2);
		}

	private:
		std::vector<TNode> nodes;

		size_t cur_idx; // index of first free (unused) slot
		size_t max_idx; // index of last free (unused) slot
	};
}

#endif

