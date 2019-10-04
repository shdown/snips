#include <new>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <memory>

namespace tree {

typedef long long ValueType;

namespace details {

struct Node
{
    size_t cnt;
    size_t prior;

    size_t up_pref;
    size_t up_suff;
    size_t down_pref;
    size_t down_suff;

    ValueType rightmost;
    ValueType leftmost;
    ValueType value;
    ValueType sum;
    ValueType to_add;
    bool rev;
    bool assign;

    Node *left, *right;

    explicit Node(ValueType value)
        : cnt(1)
        , prior(std::rand())
        , up_pref(1)
        , up_suff(1)
        , down_pref(1)
        , down_suff(1)
        , rightmost(value)
        , leftmost(value)
        , value(value)
        , sum(value)
        , to_add(0)
        , rev(false)
        , assign(false)
        , left(nullptr)
        , right(nullptr)
    {}
};

inline void tree_assign(Node *t, ValueType value)
{
    if (t) {
        t->value = value;
        t->to_add = 0;
        t->assign = true;
    }
}

inline size_t subtree_size(Node *t)
{
    return t ? t->cnt : 0;
}

void push(Node *t)
{
    if (!t) {
        return;
    }
    if (t->assign) {
        const ValueType value = t->value;
        t->assign = false;
        t->up_pref = t->down_pref = t->up_suff = t->down_suff = t->cnt;
        t->leftmost = t->rightmost = value;
        t->sum = value * t->cnt;
        tree_assign(t->left, value);
        tree_assign(t->right, value);
    }
    const ValueType to_add = t->to_add;
    if (to_add) {
        t->value += to_add;
        t->leftmost += to_add;
        t->rightmost += to_add;
        t->sum += to_add * t->cnt;
        if (t->left) {
            t->left->to_add += to_add;
        }
        if (t->right) {
            t->right->to_add += to_add;
        }
        t->to_add = 0;
    }
    if (t->rev) {
        t->rev = false;
        std::swap(t->left, t->right);
        std::swap(t->up_pref, t->down_suff);
        std::swap(t->down_pref, t->up_suff);
        std::swap(t->leftmost, t->rightmost);
        if (t->left) {
            t->left->rev ^= true;
        }
        if (t->right) {
            t->right->rev ^= true;
        }
    }
}

struct LeftTag {};
struct RightTag {};

inline ValueType get_outermost(Node *t, LeftTag) { return t->leftmost; }
inline ValueType get_outermost(Node *t, RightTag) { return t->rightmost; }

inline Node * get_child_ptr(Node *t, LeftTag) { return t->left; }
inline Node * get_child_ptr(Node *t, RightTag) { return t->right; }

inline LeftTag reverse_dir(RightTag) { return LeftTag(); }
inline RightTag reverse_dir(LeftTag) { return RightTag(); }

template<class DirTag, class Accessor, class Compare>
inline void update_sorted_end(Node *t, DirTag tag, Accessor acc, Compare cmp)
{
    Node *d = get_child_ptr(t, tag);
    Node *r = get_child_ptr(t, reverse_dir(tag));
    if (d) {
        if ((acc(t) = acc(d)) == d->cnt &&
            cmp(get_outermost(d, reverse_dir(tag)), t->value))
        {
            ++acc(t);
            if (r && cmp(t->value, get_outermost(r, tag))) {
                acc(t) += acc(r);
            }
        }
    } else {
        acc(t) = 1;
        if (r && cmp(t->value, get_outermost(r, tag))) {
            acc(t) += acc(r);
        }
    }
}

void update(Node *t)
{
    if (!t) {
        return;
    }
    push(t);
    t->cnt = 1;
    t->sum = t->rightmost = t->leftmost = t->value;
    if (t->left) {
        push(t->left);
        t->cnt += t->left->cnt;
        t->sum += t->left->sum;
        t->leftmost = t->left->leftmost;
    }
    if (t->right) {
        push(t->right);
        t->cnt += t->right->cnt;
        t->sum += t->right->sum;
        t->rightmost = t->right->rightmost;
    }

    auto le = [](ValueType a, ValueType b){ return a <= b; };
    auto ge = [](ValueType a, ValueType b){ return a >= b; };

    update_sorted_end(t, LeftTag(), [](Node *t) -> size_t& { return t->up_pref; }, le);
    update_sorted_end(t, LeftTag(), [](Node *t) -> size_t& { return t->down_pref; }, ge);

    update_sorted_end(t, RightTag(), [](Node *t) -> size_t& { return t->up_suff; }, ge);
    update_sorted_end(t, RightTag(), [](Node *t) -> size_t& { return t->down_suff; }, le);
}

inline Node * merge(Node *left, Node *right)
{
    if (!left || !right) {
        return left ? left : right;
    }
    Node *ans;
    push(left);
    push(right);
    if (left->prior > right->prior) {
        left->right = merge(left->right, right);
        ans = left;
    } else {
        right->left = merge(left, right->left);
        ans = right;
    }
    update(ans);
    return ans;
}

inline std::pair<Node *, Node *> split(Node *t, size_t key)
{
    if (!t) {
        return {nullptr, nullptr};
    }
    std::pair<Node *, Node *> ans;
    push(t);
    size_t curKey = 1 + subtree_size(t->left);
    if (key < curKey) {
        std::pair<Node *, Node *> prime = split(t->left, key);
        t->left = prime.second;
        ans = {prime.first, t};
    } else {
        std::pair<Node *, Node *> prime = split(t->right, key - curKey);
        t->right = prime.first;
        ans = {t, prime.second};
    }
    update(t);
    return ans;
}

inline void node_assign(Node *t, ValueType value)
{
    const ValueType delta = value - t->value;
    t->to_add += delta;
    if (t->left) {
        t->left->to_add -= delta;
    }
    if (t->right) {
        t->right->to_add -= delta;
    }
}

template<class Predicate>
inline std::pair<ValueType, Node *> assign_last_of(
    Node *t, ValueType value, Predicate pred, Node *res = nullptr)
{
    push(t);
    std::pair<ValueType, Node *> prime = {0, res};
    if (pred(t->value)) {
        prime.second = t;
        if (t->right) {
            prime = assign_last_of(t->right, value, pred, prime.second);
        }
    } else {
        if (t->left) {
            prime = assign_last_of(t->left, value, pred, prime.second);
        }
    }
    if (prime.second == t) {
        prime.first = t->value;
        node_assign(t, value);
    }
    update(t);
    return prime;
}

template<class Handler>
inline void traverse(Node *t, Handler handler)
{
    if (!t) {
        return;
    }
    push(t);
    traverse(t->left, handler);
    handler((ValueType) t->value);
    traverse(t->right, handler);
}

} // namespace details

template<class Allocator = std::allocator<details::Node>>
class Tree
{
    details::Node *ptr_;
    Allocator alloc_;

