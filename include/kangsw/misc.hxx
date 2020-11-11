#pragma once
#include <execution>
#include <iterator>
#include <optional>
#include <string>

namespace kangsw {
template <typename Ty_>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class counter_base {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = Ty_&;
    using pointer = Ty_*;

public:
    counter_base() noexcept
        : count_(0)
    {
        ;
    }
    counter_base(Ty_ rhs) noexcept
        : count_(rhs)
    {
        ;
    }
    counter_base(counter_base const& rhs) noexcept
        : count_(rhs.count_)
    {
        ;
    }

public:
    friend counter_base operator+(counter_base c, difference_type n) { return counter_base(c.count_ + n); }
    friend counter_base operator+(difference_type n, counter_base c) { return c + n; }
    friend counter_base operator-(counter_base c, difference_type n) { return counter_base(c.count_ - n); }
    friend counter_base operator-(difference_type n, counter_base c) { return c - n; }
    difference_type operator-(counter_base o) { return count_ - o.count_; }
    counter_base& operator+=(difference_type n) { return count_ += n, *this; }
    counter_base& operator-=(difference_type n) { return count_ -= n, *this; }
    counter_base& operator++() { return ++count_, *this; }
    counter_base operator++(int) { return ++count_, counter_base(count_ - 1); }
    counter_base& operator--() { return --count_, *this; }
    counter_base operator--(int) { return --count_, counter_base(count_ - 1); }
    bool operator<(counter_base o) const { return count_ < o.count_; }
    bool operator>(counter_base o) const { return count_ > o.count_; }
    bool operator==(counter_base o) const { return count_ == o.count_; }
    bool operator!=(counter_base o) const { return count_ != o.count_; }
    Ty_ const& operator*() const { return count_; }
    Ty_ const* operator->() const { return &count_; }
    Ty_ const& operator*() { return count_; }
    Ty_ const* operator->() { return &count_; }

private:
    Ty_ count_;
};

template <typename Ty_>
class counter_range_base {
public:
    counter_range_base(Ty_ min, Ty_ max) noexcept
        : min_(min)
        , max_(max)
    {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    counter_range_base(Ty_ max) noexcept
        : min_(Ty_{})
        , max_(max)
    {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    counter_base<Ty_> begin() const { return min_; }
    counter_base<Ty_> cbegin() const { return min_; }
    counter_base<Ty_> end() const { return max_; }
    counter_base<Ty_> cend() const { return max_; }

private:
    Ty_ min_, max_;
};

using counter = counter_base<int64_t>;
using counter_range = counter_range_base<int64_t>;

/**
 * Executes for_each with given parallel execution policy. However, it provides current partition index within given callback.
 * It is recommended to set num_partitions as same as current thread count.
 */
template <typename It_, typename Fn_, typename ExPo_>
void for_each_partition(ExPo_&&, It_ first, It_ last, Fn_&& cb, size_t num_partitions = std::thread::hardware_concurrency())
{
    if (first == last) { throw std::invalid_argument("Zero argument"); }
    if (num_partitions == 0) { throw std::invalid_argument("Invalid partition size"); }
    size_t num_elems = std::distance(first, last);
    size_t steps = (num_elems - 1) / num_partitions + 1;
    num_partitions = std::min(num_elems, num_partitions);
    counter_range partitions(num_partitions);

    std::for_each(
      ExPo_{},
      partitions.begin(),
      partitions.end(),
      [num_elems, steps, &cb, &first](size_t partition_index) {
          It_ it = first, end;
          std::advance(it, steps * partition_index);
          std::advance(end = it, steps * (partition_index + 1) <= num_elems ? steps : num_elems - steps * partition_index);

          for (; it != end; ++it) {
              if constexpr (std::is_invocable_v<Fn_, decltype(*it)>) {
                  cb(*it);
              }
              else if constexpr (std::is_invocable_v<Fn_, decltype(*it), size_t /*partition*/>) {
                  cb(*it, partition_index);
              }
              else {
                  static_assert(false, "given callback has invalid signature");
              }
          }
      });
}

/**
 * convenient helper method for for_reach_partition
 */
template <typename ExPo_, typename Fn_>
void for_each_indexes(ExPo_&&, int64_t begin, int64_t end, Fn_&& cb, size_t num_partitions = std::thread::hardware_concurrency())
{
    if (begin < end) { throw std::invalid_argument("end precedes begin"); }

    counter_range range(begin, end);
    for_each_partition(ExPo_{}, range.begin(), range.end(), std::forward<Fn_>(cb), num_partitions);
}

template <typename ExPo_, typename Fn_>
void for_each_indexes(int64_t begin, int64_t end, Fn_&& cb)
{
    counter_range range(begin, end);
    std::for_each(range.begin(), range.end(), std::forward<Fn_>(cb));
}

template <typename... Args_>
std::string format(char const* fmt, Args_&&... args)
{
    std::string s;
    auto buflen = snprintf(nullptr, 0, fmt, std::forward<Args_>(args)...);
    s.resize(buflen);

    snprintf(s.data(), buflen, fmt, std::forward<Args_>(args)...);
    return s;
}

enum class recurse_return {
    do_continue,
    do_break
};

namespace impl__ {
enum class recurse_policy_base {
    preorder,
    postorder,
};
template <impl__::recurse_policy_base Val_>
using recurse_policy_v = std::integral_constant<impl__::recurse_policy_base, Val_>;

} // namespace impl__

namespace recurse {
constexpr impl__::recurse_policy_v<impl__::recurse_policy_base::preorder> preorder;
constexpr impl__::recurse_policy_v<impl__::recurse_policy_base::postorder> postorder;
} // namespace recurse

/**
 * ��������� �۾��� �����մϴ�.
 * @param root ��Ʈ�� �Ǵ� ����Դϴ�.
 * @param recurse Ty_�κ��� ���� ��带 �����մϴ�. void(Ty_& parent, void (emplacer)(Ty_&)) �ñ״��ĸ� ���� �ݹ�����, parent�� �ڼ� ��带 iterate�� ������ ��忡 ���� emplacer(node)�� ȣ���Ͽ� ������� �۾��� ������ �� �ֽ��ϴ�.
 * @param op ��� �� �� ��忡 ���� ������ �۾��Դϴ�. [optional] recurse_return�� ��ȯ�Ͽ� ��� ���� �������� �� �ֽ��ϴ�.
 *          ������ signature: 
 * 
 */
template <
  typename Ty_, typename Recurse_, typename Op_,
  impl__::recurse_policy_base Policy_ = impl__::recurse_policy_base::preorder>
decltype(auto) recurse_for_each(
  Ty_&& root, Recurse_&& recurse, Op_&& op,
  std::integral_constant<impl__::recurse_policy_base, Policy_> = {})
{
    auto operate = [&](auto&& ref) {
        if constexpr (std::is_invocable_v<Op_, Ty_, size_t>) {
            if constexpr (std::is_invocable_r_v<recurse_return, Op_, Ty_, size_t>) {
                if (op(ref.first, ref.second) == recurse_return::do_break) { return false; }
            }
            else {
                op(ref.first, ref.second);
            }
        }
        else {
            if constexpr (std::is_invocable_r_v<recurse_return, Op_, Ty_, size_t>) {
                if (op(ref.first) == recurse_return::do_break) { return false; }
            }
            else {
                op(ref.first);
            }
        }

        return true;
    };

    if constexpr (Policy_ == impl__::recurse_policy_base::preorder) {
        std::vector<std::pair<Ty_&, size_t>> stack;
        stack.emplace_back(root, 0);

        while (!stack.empty()) {
            auto ref = stack.back();
            stack.pop_back();

            operate(ref);
            recurse(ref.first, [&stack, n = ref.second + 1](Ty_& arg) { stack.emplace_back(arg, n); });
        }
    }
    else if constexpr (Policy_ == impl__::recurse_policy_base::postorder) {
        static_assert(false);
    }
    else {
        static_assert(false);
    }
}

namespace impl__ {
template <typename... Args_>
class zip_iterator {
public:
    using tuple_type = std::tuple<Args_...>;
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<decltype(*Args_{})...>;
    using reference = value_type&;
    using pointer = value_type*;

private:
    template <size_t... N_>
    value_type _deref(std::index_sequence<N_...>) const
    {
        return std::make_tuple(std::ref(*std::get<N_>(iterator_pack_))...);
    }

    template <size_t N_ = 0>
    bool _compare_strict(zip_iterator const& op, bool previous)
    {
        if constexpr (N_ != sizeof...(Args_)) {
            auto result = std::get<N_>(op) == std::get<N_>(iterator_pack_);
            if (result != previous) {
                throw std::invalid_argument("packed tuples has difference lengths");
            }

            return _compare_strict<N_ + 1>(op, result);
        }
        else {
            return previous;
        }
    }

public:
    bool operator==(zip_iterator const& op) const
    {
        return _compare_strict(op, std::get<0>(op) == std::get<0>(iterator_pack_));
    }

    zip_iterator& operator++()
    {
        std::apply([](auto&&... arg) { (++arg, ...); }, iterator_pack_);
        return *this;
    }

    zip_iterator operator++(int)
    {
        auto copy = *this;
        return ++*this, copy;
    }

    value_type operator*() const
    {
        return _deref(std::make_index_sequence<sizeof...(Args_)>{});
    }

public:
    tuple_type iterator_pack_;
};

template <typename... Args_>
class zip_range {
public:
    using tuple_type = std::tuple<Args_...>;

    zip_iterator<Args_...> begin() const { return {begin_}; }
    zip_iterator<Args_...> end() const { return {end_}; }

public:
    tuple_type begin_;
    tuple_type end_;
};
} // namespace impl__

} // namespace kangsw
