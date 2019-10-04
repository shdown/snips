#include <algorithm>
#include <cassert>

namespace details {

template<class T>
class Reference
{
    struct Counters
    {
        long nref;
        long nweakref;
    };

    T *ptr_;
    Counters *cnt_;

public:
    Reference()
        : ptr_(nullptr)
        , cnt_(nullptr)
    {}

    Reference(T *ptr)
        : ptr_(ptr)
        , cnt_(new Counters{1L, 0L})
    {}

    void inc_nref()
    {
        if (cnt_) {
            ++cnt_->nref;
        }
    }

    void inc_nweakref()
    {
        if(cnt_) {
            ++cnt_->nweakref;
        }
    }

    void dec_nref()
    {
        if (cnt_ && !--cnt_->nref) {
            delete ptr_;
            if (!cnt_->nweakref) {
                delete cnt_;
            }
        }
    }

    void dec_nweakref()
    {
        if (cnt_ && !--cnt_->nweakref && !cnt_->nref) {
            delete cnt_;
        }
    }

    long nref() const { return cnt_ ? cnt_->nref : 0L; }

    T* pointer() const { return ptr_; }
};

} // namespace details

template<class T> class WeakPtr;

template<class T>
class SharedPtr
{
    friend class WeakPtr<T>;

    details::Reference<T> ref_;

public:
    SharedPtr() : ref_() {}

    SharedPtr(T *ptr) : ref_(ptr) {}

    SharedPtr(const WeakPtr<T> &ptr);

    SharedPtr(const SharedPtr &that)
        : ref_(that.ref_)
    {
        ref_.inc_nref();
    }

    SharedPtr(SharedPtr &&that)
        : ref_(that.ref_)
    {
        that.ref_ = details::Reference<T>();
    }

    SharedPtr& operator =(const SharedPtr &that)
    {
        if (&that != this) {
            ref_.dec_nref();
            ref_ = that.ref_;
            ref_.inc_nref();
        }
        return *this;
    }

    SharedPtr& operator =(SharedPtr &&that)
    {
        assert(this != &that);
        ref_.dec_nref();
        ref_ = that.ref_;
        that.ref_ = details::Reference<T>();
        return *this;
    }

    T& operator *() const { return *ref_.pointer(); }

    T* operator ->() const { return ref_.pointer(); }

    T* get() const { return ref_.pointer(); }

    long use_count() const { return ref_.nref(); }

    void reset() { ref_.dec_nref(); ref_ = details::Reference<T>(); }

    void reset(T *ptr) { ref_.dec_nref(); ref_ = details::Reference<T>(ptr); }

    void swap(SharedPtr &that) { std::swap(ref_, that.ref_); }

    ~SharedPtr() { ref_.dec_nref(); }
};

template<class T>
class WeakPtr
{
    friend class SharedPtr<T>;

    details::Reference<T> ref_;

public:
    WeakPtr() : ref_() {}

    WeakPtr(const SharedPtr<T> &obj)
        : ref_(obj.ref_)
    {
        ref_.inc_nweakref();
    }

    WeakPtr(const WeakPtr &that)
        : ref_(that.ref_)
    {
        ref_.inc_nweakref();
    }

    WeakPtr(WeakPtr &&that)
        : ref_(that.ref_)
    {
        that.ref_ = details::Reference<T>();
    }

    WeakPtr& operator =(const WeakPtr &that)
    {
        if (&that != this) {
            ref_.dec_nweakref();
            ref_ = that.ref_;
            ref_.inc_nweakref();
        }
        return *this;
    }

    WeakPtr& operator =(WeakPtr &&that)
    {
        assert(this != &that);
        ref_.dec_nweakref();
        ref_ = that.ref_;
        that.ref_ = details::Reference<T>();
        return *this;
    }

    long use_count() const { return ref_.nref(); }

    bool expired() const { return use_count() == 0L; }

    void reset() { ref_.dec_nweakref(); ref_ = details::Reference<T>(); }

    void swap(WeakPtr &that) { std::swap(ref_, that.ref_); }

    SharedPtr<T> lock() const { return SharedPtr<T>(*this); }

    ~WeakPtr() { ref_.dec_nweakref(); }
};

template<class T>
inline SharedPtr<T>::SharedPtr(const WeakPtr<T> &ptr)
    : ref_(ptr.ref_)
{
    if (ptr.expired()) {
        ref_ = details::Reference<T>();
    } else {
        ref_.inc_nref();
    }
}

template<class T>
class UniquePtr
{
    T *ptr_;

public:
    UniquePtr(T *ptr = nullptr) : ptr_(ptr) {}
    UniquePtr(const UniquePtr &) = delete;
    UniquePtr(UniquePtr &&that) : ptr_(that.ptr_) { that.ptr_ = nullptr; }

    UniquePtr& operator =(const UniquePtr &) = delete;
    UniquePtr& operator =(UniquePtr &&that)
    {
        delete ptr_;
        ptr_ = that.ptr_;
        that.ptr_ = nullptr;
        return *this;
    }

    T& operator *() const { return *ptr_; }
    T* operator ->() const { return ptr_; }
    T* get() const { return ptr_; }

    T* release() { T *saved = ptr_; ptr_ = nullptr; return saved; }
    void reset(T *ptr = nullptr) { delete ptr_; ptr_ = ptr; }
    void swap(UniquePtr &that) { std::swap(ptr_, that.ptr_); }

    ~UniquePtr() { delete ptr_; }
};