    Tree(details::Node *ptr, const Allocator &alloc = Allocator())
        : ptr_(ptr)
        , alloc_(alloc)
    {}

    template<class ...Args>
    details::Node * construct_node_(Args&& ...args)
    {
        details::Node *res = alloc_.allocate(1);
        alloc_.construct(res, std::forward<Args>(args)...);
        return res;
    }

public:
    Tree() : ptr_(nullptr) {}

    explicit Tree(ValueType value, const Allocator &alloc = Allocator())
        : ptr_(construct_node_(value))
        , alloc_(alloc)
    {}

    Tree(const Tree &that) = delete;
    Tree(Tree &&that)
        : ptr_(that.ptr_)
        , alloc_(std::move(that.alloc_))
    {
        that.ptr_ = nullptr;
    }

    Tree& operator =(const Tree &that) = delete;
    Tree& operator =(Tree &&that)
    {
        if (&that != this) {
            ptr_ = that.ptr_;
            alloc_ = std::move(that.alloc_);
            that.ptr_ = nullptr;
        }
        return *this;
    }

    size_t size() const { return details::subtree_size(ptr_); }
    bool empty() const { return !ptr_; }
    ValueType sum() const { details::push(ptr_); return ptr_ ? ptr_->sum : 0; }

    size_t down_suff() const { details::push(ptr_); return ptr_ ? ptr_->down_suff : 0; }
    size_t up_suff() const { details::push(ptr_); return ptr_ ? ptr_->up_suff : 0; }
    ValueType rightmost() const { details::push(ptr_); return ptr_->rightmost; }
    ValueType leftmost() const { details::push(ptr_); return ptr_->leftmost; }

    void assign(ValueType value) { details::tree_assign(ptr_, value); }
    void add(ValueType value) { if (ptr_) { ptr_->to_add += value; } }
    void reverse() { if (ptr_) { ptr_->rev ^= true; } }

    void merge(Tree &&that)
    {
        ptr_ = details::merge(ptr_, that.ptr_);
        that.ptr_ = nullptr;
    }

    Tree split(size_t key)
    {
        const auto res = details::split(ptr_, key);
        ptr_ = res.second;
        return Tree(res.first);
    }

    template<class Predicate>
    std::pair<bool, ValueType> assign_last_of(ValueType value, Predicate p)
    {
        const auto res = details::assign_last_of(ptr_, value, p);
        return {res.second, res.first};
    }

    // "I'm too lazy to write an iterator"
    template<class Handler>
    void traverse(Handler handler) const
    {
        details::traverse(ptr_, handler);
    }

    void push_back(ValueType x)
    {
        merge(Tree(x));
    }

    ~Tree() {
        if (ptr_) {
            alloc_.destroy(ptr_);
            alloc_.deallocate(ptr_, 1);
        }
    }
};

} // namespace tree

