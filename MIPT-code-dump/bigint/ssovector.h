#include <iterator>
#include <memory>
#include <stdio.h>

// Needed this for optimization.
template<typename T, size_t SmallThreshold>
class SSOVector
{
    size_t size_;
    size_t capacity_;
    union {
        T small_[SmallThreshold];
        T *large_;
    };

    bool isLarge_() const { return capacity_ > SmallThreshold; }

    void growCapacity_(size_t n)
    {
        T *newLarge = new T[n];
        //Since C++17:
        //std::uninitialized_move(data(), data() + size_, newLarge);
        {
            T *ptr = data();
            for (size_t i = 0; i < size_; ++i) {
                new (newLarge + i) T(ptr[i]);
                ptr[i].~T();
            }
        }
        if (isLarge_()) {
            ::operator delete[](reinterpret_cast<void*>(large_));
        }
        large_ = newLarge;
        capacity_ = n;
    }

    void growIfFull_()
    {
        if (size_ == capacity_) {
            growCapacity_(2 * capacity_);
        }
    }

public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

          T* data()       { return isLarge_() ? large_ : small_; }
    const T* data() const { return isLarge_() ? large_ : small_; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void swap(SSOVector &that)
    {
        if (isLarge_()) {
            if (that.isLarge_()) {
                std::swap(size_, that.size_);
                std::swap(capacity_, that.capacity_);
                std::swap(large_, that.large_);
            } else {
                SSOVector tmp = that;
                that = *this;
                *this = tmp;
            }
        } else {
            SSOVector tmp = *this;
            *this = that;
            that = tmp;
        }
    }

    void reserve(size_t n)
    {
        if (capacity_ < n) {
            growCapacity_(n);
        }
    }

    void resize(size_t n)
    {
        if (size_ < n) {
            reserve(n);
            std::uninitialized_fill(data() + size_, data() + n, T());
        } else {
            T *ptr = data();
            for (size_t i = n; i < size_; ++i) {
                ptr[i].~T();
            }
        }
        size_ = n;
    }

    void push_back(const T &value)
    {
        growIfFull_();
        new (data() + size_) T(value);
        ++size_;
    }

    const T& back() const { return (*this)[size_ - 1]; }
          T& back()       { return (*this)[size_ - 1]; }

    void pop_back()
    {
        data()[size_ - 1].~T();
        --size_;
    }

    void clear()
    {
        T *ptr = data();
        for (size_t i = 0; i < size_; ++i) {
            ptr[i].~T();
        }
        size_ = 0;
    }

    SSOVector()
        : size_(0)
        , capacity_(SmallThreshold)
    {}

    SSOVector(std::initializer_list<T> l)
        : size_(0)
        , capacity_(SmallThreshold)
    {
        reserve(l.size());
        std::uninitialized_copy(l.begin(), l.end(), data());
        size_ = l.size();
    }

    SSOVector(const SSOVector &that)
        : size_(0)
        , capacity_(SmallThreshold)
    {
        reserve(that.size());
        std::uninitialized_copy(that.begin(), that.end(), data());
        size_ = that.size();
    }

    SSOVector(SSOVector &&that)
        : size_(0)
        , capacity_(SmallThreshold)
    {
        if (that.isLarge_()) {
            size_ = that.size_;
            capacity_ = that.capacity_;
            large_ = that.large_;
            that.size_ = 0;
            that.capacity_ = SmallThreshold;
        } else {
            assign(that);
        }
    }

    SSOVector& operator =(const SSOVector &that)
    {
        assign(that);
        return *this;
    }

    SSOVector& operator =(SSOVector &&that)
    {
        if (this != &that) {
            if (that.isLarge_()) {
                clear();
                size_ = that.size_;
                capacity_ = that.capacity_;
                large_ = that.large_;
                that.size_ = 0;
                that.capacity_ = SmallThreshold;
            } else {
                assign(that);
            }
        }
        return *this;
    }

    void assign(const SSOVector &that)
    {
        if (this != &that) {
            clear();
            reserve(that.size());
            std::uninitialized_copy(that.begin(), that.end(), data());
            size_ = that.size();
        }
    }

    void assign(size_t n, const T &value)
    {
        clear();
        reserve(n);
        std::uninitialized_fill(data(), data() + n, value);
        size_ = n;
    }

    iterator insert(iterator pos, const T &value)
    {
        size_t index = pos - data();
        growIfFull_();
        T *ptr = data();
        for (size_t i = size_; i != index; --i) {
            if (i == size_) {
                new (ptr + i) T(ptr[i - 1]);
            } else {
                ptr[i] = ptr[i - 1];
            }
        }
        ptr[index] = value;
        ++size_;
        return ptr + index;
    }

          T& operator [](size_t index)       { return data()[index]; }
    const T& operator [](size_t index) const { return data()[index]; }

          iterator begin()       { return data(); }
    const_iterator begin() const { return data(); }

          iterator end()       { return data() + size_; }
    const_iterator end() const { return data() + size_; }

          reverse_iterator rbegin()       { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

          reverse_iterator rend()       { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    ~SSOVector()
    {
        clear();
        if (isLarge_()) {
            ::operator delete[](reinterpret_cast<void*>(large_));
        }
    }
};
