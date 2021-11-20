#pragma once

#include <vector>
#include <iterator>
#include <algorithm>
#include <concepts>
#include <cassert>

template<typename index_T, typename dense_T, typename dense_base_T, index_T dense_base_T::* dense_T_index_ptr> requires
    requires (dense_T t){index_T(t.*dense_T_index_ptr);}
class dense_set
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
        explicit iterator(pointer ptr, pointer ptr_end) : ptr_(ptr), ptr_end_(ptr_end) { }
        self_type& operator++() { ++ptr_; go_to_valid_element(); return *this;  }
        self_type operator++(int junk) { self_type i = *this; ++ptr_; go_to_valid_element(); return i; }
        reference operator*() { return *ptr_; }
        pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) const { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) const { return ptr_ != rhs.ptr_; }

        void go_to_valid_element()
        {
            assert(ptr_ <= ptr_end_);

            while(ptr_ != ptr_end_)
            {
                if(*ptr_.*dense_T_index_ptr != index_T(-1))
                    break;

                ++ptr_;
            }
        }

    private:
        pointer ptr_;
        pointer ptr_end_;
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
        explicit const_iterator(pointer ptr, pointer ptr_end) : ptr_(ptr), ptr_end_(ptr_end) { }
        self_type& operator++() { ++ptr_; go_to_valid_element(); return *this;  }
        self_type operator++(int junk) { self_type i = *this; ++ptr_; go_to_valid_element(); return i; }
        reference operator*() { return *ptr_; }
        pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) const { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) const { return ptr_ != rhs.ptr_; }

        void go_to_valid_element()
        {
            assert(ptr_ <= ptr_end_);

            while(ptr_ != ptr_end_)
            {
                if(*ptr_.*dense_T_index_ptr != index_T(-1))
                    break;

                ++ptr_;
            }
        }

    private:
        pointer ptr_;
        pointer ptr_end_;
    };

    dense_set() = default;
    dense_set(const dense_set& other, index_T offset) {add_elements(other, offset);}
    dense_set(dense_set&& other, index_T offset) {add_elements(std::move(other), offset);}

    void deinit()
    {
        array_offset = index_T(-1);
        array.clear();
    }


    iterator begin() {auto it = iterator(array.data(), array.data() + array.size()); it.go_to_valid_element(); return it;}
    iterator end() {auto it = iterator(array.data() + array.size(), array.data() + array.size()); it.go_to_valid_element(); return it;}
    const_iterator begin() const {auto it = const_iterator(array.data(), array.data() + array.size()); it.go_to_valid_element(); return it;}
    const_iterator end() const {auto it = const_iterator(array.data() + array.size(), array.data() + array.size()); it.go_to_valid_element(); return it;}
    const_iterator cbegin() const {auto it = const_iterator(array.data(), array.data() + array.size()); it.go_to_valid_element(); return it;}
    const_iterator cend() const {auto it = const_iterator(array.data() + array.size(), array.data() + array.size()); it.go_to_valid_element(); return it;}

    [[nodiscard]] std::pair<iterator, iterator> get_range_iterators(const std::pair<index_T, index_T>& range)
    {
        std::pair<iterator, iterator> range_iterators = {iterator(array.data() + range.first - array_offset     , array.data() + range.second - array_offset + 1),
                                                         iterator(array.data() + range.second - array_offset + 1, array.data() + range.second - array_offset + 1)};

        return range_iterators;
    }

    [[nodiscard]] std::pair<const_iterator, const_iterator> get_range_iterators(const std::pair<index_T, index_T>& range) const
    {
        std::pair<const_iterator, const_iterator> range_iterators = {const_iterator(array.data() + range.first - array_offset     , array.data() + range.second - array_offset + 1),
                                                                     const_iterator(array.data() + range.second - array_offset + 1, array.data() + range.second - array_offset + 1)};

        return range_iterators;
    }

    [[nodiscard]] dense_T& operator[](index_T index)
    {
        assert(index != index_T(-1) && array_offset != index_T(-1));
        assert(array_offset <= index && index < array_offset + array.size());
        assert(array[index - array_offset].*dense_T_index_ptr != (index_T)(-1));

        return array[index - array_offset];
    }
    const dense_T& operator[](index_T index) const
    {
        return const_cast<dense_set>(this)->operator[](index);
    }

    bool does_exist(index_T index) const
    {
        return index >= array_offset &&
               index < array_offset + array.size() &&
               array[index - array_offset].*dense_T_index_ptr != index_T(-1);
    }

    [[nodiscard]] size_t container_size() const
    {
        return array.size();
    }

    [[nodiscard]] size_t size() const
    {
        return valid_objs;
    }

    void add_element(const dense_T& dense_element, index_T offset = 0)
    {
        index_T index = dense_element.*dense_T_index_ptr + offset;

        extent_array(index);

        auto& ref = array[index - array_offset] = dense_element;
        ref.*dense_T_index_ptr += offset;

        ++valid_objs;
    }

    void add_element(dense_T&& dense_element, index_T offset = 0)
    {
        index_T index = dense_element.*dense_T_index_ptr + offset;

        extent_array(index);

        auto& ref = array[index - array_offset] = std::move(dense_element);
        ref.*dense_T_index_ptr += offset;

        ++valid_objs;
    }

    void add_elements(const dense_set& other, index_T offset = 0)
    {
        if (other.array_offset == index_T(-1))
            return;

        extent_array(other.array_offset + offset, other.array_offset + other.array.size() - 1 + offset);

        for(auto it = other.array.begin(); it != other.array.end(); ++it)
        {
            add_element(*it, offset);
        }
    }

    void add_elements(dense_set&& other, index_T offset = 0)
    {
        if (other.array_offset == index_T(-1))
            return;

        extent_array(other.array_offset + offset, other.array_offset + other.array.size() - 1 + offset);

        for(auto it = std::make_move_iterator(other.array.begin()); it != std::make_move_iterator(other.array.end()); ++it)
        {
            add_element(*it, offset);
        }
    }

    template<class InputIt>
        requires std::same_as<dense_set, typename InputIt::value_type>
    void add_elements_list(InputIt first, InputIt last)
    {
        if (first == last)
            return;

        index_T min_index = -1;
        index_T max_index = 0;
        for (auto _first = first; _first != last; ++_first)
        {
            if((*_first).array_offset != index_T(-1))
            {
                min_index = std::min(min_index, index_T((*_first).array_offset));
                max_index = std::max(max_index, index_T((*_first).array_offset + (*_first).array.size() - 1));
            }
        }

        if(min_index != index_T(-1))
        {
            extent_array(min_index, max_index);

            for (auto _first = first; _first != last; ++_first)
            {
                add_elements((*_first));
            }
        }
    }

    void remove_oneshot_added_ranges(std::vector<std::pair<index_T, index_T>> ranges_to_remove)
    {
        if(ranges_to_remove.empty())
            return;

        for(const auto& range: ranges_to_remove)
        {
            for(auto index = range.first; index <= range.second; ++index)
            {
                if(array[index - array_offset].*dense_T_index_ptr != index_T(-1))
                {
                    array[index - array_offset].~dense_T();
                    array[index - array_offset].*dense_T_index_ptr = index_T(-1);

                    --valid_objs;
                }
            }
        }
    }

private:
    void extent_array(size_t min_index, size_t max_index)
    {
        if(array_offset == index_T(-1)) [[unlikely]]
        {
            size_t range_size = max_index - min_index + 1;
            array.resize(range_size, -1);

            array_offset = static_cast<index_T>(min_index);
        }
        else [[likely]]
        {
            if(min_index < array_offset) [[unlikely]]
            {
                std::vector<dense_T> new_array;

                size_t new_reserve_size = std::max(max_index, array_offset + array.size() - 1) - min_index;
                new_array.reserve(new_reserve_size);

                size_t front_blank_range_size = array_offset - min_index;
                new_array.resize(front_blank_range_size, dense_T(-1));

                std::copy(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()),
                          std::back_inserter(new_array));

                array = std::move(new_array);

                array_offset = static_cast<index_T>(min_index);
            }
            if(max_index > array_offset + array.size() - 1) [[unlikely]]
            {
                size_t range_size = max_index - array_offset + 1;
                array.resize(range_size, dense_T(-1));
            }
        }
    }
    void extent_array(size_t index)
    {
        extent_array(index, index);
    }

    private:
    index_T array_offset = index_T(-1);
    std::vector<dense_T> array;

    size_t valid_objs = 0;
};