class MyDataStructure
{
public:
    typedef tree::ValueType ValueType;

private:
    typedef tree::Tree<> Tree_;

    mutable Tree_ t_;

    struct NextPermTag_ {};
    struct PrevPermTag_ {};

    static size_t get_suffix_(Tree_ &t, NextPermTag_) { return t.down_suff(); }
    static size_t get_suffix_(Tree_ &t, PrevPermTag_) { return t.up_suff(); }

    static bool compare_boundary_(ValueType boundary, ValueType x, NextPermTag_)
    {
        return x > boundary;
    }

    static bool compare_boundary_(ValueType boundary, ValueType x, PrevPermTag_)
    {
        return x < boundary;
    }

    template<class Tag>
    static void next_or_prev_perm_(Tree_ &t, Tag)
    {
        const size_t k = t.size() - MyDataStructure::get_suffix_(t, Tag());
        if (k) {
            Tree_ left = t.split(k);
            const ValueType boundary = left.rightmost();
            const ValueType oldval = t.assign_last_of(
                boundary, [boundary](ValueType x){
                    return MyDataStructure::compare_boundary_(boundary, x, Tag());
                }).second;
            // assign to rightmost
            (void) left.assign_last_of(oldval, [](ValueType){ return true; });
            t.reverse();
            left.merge(std::move(t));
            t = std::move(left);
        } else {
            t.reverse();
        }
    }

    template<class Handler>
    void visit_segment_(size_t l, size_t r, Handler handler) const
    {
        Tree_ left = t_.split(l);
        Tree_ middle = t_.split(r - l + 1);
        handler(middle);
        middle.merge(std::move(t_));
        left.merge(std::move(middle));
        t_ = std::move(left);
    }

public:

    MyDataStructure() : t_() {};

    void push_back(ValueType value)
    {
        t_.merge(Tree_(value));
    }

    ValueType sum(size_t l, size_t r) const
    {
        ValueType result;
        visit_segment_(l, r, [&result](Tree_ &middle){ result = middle.sum(); });
        return result;
    }

    void insertAt(size_t pos, ValueType value)
    {
        Tree_ left = t_.split(pos);
        Tree_ middle(value);
        middle.merge(std::move(t_));
        left.merge(std::move(middle));
        t_ = std::move(left);
    }

    void eraseAt(size_t pos)
    {
        Tree_ left = t_.split(pos);
        (void) t_.split(1);
        left.merge(std::move(t_));
        t_ = std::move(left);
    }

    void assign(size_t l, size_t r, ValueType value)
    {
        visit_segment_(l, r, [value](Tree_ &middle){ middle.assign(value); });
    }

    void add(size_t l, size_t r, ValueType value)
    {
        visit_segment_(l, r, [value](Tree_ &middle){ middle.add(value); });
    }

    void next_perm(size_t l, size_t r)
    {
        visit_segment_(l, r, [](Tree_ &middle){
            MyDataStructure::next_or_prev_perm_(middle, NextPermTag_());
        });
    }

    void prev_perm(size_t l, size_t r)
    {
        visit_segment_(l, r, [](Tree_ &middle){
            MyDataStructure::next_or_prev_perm_(middle, PrevPermTag_());
        });
    }

    template<class Handler>
    void traverse(Handler handler) const
    {
        t_.traverse(handler);
    }
};

int
main()
{
    int n;
    std::scanf("%d", &n);
    MyDataStructure ds;
    for (int i = 0; i < n; ++i) {
        int x;
        std::scanf("%d", &x);
        ds.push_back(x);
    }
    int q;
    std::scanf("%d", &q);
    for (int i = 0; i < q; ++i) {
        int cmd;
        std::scanf("%d", &cmd);
        switch (cmd) {
        case 1: {
                int l, r;
                std::scanf("%d%d", &l, &r);
                std::printf("%lld\n", ds.sum(l, r));
            }
            break;
        case 2: {
                int x, pos;
                std::scanf("%d%d", &x, &pos);
                ds.insertAt(pos, x);
            }
            break;
        case 3: {
                int pos;
                std::scanf("%d", &pos);
                ds.eraseAt(pos);
            }
            break;
        case 4: {
                int x, l, r;
                std::scanf("%d%d%d", &x, &l, &r);
                ds.assign(l, r, x);
            }
            break;
        case 5: {
                int x, l, r;
                std::scanf("%d%d%d", &x, &l, &r);
                ds.add(l, r, x);
            }
            break;
        case 6: {
                int l, r;
                std::scanf("%d%d", &l, &r);
                ds.next_perm(l, r);
            }
            break;
        case 7: {
                int l, r;
                std::scanf("%d%d", &l, &r);
                ds.prev_perm(l, r);
            }
            break;
        }
    }

    ds.traverse([](long long x){ std::printf("%lld ", x); });
    std::printf("\n");
}
