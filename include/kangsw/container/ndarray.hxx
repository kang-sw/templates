#pragma once
#include <array>
#include <numeric>
#include <vector>
#include "kangsw/helpers/zip.hxx"

namespace kangsw::inline containers {
/**
 * N-dimensional array
 * Basically, wrapper of an vector
 */
template <typename Ty_, size_t Dim_ = 1>
class ndarray {
public:
    using value_type = typename std::vector<Ty_>::value_type;
    using reference = typename std::vector<Ty_>::reference;
    using const_reference = typename std::vector<Ty_>::const_reference;
    using size_type = size_t;
    using dimension_type = std::array<size_type, Dim_>;
    enum : size_t { dimension = Dim_ };

private:
    template <size_type D_, bool Check_ = false, typename T_, typename... Args_>
    size_type _reduce_index(T_ idx, Args_... args) const {
        if constexpr (Check_) {
            if (idx >= dim_[D_]) {
                throw std::invalid_argument("array index out of range");
            }
        }

        if constexpr (sizeof...(Args_)) {
            return idx * steps_[D_] + _reduce_index<D_ + 1, Check_>(args...);
        }
        else {
            return idx;
        }
    }

    auto _get_index(dimension_type const& r) const {
        size_type index = 0;
        for (size_type i = 0; i < r.size() - 1; ++i) { index += steps_[i] * r[i]; }
        // printf("%d %d %d ==> %d\n", r[0], r[1], r[2], index + r.back());
        return index + r.back();
    }

    auto _apply_reshape() {
        data_.resize(std::reduce(dim_.begin(), dim_.end(), size_type(1), std::multiplies<>{}));

        auto it_dim = dim_.end() - 1;
        auto it_dim_end = dim_.begin();
        auto it_step = steps_.end() - 1;

        for (size_t step = 1;; --it_step) {
            step *= *it_dim;
            *it_step = step;

            if (--it_dim == it_dim_end) { break; }
        }
    }

public:
    template <typename... Ints_>
    requires((sizeof...(Ints_) == dimension) && (std::is_integral_v<Ints_> && ...))
      ndarray(Ints_... ints) {
        if constexpr (sizeof...(Ints_)) { reshape(ints...); }
    }

    ndarray() noexcept = default;
    ndarray(ndarray const&) noexcept = default;
    ndarray(ndarray&&) noexcept = default;
    ndarray& operator=(ndarray&&) noexcept = default;
    ndarray& operator=(ndarray const&) noexcept = default;

public:
    template <typename... Values_>
    requires((sizeof...(Values_) == dimension) && (std::is_integral_v<Values_> && ...)) //
      void reshape(Values_... values) {
        static_assert(sizeof...(values) == dimension);
        auto value = {size_type(values)...};
        std::copy(value.begin(), value.end(), dim_.begin());

        _apply_reshape();
    }

    void reshape(dimension_type const& new_dims) {
        dim_ = new_dims;
        _apply_reshape();
    }

    template <typename... Idxs_> //
    requires((sizeof...(Idxs_) == dimension) && (std::is_integral_v<Idxs_> && ...))
    reference operator()(Idxs_... index) {
        return data_[_reduce_index<0>(index...)];
    }

    template <typename... Idxs_>
    requires((sizeof...(Idxs_) == dimension) && (std::is_integral_v<Idxs_> && ...))
      const_reference operator()(Idxs_... index) const {
        return data_[_reduce_index<0>(index...)];
    }

    template <typename... Idxs_>
    requires((sizeof...(Idxs_) == dimension) && (std::is_integral_v<Idxs_> && ...))
      reference at(Idxs_... index) {
        return data_[_reduce_index<0, true>(index...)];
    }

    template <typename... Idxs_>
    requires((sizeof...(Idxs_) == dimension) && (std::is_integral_v<Idxs_> && ...))
      const_reference at(Idxs_... index)
    const {
        return data_[_reduce_index<0, true>(index...)];
    }

    const_reference at(dimension_type const& i) const { return data_.at(_get_index(i)); }
    reference at(dimension_type const& i) { return data_.at(_get_index(i)); }
    const_reference operator[](dimension_type const& i) const { return data_[_get_index(i)]; }
    reference operator[](dimension_type const& i) { return data_[_get_index(i)]; }

    auto begin() { return data_.begin(); }
    auto cbegin() const { return data_.cbegin(); }
    auto end() { return data_.end(); }
    auto cend() const { return data_.cend(); }

    auto size() const { return data_.size(); }
    auto dims() const { return dim_; }
    auto shrink_to_fit() { data_.shrink_to_fit(); }

    auto data() const { return data_.data(); }
    auto data() { return data_.data(); }

    template <typename It_>
    void assign(It_ first, It_ last) {
        if (std::distance(first, last) != size()) { throw std::logic_error{"Assignment size mismatch"}; }
        data_.assign(first, last);
    }

    void assign(std::initializer_list<value_type> values) { assign(values.begin(), values.end()); }

    bool operator==(ndarray const& r) const { return dim_ == r.dim_ && data_ == r.data_; }
    bool operator!=(ndarray const& r) const { return !(*this == r); }

private:
    dimension_type dim_;
    std::array<size_type, dimension - 1> steps_;
    std::vector<Ty_> data_;
}; // namespace kangsw::inline containers

} // namespace kangsw::inline containers
