#pragma once

#include <vector>
#include <iterator>
#include <algorithm>
#include <concepts>
#include <cassert>

template<typename index_T, typename dense_T, typename dense_base_T, index_T dense_base_T::* dense_T_index_ptr> requires
    requires (dense_T t){index_T(t.*dense_T_index_ptr);}
class sparse_set
{
public:
    class iterator
    {
    public:
        typedef iterator self_type;
        typedef dense_T value_type;
        typedef dense_T& reference;
        typedef dense_T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;
        explicit iterator(pointer ptr) : ptr_(ptr) { }
        self_type& operator++() { ++ptr_; return *this;  }
        self_type operator++(int junk) { self_type i = *this; ++ptr_; return i; }
        reference operator*() { return *ptr_; }
        pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
    private:
        pointer ptr_;
    };

    class const_iterator
    {
    public:
        typedef const_iterator self_type;
        typedef dense_T value_type;
        typedef const dense_T& reference;
        typedef const dense_T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;
        explicit const_iterator(pointer ptr) : ptr_(ptr) { }
        self_type& operator++() { ++ptr_; return *this;  }
        self_type operator++(int junk) { self_type i = *this; ++ptr_; return i; }
        reference operator*() { return *ptr_; }
        pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
    private:
        pointer ptr_;
    };

    sparse_set() = default;
    sparse_set(const sparse_set& other, index_T offset) {add_elements(other, offset);}
    sparse_set(sparse_set&& other, index_T offset) {add_elements(std::move(other), offset);}

    void deinit()
    {
        sparse_array_offset = index_T(-1);
        sparse_array.clear();
        dense_array.clear();
    }

    iterator begin() {return iterator(dense_array.data());}
    iterator end() {return iterator(dense_array.data() + dense_array.size());}
    const_iterator begin() const {return const_iterator(dense_array.data());}
    const_iterator end() const {return const_iterator(dense_array.data() + dense_array.size());}
    const_iterator cbegin() const {return const_iterator(dense_array.data());}
    const_iterator cend() const {return const_iterator(dense_array.data() + dense_array.size());}

    [[nodiscard]] dense_T& operator[](index_T index)
    {
        assert(index != index_T(-1) && sparse_array_offset != index_T(-1));
        assert(sparse_array_offset <= index && index < sparse_array_offset + sparse_array.size());
        assert(sparse_array[index - sparse_array_offset] != (index_T)(-1));

        size_t dense_index = sparse_array[index - sparse_array_offset];
        return dense_array[dense_index];
    }
    const dense_T& operator[](index_T index) const
    {
        return const_cast<sparse_set>(this)->operator[](index);
    }

    bool does_exist(index_T index) const
    {
        return index >= sparse_array_offset &&
               index < sparse_array_offset + sparse_array.size() &&
               sparse_array[index - sparse_array_offset] != index_T(-1);
    }

    [[nodiscard]] size_t size() const
    {
        return dense_array.size();
    }

    void add_element(const dense_T& dense_element, index_T offset = 0)
    {
        auto& ref = dense_array.emplace_back(dense_element);
        index_T dense_index = static_cast<index_T>(dense_array.size() - 1);

        ref.*dense_T_index_ptr += offset;
        index_T sparse_index = ref.*dense_T_index_ptr;

        extent_array(sparse_index);

        sparse_array[sparse_index - sparse_array_offset] = dense_index;
    }

    void add_element(dense_T&& dense_element, index_T offset = 0)
    {
        auto& ref = dense_array.emplace_back(std::move(dense_element));
        index_T dense_index = static_cast<index_T>(dense_array.size() - 1);

        ref.*dense_T_index_ptr += offset;
        index_T sparse_index = ref.*dense_T_index_ptr;

        extent_array(sparse_index);

        sparse_array[sparse_index - sparse_array_offset] = dense_index;
    }

    void add_elements(const sparse_set& other, index_T offset = 0)
    {
        if (other.sparse_array_offset == -1)
            return;

        extent_array(other.sparse_array_offset + offset, other.sparse_array_offset + other.sparse_array.size() - 1 + offset);
        dense_array.reserve(size() + other.size());

        for(auto it = other.dense_array.begin(); it != other.dense_array.end(); ++it)
        {
            add_element(*it, offset);
        }
    }

    void add_elements(sparse_set&& other, index_T offset = 0)
    {
        if (other.sparse_array_offset == -1)
            return;

        extent_array(other.sparse_array_offset + offset, other.sparse_array_offset + other.sparse_array.size() - 1 + offset);
        dense_array.reserve(size() + other.size());

        for(auto it = std::make_move_iterator(other.dense_array.begin()); it != std::make_move_iterator(other.dense_array.end()); ++it)
        {
            add_element(*it, offset);
        }
    }

