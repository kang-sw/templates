#pragma once
#include <array>
#include <cassert>

namespace kangsw {
/**
 * 스레드에 매우 안전하지 않은 클래스입니다.
 * 별도의 스레드와 사용 시 반드시 락 필요
 */
template <typename Ty_, size_t Cap_>
class circular_queue {
    using chunk_t = std::array<int8_t, sizeof(Ty_)>;

public:
    using value_type = Ty_;

public:
    template <bool Constant_ = true>
    class iterator {
    public:
        using owner_type = std::conditional_t<Constant_, circular_queue const, circular_queue>;
        using value_type = Ty_;
        using value_type_actual = std::conditional_t<Constant_, Ty_ const, Ty_>;
        using pointer = value_type_actual const*;
        using reference = value_type_actual&;
        using difference_type = ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

    public:
        iterator() noexcept = default;
        iterator(const iterator&) noexcept = default;
        iterator& operator=(const iterator&) noexcept = default;

        auto& operator*() const { return _owner->_at(_head);  }
        auto operator->() const { return &*(*this); }

        bool operator==(iterator const& op) const noexcept { return _head == op._head; }
        bool operator<(iterator const& op) const noexcept { return _idx() < op._idx(); }
        bool operator<=(iterator const& op) const noexcept { return !(op < *this); }

        auto& operator++() noexcept { return _head = _next(_head), *this; }
        auto& operator--() noexcept { return _head = _prev(_head), *this; }

        auto operator++(int) noexcept {
            auto c = *this;
            return ++*this, c;
        }
        auto operator--(int) noexcept {
            auto c = *this;
            return --*this, c;
        }

        auto& operator-=(ptrdiff_t i) noexcept { return _head = _jmp(_head, -i), *this; }
        auto& operator+=(ptrdiff_t i) noexcept { return _head = _jmp(_head, i), *this; }

        auto operator-(iterator const& op) const noexcept { return static_cast<ptrdiff_t>(_idx() - op._idx()); }

        friend auto operator+(iterator it, ptrdiff_t i) { return it += i; }
        friend auto operator+(ptrdiff_t i, iterator it) { return it += i; }
        friend auto operator-(iterator it, ptrdiff_t i) { return it -= i; }
        friend auto operator-(ptrdiff_t i, iterator it) { return it -= i; }

    private:
        size_t _idx() const noexcept { return _owner->_idx_linear(_head); }

    private:
        iterator(owner_type* o, size_t h) :
            _owner(o), _head(h) {}
        friend class circular_queue;

        owner_type* _owner;
        size_t _head;
    };

public:
    void push(Ty_ const& s) { new (_data[_reserve()].data()) Ty_(s); }
    void push(Ty_&& s) { new (_data[_reserve()].data()) Ty_(std::move(s)); }
    void pop() { _release(); }
    void push_back(Ty_ const& s) { this->push(s); }
    void push_back(Ty_&& s) { this->push(std::move(s)); }

    void push_rotate(Ty_ const& s) {
        if (is_full()) { pop(); }
        this->push(s);
    }

    void push_rotate(Ty_ && s) {
        if (is_full()) { pop(); }
        this->push(std::move(s));
    }

    size_t size() const {
        return _head >= _tail ? _head - _tail : _head + (Cap_ + 1) - _tail;
    }
    
    auto cbegin() const noexcept { return iterator<true>(this, _tail); }
    auto cend() const noexcept { return iterator<true>(this, _head); }
    auto begin() const noexcept { return cbegin(); }
    auto end() const noexcept { return cend(); }
    auto begin() noexcept { return iterator<false>(this, _tail); }
    auto end() noexcept { return iterator<false>(this, _head); }

    constexpr size_t capacity() const { return Cap_; }
    bool empty() const { return _head == _tail; }

    Ty_& peek() { return _front(); }
    Ty_ const& peek() const { return _front(); }

    Ty_& latest() { return _back(); }

    bool is_full() const { return _next(_head) == _tail; }

    template <class Fn_>
    void for_each(Fn_&& fn) {
        for (size_t it = _tail; it != _head; it = _next(it)) { fn(_at(it)); }
    }

    template <class Fn_>
    void for_each(Fn_&& fn) const {
        for (size_t it = _tail; it != _head; it = _next(it)) { fn(static_cast<const Ty_&>(_at(it))); }
    }

    void clear() {
        while (!empty()) { pop(); }
    }

    ~circular_queue() { clear(); }

private:
    constexpr static size_t _cap() { return Cap_ + 1; }

    size_t _reserve() {
        if (is_full()) throw std::bad_array_new_length();
        auto retval = _head;
        _head = _next(_head);
        return retval;
    }

    constexpr static size_t _next(size_t current) {
        return ++current == _cap() ? 0 : current;
    }

    constexpr static size_t _prev(size_t current) {
        return --current == ~size_t{} ? _cap() - 1 : current;
    }

    constexpr static size_t _jmp(size_t at, ptrdiff_t jmp) {
        if (jmp >= 0) { return (at + jmp) % _cap(); }
        return at += jmp, at + _cap() * ((ptrdiff_t)at < 0);
    }

    size_t _idx_linear(size_t i) const noexcept
    {
        if (_head >= _tail) { return i; }
        return i - _tail * (i >= _tail) + (_cap() - _tail) * (i < _tail);
    }

    void _release() {
        assert(!empty());
        reinterpret_cast<Ty_&>(_data[_tail]).~Ty_();
        _tail = _next(_tail);
    }

    Ty_& _front() const {
        assert(!empty());
        return reinterpret_cast<Ty_&>(const_cast<chunk_t&>(_data[_tail]));
    }

    Ty_& _at(size_t i) const {
        assert(!empty());
        return reinterpret_cast<Ty_&>(const_cast<chunk_t&>(_data[i]));
    }

    Ty_& _back() const {
        assert(!empty());
        return _at(_prev(_head));
    }

private:
    std::array<chunk_t, Cap_ + 1> _data;
    size_t _head = {};
    size_t _tail = {};
};
} // namespace kangsw
