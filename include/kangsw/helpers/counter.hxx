#pragma once
#include <array>
#include <iterator>
#include <numeric>
#include "tuple_for_each.hxx"

namespace kangsw::inline counters {

template <typename Ty_, size_t Dim_ = 1>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter;

/**
 * iota counter iterator
 */
template <typename Ty_>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter<Ty_, 1> {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = Ty_&;
    using pointer = Ty_*;

public:
    constexpr _counter() noexcept
        :
        count_(0) {
        ;
    }
    constexpr _counter(Ty_ rhs) noexcept
        :
        count_(rhs) {
        ;
    }
    constexpr _counter(_counter const& rhs) noexcept
        :
        count_(rhs.count_) {
        ;
    }

public:
    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr friend _counter operator+(_counter c, Integer_ n) { return _counter(c.count_ + n); }

    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr friend _counter operator+(Integer_ n, _counter c) { return c + n; }

    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr friend _counter operator-(_counter c, difference_type n) { return _counter(c.count_ - n); }

    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr friend _counter operator-(Integer_ n, _counter c) { return c - n; }

    constexpr difference_type operator-(_counter o) const { return count_ - o.count_; }

    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr _counter& operator+=(Integer_ n) { return count_ += n, *this; }

    template <typename Integer_>
    requires std::is_integral_v<Integer_>
    constexpr _counter& operator-=(Integer_ n) { return count_ -= n, *this; }

    constexpr _counter& operator++() { return ++count_, *this; }
    constexpr _counter operator++(int) { return ++count_, _counter(count_ - 1); }
    constexpr _counter& operator--() { return --count_, *this; }
    constexpr _counter operator--(int) { return --count_, _counter(count_ - 1); }
    constexpr bool operator<(_counter o) const { return count_ < o.count_; }
    constexpr bool operator>(_counter o) const { return count_ > o.count_; }
    constexpr bool operator==(_counter o) const { return count_ == o.count_; }
    constexpr bool operator!=(_counter o) const { return count_ != o.count_; }
    constexpr auto /*Ty_ const&*/ operator*() const { return count_; }
    constexpr auto /*Ty_ const**/ operator->() const { return &count_; }
    constexpr auto /*Ty_ const&*/ operator*() { return count_; }
    constexpr auto /*Ty_ const**/ operator->() { return &count_; }

private:
    Ty_ count_;
};

/**
 * iota counter iterator
 */
template <typename Ty_>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter<Ty_, -1> {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = Ty_&;
    using pointer = Ty_*;

public:
    constexpr _counter() noexcept
        :
        count_(0) {
        ;
    }
    constexpr _counter(Ty_ rhs) noexcept
        :
        count_(rhs) {
        ;
    }
    constexpr _counter(_counter const& rhs) noexcept
        :
        count_(rhs.count_) {
        ;
    }

public:
    constexpr friend _counter operator-(_counter c, difference_type n) { return _counter(c.count_ + n); }
    constexpr friend _counter operator-(difference_type n, _counter c) { return c + n; }
    constexpr friend _counter operator+(_counter c, difference_type n) { return _counter(c.count_ - n); }
    constexpr friend _counter operator+(difference_type n, _counter c) { return c - n; }
    constexpr difference_type operator-(_counter o) const { return o.count_ - count_; }
    constexpr _counter& operator-=(difference_type n) { return count_ += n, *this; }
    constexpr _counter& operator+=(difference_type n) { return count_ -= n, *this; }
    constexpr _counter& operator--() { return ++count_, *this; }
    constexpr _counter operator--(int) { return ++count_, _counter(count_ - 1); }
    constexpr _counter& operator++() { return --count_, *this; }
    constexpr _counter operator++(int) { return --count_, _counter(count_ - 1); }
    constexpr bool operator>(_counter o) const { return count_ < o.count_; }
    constexpr bool operator<(_counter o) const { return count_ > o.count_; }
    constexpr bool operator==(_counter o) const { return count_ == o.count_; }
    constexpr bool operator!=(_counter o) const { return count_ != o.count_; }
    constexpr auto /*Ty_ const&*/ operator*() const { return count_; }
    constexpr auto /*Ty_ const**/ operator->() const { return &count_; }
    constexpr auto /*Ty_ const&*/ operator*() { return count_; }
    constexpr auto /*Ty_ const**/ operator->() { return &count_; }

private:
    Ty_ count_;
};

template <typename Ty_>
class iota {
public:
    constexpr iota(Ty_ min, Ty_ max) noexcept
        :
        min_(min),
        max_(max) {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr iota(Ty_ max) noexcept
        :
        min_(Ty_{}),
        max_(max) {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr _counter<Ty_> begin() { return min_; }
    constexpr _counter<Ty_> begin() const { return min_; }
    constexpr _counter<Ty_> cbegin() const { return min_; }

    constexpr _counter<Ty_> end() { return max_; }
    constexpr _counter<Ty_> end() const { return max_; }
    constexpr _counter<Ty_> cend() const { return max_; }

private:
    Ty_ min_, max_;
};

class _counter_end_marker_t {};

template <typename Ty_, size_t Dim_>
class _counter {
public:
    enum { num_dimension = Dim_ };

    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = value_type&;
    using pointer = value_type*;

    using dimension = std::array<Ty_, num_dimension>;

public:
    template <typename... Ints_>
    constexpr _counter& fetch_from(Ints_&&... ints) { return *this; }

    constexpr auto& operator[](size_t dim) const { return current[dim]; }
    constexpr auto& operator[](size_t dim) { return current[dim]; }

    template <size_t N_ = Dim_ - 1>
    constexpr void incr() {
        if constexpr (N_ == size_t(-1)) {
            current = max;
        }
        else {
            if (++current[N_] == max[N_]) {
                current[N_] = value_type{};
                incr<N_ - 1>();
            }
        }
    }

    constexpr _counter& operator++() { return incr(), *this; }
    constexpr _counter operator++(int) {
        auto cpy = *this;
        incr();
        return cpy;
    }

    constexpr bool operator==(_counter const& o) const { return current == o.current; }
    constexpr bool operator!=(_counter const& o) const { return !(*this == o); }

    // constexpr bool operator==(_counter_end_marker_t) const { return current[0] == max[0]; }
    // constexpr bool operator!=(_counter_end_marker_t) const { return current[0] != max[0]; }

    constexpr auto& operator*() const { return current; }
    constexpr auto operator->() const { return &current; }
    constexpr auto& operator*() { return current; }
    constexpr auto operator->() { return &current; }

public:
    dimension max;
    dimension current;
};

template <typename SizeTy_, size_t Dim_>
struct _count_index {
    using iterator = _counter<SizeTy_, Dim_>;
    using dimension = typename iterator::dimension;
    constexpr iterator begin() const { return _counter<SizeTy_, Dim_>{max, {}}; }
    // constexpr iterator end() const { return _counter<SizeTy_, Dim_>{max, max}; }
    constexpr iterator end() const { return _counter<SizeTy_, Dim_>{max, max}; }

    dimension max;
};

template <typename SizeTy_>
struct _count_index<SizeTy_, 0> {};

template <typename SizeTy_>
constexpr auto counter(SizeTy_ size) {
    return iota<SizeTy_>{size};
} // namespace kangsw

template <typename SizeTy_>
constexpr auto rcounter(SizeTy_ size) {
    struct min_counter_gen {
        SizeTy_ begin_;
        SizeTy_ end_;
        constexpr _counter<SizeTy_, -1> begin() const { return {begin_}; }
        constexpr _counter<SizeTy_, -1> end() const { return {end_}; }
    };

    return min_counter_gen{.begin_ = SizeTy_(size - 1), .end_ = SizeTy_(-1)};
} // namespace kangsw

template <typename SizeTy_, typename... Ints_>
constexpr auto counter(SizeTy_ size, Ints_... args) {
    constexpr auto n_dim = sizeof...(Ints_) + 1;
    using size_type = std::decay_t<SizeTy_>;
    _count_index<size_type, n_dim> counter{};
    counter.max[0] = std::forward<SizeTy_>(size);
    tuple_for_each(
      std::forward_as_tuple(std::forward<Ints_>(args)...),
      [&]<typename Int_>(Int_&& r, size_t i) { counter.max[i + 1] = std::forward<Int_>(r); });
    return counter;
}

template <typename SizeTy_, size_t Dim_>
constexpr auto counter(std::array<SizeTy_, Dim_> const& idx) {
    _count_index<SizeTy_, Dim_> counter{};
    bool has_zero = false;
    for (size_t i = 0; i < Dim_; ++i) {
        counter.max[i] = idx[i];
        has_zero = has_zero || idx[i] == 0;
    }

    counter.max[0] *= !has_zero;
    return counter;
}

} // namespace kangsw::inline counters
