#pragma once
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include "counter.hxx"

namespace kangsw {

/**
 * convenient helper method for for_reach_partition
 */
template <typename ExPo_, typename Fn_>
void for_each_indexes(ExPo_&&, int64_t begin, int64_t end, Fn_&& cb, size_t num_partitions = std::thread::hardware_concurrency()) {
    if (begin < end) { throw std::invalid_argument("end precedes begin"); }

    for_each_threads(iota{begin, end}, std::forward<Fn_>(cb));
}

template <typename ExPo_, typename Fn_>
void for_each_indexes(int64_t begin, int64_t end, Fn_&& cb) {
    iota range(begin, end);
    std::for_each(range.begin(), range.end(), std::forward<Fn_>(cb));
}

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
 * 
 */
template <
  typename Ref_, typename Recurse_,
  impl__::recurse_policy_base Policy_ = impl__::recurse_policy_base::preorder>
decltype(auto) recurse_for_each(
  Ref_ root, Recurse_&& recurse,
  std::integral_constant<impl__::recurse_policy_base, Policy_> = {}) {
    if constexpr (Policy_ == impl__::recurse_policy_base::preorder) {
        std::vector<std::pair<Ref_, size_t>> stack;
        stack.emplace_back(root, 0);

        while (!stack.empty()) {
            auto ref = stack.back();
            stack.pop_back();

            if constexpr (std::is_invocable_v<Recurse_, Ref_, void(Ref_)>) {
                recurse(ref.first, [&stack, n = ref.second + 1](Ref_ arg) { stack.emplace_back(arg, n); });
            }
            else if constexpr (std::is_invocable_v<Recurse_, Ref_, size_t, void(Ref_)>) {
                recurse(ref.first, ref.second, [&stack, n = ref.second + 1](Ref_ arg) { stack.emplace_back(arg, n); });
            }
            else {
                static_assert(false);
            }
        }
    }
    else if constexpr (Policy_ == impl__::recurse_policy_base::postorder) {
        static_assert(false); // do it later
    }
    else {
        static_assert(false);
    }
}

template <typename It_, typename Fn_, typename ExPo_>
void for_each_partition(ExPo_&&, It_ first, It_ last, Fn_&& cb, size_t num_partitions) {
    if (first == last) { throw std::invalid_argument("Zero argument"); }
    if (num_partitions == 0) { throw std::invalid_argument("Invalid partition size"); }
    size_t num_elems = std::distance(first, last);
    size_t steps = std::max<size_t>(1, num_elems / num_partitions);
    num_partitions = std::min(num_elems, num_partitions);
    iota partitions(num_partitions);

    std::for_each(
      ExPo_{},
      partitions.begin(),
      partitions.end(),
      [num_elems, num_partitions, steps, &cb, &first](size_t partition_index) {
          It_ it = first, end;
          size_t current_index = steps * partition_index;
          std::advance(it, current_index);
          std::advance(end = it, partition_index + 1 == num_partitions ? num_elems - current_index : steps);

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

} // namespace kangsw
