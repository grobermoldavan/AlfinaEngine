#ifndef AL_STATIC_UNORDERED_LIST_H
#define AL_STATIC_UNORDERED_LIST_H

#include <cstddef>
#include <array>
#include <concepts>
#include <iostream>

#include "function.h"
#include "stack.h"

namespace al
{
    // @NOTE : This is a custom data structure that acts mostly like a linked list.
    //         New elements can be inserted reasonably fast (only few pointers must 
    //         be updated), but unlike most list implementations, this one does not
    //         dynamically allocate any memory when inserting elements - all nodes
    //         are pre-allocated and stored in a single block of memory.
    //         Time complexity of find_free_node method is O(n) in worst case, where
    //         "n" is a number of nodes in list, so whole insertion operation takes O(n).
    //         Removing elements also has O(n) time complexity in worst case
    //         (because of find_prev_node method).
    
    template<std::default_initializable T, std::size_t Size>
    class SuList
    {
    public:
        struct SuListNode
        {
            T value;
            SuListNode* nextNode;
        };

        struct SuListNodePair
        {
            SuListNode* node;
            SuListNode* prev;
        };

        SuList() noexcept
            : nodes{ }
            , head{ nullptr }
            , size{ 0 }
        {
            for (std::size_t it = 0; it < Size; it++)
            {
                SuListNode& node = nodes[it];
                node.nextNode = nullptr;
            }
        }

        ~SuList() = default;

        T* get() noexcept
        {
            SuListNodePair pair = find_free_node();
            if (pair.node)
            {
                SuListNode* node = pair.node;
                if (pair.prev)
                {
                    SuListNode* prev = pair.prev;
                    node->nextNode = prev->nextNode;
                    prev->nextNode = node;
                }
                else
                {
                    // If we are here that means that we are inserting 
                    // first element to the list, so we set head's nextNode to itself
                    node->nextNode = node;
                    head = node;
                }

                size++;
            }

            return reinterpret_cast<T*>(pair.node);
        }

        void remove(T* element) noexcept
        {
            // @TODO : add input element validation :
            //         - might be nullptr
            //         - might be wrong pointer (not belongs to this list)
            //         - might be a pointer to an already removed element

            SuListNode* node = reinterpret_cast<SuListNode*>(element);
            SuListNode* prev = find_prev_node(node);

            if (prev == node)
            {
                head = nullptr;
            }
            else
            {
                prev->nextNode = node->nextNode;
                if (node == head)
                {
                    head = node->nextNode;
                }
            }

            node->value.~T();
            node->nextNode = nullptr;

            size--;
        }

        void remove_by_condition(al::Function<bool(T*)> compare)
        {
            StaticStack<T*, Size> toRemove;
            for_each([&](T* element)
            {
                if (compare(element))
                {
                    toRemove.push(element);
                }
            });

            T* element;
            while(toRemove.pop(&element))
            {
                remove(element);
            }
        }

        std::size_t get_size() const noexcept
        {
            return size;
        }

        // @NOTE : no bounds check
        T& at(std::size_t index) noexcept
        {
            std::size_t it = 0;
            SuListNode* node = head;
            while (it != index)
            {
                node = node->nextNode;
                it++;
            }
            return node->value;
        }

        // @NOTE : no bounds check
        const T& at(std::size_t index) const noexcept
        {
            std::size_t it = 0;
            SuListNode* node = head;
            while (it != index)
            {
                node = node->nextNode;
                it++;
            }
            return node->value;
        }

        // @NOTE : no bounds check
        T& operator [] (std::size_t index) noexcept
        {
            return at(index);
        }

        // @NOTE : no bounds check
        const T& operator [] (std::size_t index) const noexcept
        {
            return at(index);
        }

        void for_each(al::Function<void(T*)> func)
        {
            SuListNode* begin = head;
            SuListNode* node = head;
            if (!node)
            {
                return;
            }

            while(node)
            {
                func(reinterpret_cast<T*>(node));
                node = node->nextNode;
                if (node == begin)
                {
                    break;
                }
            }
        }

        void for_each_interruptible(al::Function<bool(T*)> func)
        {
            SuListNode* begin = head;
            SuListNode* node = head;
            if (!node)
            {
                return;
            }

            while(node)
            {
                const bool result = func(reinterpret_cast<T*>(node));
                if (!result)
                {
                    break;
                }

                node = node->nextNode;
                if (node == begin)
                {
                    break;
                }
            }
        }

        void dbg_print() const noexcept
        {
            for_each([](T* node)
            {
                std::cout << "[" << *node << "]";
            });
        }

    private:
        std::array<SuListNode, Size> nodes;
        SuListNode* head;
        std::size_t size;

        SuListNodePair find_free_node() noexcept
        {
            if (!head)
            {
                return { &nodes[0], nullptr };
            }

            SuListNode* startNode = head;
            SuListNode* prev = nullptr;
            SuListNode* node = head;
            while(!is_node_free(node))
            {
                prev = node;
                node = step_right(node);
                if (node == startNode)
                {
                    // List is full
                    return { nullptr, nullptr };
                }
            }

            return { node, prev };
        }

        SuListNode* find_prev_node(SuListNode* target) noexcept
        {
            // Previous node is that node, which nextNode value
            // points to the target
            // @TODO : add input node validation :
            //         - might be a nullptr
            //         - might be the wrong pointer (not belongs to this list)
            //         - might be pointer to a free node (current realization
            //           of this method does not handle this situation)

            SuListNode* node = target;
            while(true)
            {
                if (node->nextNode == target)
                {
                    break;
                }
                node = step_left(node);
            }
            
            return node;
        }

        inline bool is_node_belongs_to_list(SuListNode* node) const noexcept
        {
            return (nodes[0] <= node) && (nodes[Size - 1] >= node);
        }

        inline bool is_node_free(SuListNode* node) const noexcept
        {
            return node->nextNode == nullptr;
        }

        inline SuListNode* step_right(SuListNode* node) noexcept
        {
            if (node == &nodes[Size - 1])
            {
                return &nodes[0];
            }
            return node + 1;
        }

        inline SuListNode* step_left(SuListNode* node) noexcept
        {
            if (node == &nodes[0])
            {
                return &nodes[Size - 1];
            }
            return node - 1;
        }
    };
}

#endif
