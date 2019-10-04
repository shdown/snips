// This code is my end-off-course test, was written in 2 hours and is pretty shitty.
#include <stddef.h>
#include <iterator>
#include <functional>

// No STL allowed...
namespace bicycle
{
    template<typename T>
    void swap(T &a, T &b)
    {
        T tmp = a;
        a = b;
        b = tmp;
    }

    template<typename T, typename Compare>
    void mergeSort(T *start, T *finish, T *buf, Compare cmp)
    {
        size_t n = finish - start;
        if (n <= 1) {
            return;
        }

        size_t half = n / 2;
        T *p = start;
        T *pLim = p + half;
        T *q = p + half;
        T *qLim = finish;

        mergeSort(p, pLim, buf, cmp);
        mergeSort(q, qLim, buf, cmp);

        T *bufptr = buf;
        while (p < pLim || q < qLim) {
            if (q == qLim || (p != pLim && !cmp(*q, *p))) {
                *bufptr++ = *p++;
            } else {
                *bufptr++ = *q++;
            }
        }
        for (size_t i = 0; i < n; ++i) {
            start[i] = buf[i];
        }
    }
};

template<typename T>
class ForwardList
{
    struct Node
    {
        T data;
        Node *next;

        Node(const T &data_, Node *next_)
            : data(data_)
            , next(next_)
        {}
    };

    template<typename RefT>
    class IteratorTmpl : public std::iterator<std::forward_iterator_tag, T>
    {
        friend class ForwardList;

        Node *ptr_;

        explicit IteratorTmpl(Node *ptr)
            : ptr_(ptr)
        {}

    public:
        typedef T value_type;
        typedef RefT& reference_type;
        typedef RefT* pointer_type;

        IteratorTmpl(const IteratorTmpl<const RefT> &that)
            : ptr_(that.ptr_)
        {}

        IteratorTmpl()
            : ptr_(nullptr)
        {}

        operator IteratorTmpl<const RefT>() const { return IteratorTmpl<const RefT>(*this); }

        IteratorTmpl& operator ++() { ptr_ = ptr_->next; return *this; }
        IteratorTmpl operator ++(int) { IteratorTmpl saved = *this; ++*this; return saved; }
        bool operator ==(IteratorTmpl that) const { return ptr_ == that.ptr_; }
        bool operator !=(IteratorTmpl that) const { return ptr_ != that.ptr_; }
        reference_type operator *() const { return ptr_->data; }
        pointer_type operator ->() const { return &ptr_->data; }
    };

    Node *first_;
    size_t size_;

    Node* pre_ith_(size_t idx) const
    {
        if (!idx) {
            return nullptr;
        }
        Node* ptr = first_;
        for (size_t i = 1; i < idx; ++i) {
            ptr = ptr->next;
        }
        return ptr;
    }

    Node* insert_after_(Node *ptr, const T &value)
    {
        Node *result;
        if (ptr) {
            ptr->next = new Node(value, ptr->next);
            result = ptr->next;
        } else {
            first_ = new Node(value, first_);
            result = first_;
        }
        ++size_;
        return result;
    }

    Node* erase_after_(Node *ptr)
    {
        Node *result;
        if (ptr) {
            Node *tmp = ptr->next->next;
            delete ptr->next;
            ptr->next = tmp;

            result = ptr->next;
        } else {
            Node *tmp = first_->next;
            delete first_;
            first_ = tmp;

            result = first_;
        }
        --size_;
        return result;
    }

    Node* erase_range_(Node *from, Node *to)
    {
        if (from) {
            if (from == to) {
                return to; // ?
            }
            while (from->next != to) {
                erase_after_(from);
            }
        } else {
            while (first_ != to) {
                erase_after_(nullptr);
            }
        }
        return to;
    }

public:
    typedef IteratorTmpl<T> iterator;
    typedef IteratorTmpl<const T> const_iterator;
    typedef T value_type;

    ~ForwardList() { erase_range_(nullptr, nullptr); }

    void clear() { erase_range_(nullptr, nullptr); }

    void push_front(const T &value) { insert_after_(nullptr, value); }

    void pop_front() { erase_after_(nullptr); }

    ForwardList(size_t n, const T &value)
        : first_(nullptr)
        , size_(0)
    {
        for (size_t i = 0; i < n; ++i) {
            try {
                push_front(value);
            } catch (...) {
                this->~ForwardList();
                throw;
            }
        }
    }

    ForwardList(size_t n)
        : ForwardList(n, T())
    {}

    ForwardList()
        : ForwardList(0)
    {}

    ForwardList(const ForwardList &that)
        : first_(nullptr)
        , size_(0)
    {
        for (const T &value : that) {
            try {
                push_front(value);
            } catch (...) {
                this->~ForwardList();
                throw;
            }
        }
        try {
            reverse();
        } catch (...) {
            this->~ForwardList();
            throw;
        }
    }

    ForwardList& operator =(const ForwardList &that) & {
        if (this != &that) {
            clear();
            for (const T &value : that) {
                push_front(value);
            }
            reverse();
        }
        return *this;
    }

    void swap(ForwardList &that) {
        bicycle::swap(first_, that.first_);
        bicycle::swap(size_, that.size_);
    }

    size_t size() const { return size_; }

    void resize(size_t n)
    {
        if (size_ < n) {
            Node *last = pre_ith_(size_);
            while (size_ < n) {
                last = insert_after_(last, T());
            }
        } else {
            erase_range_(pre_ith_(n), nullptr);
        }
    }

    void reverse()
    {
        Node *cur = first_;
        Node *next = nullptr;
        Node *prev = nullptr;
        while (cur) {
            next = cur->next;
            cur->next = prev;
            prev = cur;
            cur = next;
        }
        first_ = prev;
    }

    iterator begin() { return iterator(first_); }
    iterator end() { return iterator(nullptr); }
    const_iterator begin() const { return const_iterator(first_); }
    const_iterator end() const { return const_iterator(nullptr); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    iterator erase_after(const_iterator position)
    {
        return iterator(erase_after_(position.ptr_));
    }

    iterator erase_after(const_iterator from, const_iterator to)
    {
        return iterator(erase_range_(from.ptr_, to.ptr_));
    }

    iterator insert_after(const_iterator position, const T &value)
    {
        return iterator(insert_after_(position.ptr_, value));
    }

    template<class InputIterator>
    iterator insert_after(const_iterator position, InputIterator from, InputIterator to)
    {
        Node *ptr = position.ptr_;
        for (InputIterator it = from; it != to; ++it) {
            ptr = insert_after_(ptr, *it);
        }
        return iterator(ptr);
    }

    template<class Compare>
    void sort(Compare cmp)
    {
        size_t size = size_;
        T *buf = new T[2 * size]();
        try {
            {
                size_t i = 0;
                for (const T &value : *this) {
                    buf[i++] = value;
                }
            }
            bicycle::mergeSort(buf, buf + size, buf + size, cmp);
            clear();
            for (size_t i = 0; i < size; ++i) {
                insert_after_(nullptr, buf[i]);
            }
            reverse();
        } catch (...) {
            delete[] buf;
            throw;
        }
        delete[] buf;
    }

    void sort()
    {
        sort(std::less<T>());
    }
};
