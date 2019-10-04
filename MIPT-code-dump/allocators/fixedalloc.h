#include <stddef.h>
#include <algorithm>
#include <memory>

namespace details {

template<class T>
class Vector {
    T *data_;
    size_t size_, capacity_;

    void destroy_data_()
    {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
    }

    void ensure_(size_t req)
    {
        if (!capacity_) {
            capacity_ = 1;
        }
        while (capacity_ < req) {
            capacity_ *= 2;
        }
        T *new_ = new T[capacity_];

        for (size_t i = 0; i < size_; ++i) {
            new (static_cast<void *>(new_ + i)) T(std::move(data_[i]));
        }
        destroy_data_();
        delete[] data_;
        data_ = new_;
    }

public:
    Vector()
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
    {}

    Vector(const Vector &that)
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
    {
        *this = that;
    }

    Vector(Vector &&that)
        : data_(that.that_)
        , size_(that.size_)
        , capacity_(that.capacity_)
    {
        that.data_ = nullptr;
        that.size_ = nullptr;
        that.capacity_ = nullptr;
    }

    Vector& operator =(const Vector &that)
    {
        if (this != &that) {
            ensure_(that.size_);
            destroy_data_();
            std::uninitialized_copy(that.data_, that.data_ + that.size_, data_);
            size_ = that.size_;
        }
        return *this;
    }

    Vector& operator =(Vector &&that)
    {
        std::swap(*this, that);
        return *this;
    }

          T& back()       { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }

          T& operator [](size_t index)       { return data_[index]; }
    const T& operator [](size_t index) const { return data_[index]; }

    void pop_back()
    {
        data_[size_ - 1].~T();
        --size_;
    }

    void push_back(const T &value)
    {
        ensure_(size_ + 1);
        new (static_cast<void *>(data_ + size_)) T(value);
        ++size_;
    }

    size_t size() const { return size_; }

    bool empty() const { return !size_; }

    ~Vector()
    {
        destroy_data_();
        delete[] data_;
    }
};

}

template<size_t chunk_size>
class FixedAllocator
{
    details::Vector<char *> alloced_;
#ifdef UNUSED
    details::Vector<void *> unused_;
#endif
    char *cur_, *end_;
    size_t n_;

public:
    FixedAllocator()
        : alloced_()
#ifdef UNUSED
        , unused_()
#endif
        , cur_(new char[chunk_size])
        , end_(cur_ + chunk_size)
        , n_(chunk_size)
    {
        alloced_.push_back(cur_);
    }

    FixedAllocator(const FixedAllocator &)
        : FixedAllocator()
    {}

    FixedAllocator(FixedAllocator &&that)
        : alloced_(std::move(that.alloced_))
#ifdef UNUSED
        , unused_(std::move(that.unused_))
#endif
        , cur_(that.cur_)
        , end_(that.end_)
        , n_(that.n_)
    {}

    FixedAllocator& operator =(const FixedAllocator &)
    {
        return *this;
    }

    FixedAllocator& operator =(FixedAllocator &&that)
    {
        std::swap(*this, that);
        return *this;
    }

    void * allocate()
    {
#ifdef UNUSED
        if (!unused_.empty()) {
            void *r = unused_.back();
            unused_.pop_back();
            return r;
        }
#endif
        if (cur_ == end_) {
            cur_ = new char[n_ *= 2];
            alloced_.push_back(cur_);
            end_ = cur_ + n_;
        }
        void *r = static_cast<void *>(cur_);
        cur_ += chunk_size;
        return r;
    }

    void deallocate(void *
#ifdef UNUSED
                   ptr
#endif
                   )

    {
#ifdef UNUSED
        unused_.push_back(ptr);
#endif
    }

    ~FixedAllocator()
    {
        for (size_t i = 0; i < alloced_.size(); ++i) {
            delete[] alloced_[i];
        }
    }
};

#define XPAND() X(1) X(4) X(8)

template<class T>
class FastAllocator
{
#define X(N) static FixedAllocator<N * sizeof(T)> fa ## N ## _;
    XPAND()
#undef X

public:
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef void *void_pointer;
    typedef const void *const_void_pointer;
    typedef T value_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef size_t size_type;
    typedef ssize_t difference_type;

    template<class U>
    struct rebind
    {
        typedef FastAllocator<U> other;
    };

    FastAllocator() = default;
    template<class U> FastAllocator(const FastAllocator<U> &) {}
    FastAllocator& operator =(const FastAllocator &) = default;
    ~FastAllocator() = default;

    T * allocate(size_t n)
    {
        switch (n) {
#define X(N) case N: return static_cast<T *>(fa ## N ## _.allocate());
    XPAND()
#undef X
        default:
            return static_cast<T *>(::operator new[](n * sizeof(T)));
        }
    }

    void deallocate(T *p, size_t n)
    {
        switch (n) {
#define X(N) case N: fa ## N ## _.deallocate(p); break;
    XPAND()
#undef X
        default:
            ::operator delete[](p);
            break;
        }
    }

    template<class U, class ...Args>
    void construct(U *p, Args &&... args)
    {
        new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }

    template<class U>
    void destroy(U *p)
    {
        p->~U();
    }

    bool operator ==(const FastAllocator &that) const
    {
        return this == &that;
    }

    bool operator !=(const FastAllocator &that) const
    {
        return !(*this == that);
    }
};

#define X(N) template<class T> FixedAllocator<N * sizeof(T)> FastAllocator<T>::fa ## N ## _;
    XPAND()
#undef X

#undef XPAND