    template<class InputIt>
        requires std::same_as<sparse_set, typename InputIt::value_type>
    void add_elements_list(InputIt first, InputIt last)
    {
        if (first == last)
            return;

        index_T min_index = -1;
        index_T max_index = 0;
        size_t dense_final_size = size();
        for (auto _first = first; _first != last; ++_first)
        {
            if((*_first).sparse_array_offset != index_T(-1))
            {
                min_index = std::min(min_index, index_T((*_first).sparse_array_offset));
                max_index = std::max(max_index, index_T((*_first).sparse_array_offset + (*_first).sparse_array.size() - 1));
                dense_final_size += (*_first).size();
            }
        }

        if(min_index != index_T(-1))
        {
            extent_array(min_index, max_index);
            dense_array.reserve(dense_final_size);

            for (auto _first = first; _first != last; ++_first)
            {
                add_elements(*_first);
            }
        }
    }

    void remove_oneshot_added_ranges(std::vector<std::pair<index_T, index_T>> ranges_to_remove)
    {
        if(ranges_to_remove.empty())
            return;

        std::vector<std::pair<size_t, size_t>> dense_ranges_to_remove;
        dense_ranges_to_remove.reserve(ranges_to_remove.size());

        for(const auto& this_range: ranges_to_remove)
        {
            size_t first_dense_index = sparse_array[this_range.first - sparse_array_offset];
            size_t last_dense_index = sparse_array[this_range.second - sparse_array_offset];
            assert(first_dense_index != index_T(-1) && last_dense_index != index_T(-1));

            dense_ranges_to_remove.emplace_back(first_dense_index, last_dense_index);
        }

        std::sort(dense_ranges_to_remove.begin(), dense_ranges_to_remove.end(), [](const auto& a, const auto& b) {return a.first < b.first; });

        size_t current_remove_range = 0;
        auto it_overwrite = std::next(dense_array.begin(), dense_ranges_to_remove[0].first);
        for(auto it = it_overwrite; it != dense_array.end(); ++it)
        {
            if(current_remove_range < dense_ranges_to_remove.size() 
               && int(dense_ranges_to_remove[current_remove_range].first) <= std::distance(dense_array.begin(), it)
               && std::distance(dense_array.begin(), it) <= int(dense_ranges_to_remove[current_remove_range].second))
            {
                sparse_array[*it.*dense_T_index_ptr - sparse_array_offset] = index_T(-1);

                if (std::distance(dense_array.begin(), it) == dense_ranges_to_remove[current_remove_range].second)
                    ++current_remove_range;
            }
            else
            {
                *it_overwrite = std::move(*it);
                sparse_array[*it_overwrite.*dense_T_index_ptr - sparse_array_offset] = static_cast<index_T>(std::distance(dense_array.begin(), it_overwrite));

                ++it_overwrite;
            }
        }

        size_t new_size = std::distance(dense_array.begin(), it_overwrite);
        dense_array.resize(new_size, dense_T(-1));     
    }

private:
    void extent_array(size_t min_index, size_t max_index)
    {
        if(sparse_array_offset == index_T(-1)) [[unlikely]]
        {
            size_t range_size = max_index - min_index + 1;
            sparse_array.resize(range_size, index_T(-1));

            sparse_array_offset = static_cast<index_T>(min_index);
        }
        else [[likely]]
        {
            if(min_index < sparse_array_offset) [[unlikely]]
            { 
                std::vector<index_T> new_sparse_array;

                size_t new_reserve = std::max(max_index, sparse_array_offset + sparse_array.size() - 1) - min_index;
                new_sparse_array.reserve(new_reserve);

                size_t front_blank_range_size = sparse_array_offset - min_index;
                new_sparse_array.resize(front_blank_range_size, index_T(-1));

                std::copy(std::make_move_iterator(sparse_array.begin()), std::make_move_iterator(sparse_array.end()),
                          std::back_inserter(new_sparse_array));

                sparse_array = std::move(new_sparse_array);

                sparse_array_offset = static_cast<index_T>(min_index);
            }
            if(max_index > sparse_array_offset + sparse_array.size() - 1) [[unlikely]]
            {
                size_t range_size = max_index - sparse_array_offset + 1;
                sparse_array.resize(range_size, index_T(-1));
            }
        }
    }
    void extent_array(size_t index)
    {
        extent_array(index, index);
    }

private:
    index_T sparse_array_offset = index_T(-1);
    std::vector<index_T> sparse_array;
    std::vector<dense_T> dense_array;
};