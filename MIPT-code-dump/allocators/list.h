#include <memory>
#include <cassert>

template<class T, class Allocator = std::allocator<T> >
class List
{
public:
    struct Node
    {
        T value;
        Node *next;
        Node *prev;

        Node(const T &value)
            : value(value)
            , next(nullptr)
            , prev(nullptr)
        {}

        Node(const T &&value)
            : value(value)
            , next(nullptr)
            , prev(nullptr)
        {}
    };

    typedef size_t size_type;

private:
    typename Allocator::template rebind<Node>::other alloc_;
    Node *head_ = nullptr;
    Node *tail_ = nullptr;
    size_t size_ = 0;

    Node * insert_before_(Node *node, Node *ptr)
    {
        ++size_;
        if (!node) {
            head_ = tail_ = ptr;
        } else {
            if (node->prev) {
                node->prev->next = ptr;
            } else {
                head_ = ptr;
            }
            ptr->prev = node->prev;
            ptr->next = node;
            node->prev = ptr;
        }
        return ptr;
    }

    Node * insert_after_(Node *node, Node *ptr)
    {
        ++size_;
        if (!node) {
            head_ = tail_ = ptr;
        } else {
            if (node->next) {
                node->next->prev = ptr;
            } else {
                tail_ = ptr;
            }
            ptr->next = node->next;
            ptr->prev = node;
            node->next = ptr;
        }
        return ptr;
    }

    template<class U>
    Node * construct_node_(U &&arg)
    {
        Node *n = alloc_.allocate(1);
        alloc_.construct(n, std::forward<U>(arg));
        return n;
    }

    void destroy_node_(Node *n)
    {
        alloc_.destroy(n);
        alloc_.deallocate(n, 1);
    }

public:
    explicit List(const Allocator &alloc = Allocator())
        : alloc_(alloc)
    {}

    List(size_type count, const T &value = T(), const Allocator &alloc = Allocator())
        : alloc_(alloc)
    {
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    List(const List &that)
        : alloc_(that.alloc)
    {
        for (Node *p = that.head_; p; p = p->next) {
            push_back(p->value);
        }
    }

    List(List &&that)
        : alloc_(std::move(that.alloc))
        , head_(that.head_)
        , tail_(that.tail_)
        , size_(that.size_)
    {
        that.head_ = nullptr;
        that.tail_ = nullptr;
        that.size_ = nullptr;
    }

    List& operator =(const List &that)
    {
        if (this != &that) {
            while (size_) {
                pop_back();
            }
            for (Node *p = that.head_; p; p = p->next) {
                push_back(p->value);
            }
        }
        return *this;
    }

    List& operator =(List &&that)
    {
        std::swap(*this, that);
        return *this;
    }

    void erase(Node *node)
    {
        --size_;
        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;
        }
        destroy_node_(node);
    }

    Node * push_back(const T  &value) { return insert_after_(tail_, construct_node_(value)); }
    Node * push_back(      T &&value) { return insert_after_(tail_, construct_node_(value)); }

    Node * push_front(const T  &value) { return insert_before_(head_, construct_node_(value)); }
    Node * push_front(      T &&value) { return insert_before_(head_, construct_node_(value)); }

    Node * insert_before(Node *n, const T  &value) { return insert_before_(n, construct_node_(value)); }
    Node * insert_before(Node *n,       T &&value) { return insert_before_(n, construct_node_(value)); }

    Node * insert_after(Node *n, const T  &value) { return insert_after_(n, construct_node_(value)); }
    Node * insert_after(Node *n,       T &&value) { return insert_after_(n, construct_node_(value)); }

    void pop_back() { erase(tail_); }
    void pop_front() { erase(head_); }

    size_t size() const { return size_; }

    ~List()
    {
        while (size_) {
            pop_back();
        }
    }
};
