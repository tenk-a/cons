/**
 * @file static_inttype_vector.hpp
 * @brief Static vector class template for int type only.
 * @author Masashi Kitamura (tenka@6809.net)
 * @license Boost Software Lisence Version 1.0
 */
#ifndef STATIC_INTTYPE_VECTOR_DEFINED
#define STATIC_INTTYPE_VECTOR_DEFINED

#include <stddef.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#define STATIC_INTTYPE_VECTOR_CAPA_CLEAR
#if !defined(USE_OLD_COMPILER)
#include <iterator>
#endif

namespace _priv {

template<unsigned N> struct sizeof_to_uint { };
template<> struct sizeof_to_uint<1> { typedef std::uint8_t   type; };
template<> struct sizeof_to_uint<2> { typedef std::uint16_t  type; };
template<> struct sizeof_to_uint<4> { typedef std::uint32_t  type; };
template<> struct sizeof_to_uint<8> { typedef std::uint64_t  type; };

template<typename T, typename S>
class static_inttype_vector_base_base {
    typedef static_inttype_vector_base_base this_type;
    enum { OFS = (sizeof(T) > sizeof(S)*2) ? sizeof(T) : sizeof(S)*2 };
public:
    typedef T                   value_type;
    typedef S                   size_type;
    typedef value_type*         pointer;
    typedef value_type const*   const_pointer;

    static_inttype_vector_base_base(size_type capa) noexcept : capa_(capa), size_(0) { tmpClear(); }
    static_inttype_vector_base_base(size_type capa, size_type sz) noexcept : capa_(capa) { resize(sz); }
    static_inttype_vector_base_base(static_inttype_vector_base_base const& r) noexcept
        : capa_(r.capa_) { _assign(r.data(), r.size()); }

    bool            empty() const noexcept { return !size_; }
    size_type       size()  const noexcept { return size_; }
    size_type       max_size() const noexcept { return capa_; }
    size_type       capacity() const noexcept { return capa_; }
    void            reserve(size_type n) noexcept { assert(n <= capa_); }
    void            shrink_to_fit() {}

    pointer         data() noexcept { return reinterpret_cast<value_type*>((char*)this + OFS); }
    const_pointer   data() const noexcept { return const_cast<this_type*>(this)->data(); }

    pointer         begin() noexcept { return data(); }
    const_pointer   begin() const noexcept { return data(); }
    pointer         end() noexcept { return data() + size_; }
    const_pointer   end() const noexcept { return data() + size_; }

    value_type&       operator[](size_type n) noexcept { assert(n < size_); return data()[n]; }
    value_type const& operator[](size_type n) const noexcept { assert(n < size_); return data()[n]; }

    value_type&         front() noexcept { return *data(); }
    value_type const&   front() const noexcept { return *data(); }
    value_type&         back() noexcept { return data()[size_ - 1]; }
    value_type const&   back() const noexcept { return data()[size_ - 1]; }

    void pop_back() noexcept {
        if (size_)
            data()[--size_] = value_type();
    }

    void push_back(value_type b) noexcept {
        if (size_ < capa_)
            data()[size_++] = b;
        else
            assert(size_ < capa_);
    }

    void emplace_back(value_type b) noexcept {
        return push_back(b);
    }

    void resize(size_type n) noexcept {
        assert(n <= capa_);
        if (n > size_) {
          #if defined(STATIC_INTTYPE_VECTOR_CAPA_CLEAR)
            tmpClear();
          #else
            std::fill_n(end(), n - size_, value_type());
            //std::memset(end(), 0, (n - size_) * sizeof(value_type));
          #endif
        }
        size_ = n;
    }

    void clear() noexcept {
     #if defined(STATIC_INTTYPE_VECTOR_CAPA_CLEAR)
        size_ = 0;
        tmpClear();
     #else
        resize(0);
     #endif
    }

    void _assgin(const_pointer r, size_type rsz) noexcept {
        assert(capa_ >= rsz);
        size_ = rsz;
        std::copy(r, r + rsz, data());
    }

    void assign(const_pointer first, const_pointer last) noexcept {
        assert(capa_ >= last - first);
        size_ = last - first;
        std::copy(first, last, data());
    }

    void assign(size_type sz, value_type val) noexcept {
        assert(capa_ >= sz);
        size_ = sz;
        std::fill_n(data(), sz, val);
    }

    void swap(this_type& rhs) noexcept;
    void insert(pointer pos, const_pointer first, const_pointer last) noexcept;
    pointer erase(pointer first, pointer last) noexcept;

 #if 0
    bool operator==(this_type const& r) const noexcept { return std::equal(begin(), end(), r.begin(), r.end()); }
    bool operator!=(this_type const& r) const noexcept { return !operator==(r); }
    bool operator< (this_type const& r) const noexcept {
        return std::lexicographical_compare(begin(), end(), r.begin(), r.end());
    }
    bool operator>=(this_type const& r) const noexcept { return !operator<(r); }
    bool operator> (this_type const& r) const noexcept { return r.operator<(*this); }
    bool operator<=(this_type const& r) const noexcept { return !r.operator<(*this); }

    this_type& operator=(this_type const& r) noexcept {
        return _assgin(r.data(), r.size());
    }
 #endif

private:
    void tmpClear() noexcept {
     #if defined(STATIC_INTTYPE_VECTOR_CAPA_CLEAR)
        if (size_ < capa_) {
            //std::fill_n(data()+size_, capa_ - size_, value_type());
            std::memset(data()+size_, 0, (capa_ - size_) * sizeof(value_type));
        }
     #endif
    }

private:
    size_type   capa_;
    size_type   size_;
};

template<typename T, typename S>
void static_inttype_vector_base_base<T,S>::insert(T* pos, T const* first, T const* last) noexcept {
    pointer   b = begin();
    pointer   e = end();
    if (b <= pos && pos <= e) {
        assert(capa_ >= size_ + last - first);
        size_type add_sz= size_type(last - first);
        pointer   d     = data();
        size_type ofs   = pos - d;
        if (pos < e) {
            //std::copy_backward(pos, e, pos + add_sz);
            std::memmove(pos + add_sz, pos, (e - pos) * sizeof(value_type));
        }
        //std::copy(first, last, pos);
        std::memcpy(pos, first, (last - first) * sizeof(value_type));
        size_ += add_sz;
        tmpClear();
    } else {
        assert(begin() <= pos && pos <= end());
    }
}

template<typename T, typename S>
T* static_inttype_vector_base_base<T,S>::erase(T* first, T* last) noexcept {
    pointer   e = end();
    if (begin() <= first && first < last && last <= e) {
        if (last < e) {
            //std::copy_backward(last, e, first);
            std::memmove(first, last, (e - first) * sizeof(value_type));
        }
        size_type sub_sz= size_type(last - first);
        size_ -= sub_sz;
        tmpClear();
        return first;
    } else {
        if (first != last)
            assert(begin() <= first && first < last && last <= end());
        return end();
    }
}

template<typename T, typename S>
void static_inttype_vector_base_base<T,S>::swap(static_inttype_vector_base_base<T,S>& rhs) noexcept {
    size_type sz  = size_;
    size_type rsz = rhs.size_;
    size_type n   = (sz >= rsz) ? sz : rsz;
    assert(capa_ >= n && rhs.capa_ >= n);
    size_         = rsz;
    rhs.size_     = sz;
    T*        ld  = data();
    T*        rd  = rhs.data();
    for (size_type i = 0; i < n; ++i)
        std::swap(ld[i], rd[i]);
    tmpClear();
    rhs.tmpClear();
}

template<typename T, typename S>
class static_inttype_vector_base
 #if !defined(USE_OLD_COMPILER)
    : static_inttype_vector_base_base< typename sizeof_to_uint<sizeof(T)>::type, S >
 #else
    : public static_inttype_vector_base_base< T, S >
 #endif
{
 #if !defined(USE_OLD_COMPILER)
  public:  typedef typename sizeof_to_uint<sizeof(T)>::type             base_value_type;
  private: typedef static_inttype_vector_base_base<base_value_type,S>   base;
 #else
  public:  typedef T                                    base_value_type;
  private: typedef static_inttype_vector_base_base<T,S> base;
 #endif
public:
    typedef static_inttype_vector_base  this_type;
    typedef T                           value_type;
    typedef typename base::size_type    size_type;
    typedef ptrdiff_t                   difference_type;
    typedef value_type*                 pointer;
    typedef value_type const*           const_pointer;
    typedef value_type*                 iterator;
    typedef value_type const*           const_iterator;
 #if !defined(USE_OLD_COMPILER)
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;
 #endif

    static_inttype_vector_base(size_type capa) noexcept : base(capa) {}
    static_inttype_vector_base(size_type capa, size_type n) noexcept : base(capa, n) {}
    static_inttype_vector_base(static_inttype_vector_base const& r) noexcept : base(r) {}

    pointer             data() noexcept { return pointer(base::data()); }
    const_pointer       data() const noexcept { return const_cast<this_type*>(this)->data(); }

    iterator            begin() noexcept { return iterator(base::begin()); }
    const_iterator      begin() const noexcept { return const_iterator(base::begin()); }
    iterator            end() noexcept { return iterator(base::end()); }
    const_iterator      end() const noexcept { return const_iterator(base::end()); }
    const_iterator      cbegin() const noexcept { return const_iterator(base::begin()); }
    const_iterator      cend() const noexcept { return const_iterator(base::end()); }
 #if !defined(USE_OLD_COMPILER)
    reverse_iterator    rbegin() noexcept { return this->end(); }
    reverse_iterator    rend()   noexcept { return this->begin(); }
    const_reverse_iterator crbegin() const noexcept { return this->end(); }
    const_reverse_iterator crend()   const noexcept { return this->begin(); }
 #endif

    value_type&       operator[](size_type n) noexcept { assert(n < this->size()); return this->data()[n]; }
    value_type const& operator[](size_type n) const noexcept { assert(n < this->size()); return this->data()[n]; }

    value_type&         front() noexcept { return *this->data(); }
    value_type const&   front() const noexcept { return *this->data(); }
    value_type&         back() noexcept { return reinterpret_cast<value_type&>(base::back()); }
    value_type const&   back() const noexcept { return reinterpret_cast<value_type const&>(base::back()); }

    void push_back(value_type b) noexcept { base::push_back(*(base_value_type*)&(b)); }

    this_type& assign(const_pointer first, const_pointer last) noexcept {
        base::assign((base_value_type const*)first, (base_value_type const*)last);
        return *this;
    }

    this_type& assign(size_type n, value_type val) noexcept {
        assign(n, base_value_type(val));
        return *this;
    }

    this_type& insert(pointer pos, const_pointer first, const_pointer last) noexcept {
        base::insert((base_value_type*)pos
                , (base_value_type const*)first, (base_value_type const*)last);
        return *this;
    }

    pointer erase(pointer first, pointer last) noexcept {
        return (iterator)base::erase((base_value_type*)first, (base_value_type*)last);
    }

    void swap(static_inttype_vector_base& rhs) noexcept { base::swap(rhs); }

    bool operator==(this_type const& r) const noexcept { return std::equal(begin(), end(), r.begin(), r.end()); }
    bool operator!=(this_type const& r) const noexcept { return !operator==(r); }

    bool operator< (this_type const& r) const noexcept {
        return std::lexicographical_compare(begin(), end(), r.begin(), r.end());
    }
    bool operator>=(this_type const& r) const noexcept { return !operator<(r); }
    bool operator> (this_type const& r) const noexcept { return r.operator<(*this); }
    bool operator<=(this_type const& r) const noexcept { return !r.operator<(*this); }

 #if !defined(USE_OLD_COMPILER)
    using base::empty;
    using base::size;
    using base::capacity;
    using base::max_size;
    using base::reserve;
    using base::clear;
    using base::resize;
    using base::shrink_to_fit;
    using base::pop_back;
    using base::emplace_back;
    using base::_assgin;
 #endif
};

}   // _priv

#if !defined(USE_OLD_COMPILER)
 #define _STATIC_INTTYPE_VECTOR_BASE(T,N)   \
    _priv::static_inttype_vector_base<T, typename _priv::sizeof_to_uint<(N <= 255 ? 1 : N <= 65535 ? 2 : 4)>::type>
#else
 #define _STATIC_INTTYPE_VECTOR_BASE(T,N)   \
    _priv::static_inttype_vector_base<T, unsigned char>
#endif

template<typename T, unsigned N>
class static_inttype_vector : public _STATIC_INTTYPE_VECTOR_BASE(T,N)
{
private:
    char                                    buf_[N * sizeof(T)];

public:
    typedef _STATIC_INTTYPE_VECTOR_BASE(T,N) base;
    typedef typename base::value_type        base_value_type;
    typedef T                               value_type;
    typedef typename base::size_type        size_type;
    typedef typename base::difference_type  difference_type;
    typedef value_type&                     reference;
    typedef value_type const&               const_reference;
    typedef value_type*                     pointer;
    typedef value_type const*               const_pointer;
    typedef value_type*                     iterator;
    typedef value_type const*               const_iterator;

    static_inttype_vector() noexcept : base(N) {}
    static_inttype_vector(size_type n) noexcept : base(N, n) {}

 #if !defined(USE_OLD_COMPILER)
    template<unsigned L>
    static_inttype_vector(static_inttype_vector<T,L> const& r) noexcept : base(r) {}

    template<unsigned L>
    static_inttype_vector& operator=(static_inttype_vector<T,L> const& r) noexcept {
        base::_assgin(r);
        return *this;
    }

    template<unsigned L>
    void swap(static_inttype_vector<T,L>& rhs) noexcept { base::swap(rhs); }

    template<unsigned L> bool operator==(static_inttype_vector<T,L> const& r) const noexcept { return base::operator==(r); }
    template<unsigned L> bool operator!=(static_inttype_vector<T,L> const& r) const noexcept { return base::operator!=(r); }
    template<unsigned L> bool operator< (static_inttype_vector<T,L> const& r) const noexcept { return base::operator< (r); }
    template<unsigned L> bool operator>=(static_inttype_vector<T,L> const& r) const noexcept { return base::operator>=(r); }
    template<unsigned L> bool operator> (static_inttype_vector<T,L> const& r) const noexcept { return base::operator> (r); }
    template<unsigned L> bool operator<=(static_inttype_vector<T,L> const& r) const noexcept { return base::operator<=(r); }

    typedef std::reverse_iterator<iterator>         reverse_iterator;
    typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

    using base::rbegin;
    using base::rend;
    using base::crbegin;
    using base::crend;

    using base::empty;
    using base::size;
    using base::capacity;
    using base::max_size;
    using base::reserve;
    using base::clear;
    using base::resize;
    using base::shrink_to_fit;
    using base::data;
    using base::begin;
    using base::end;
    using base::cbegin;
    using base::cend;
    using base::operator[];
    using base::front;
    using base::back;
    using base::pop_back;
    using base::push_back;
    using base::emplace_back;
    using base::assign;
    using base::insert;
    using base::erase;
 #else
    static_inttype_vector(static_inttype_vector const& r) noexcept : base(r) {}

    static_inttype_vector& operator=(static_inttype_vector const& r) noexcept {
        base::_assgin(r);
        return *this;
    }

    void swap(static_inttype_vector& rhs) noexcept { base::swap(rhs); }
    bool operator==(static_inttype_vector const& r) const noexcept { return base::operator==(r); }
    bool operator!=(static_inttype_vector const& r) const noexcept { return base::operator!=(r); }
    bool operator< (static_inttype_vector const& r) const noexcept { return base::operator< (r); }
    bool operator>=(static_inttype_vector const& r) const noexcept { return base::operator>=(r); }
    bool operator> (static_inttype_vector const& r) const noexcept { return base::operator> (r); }
    bool operator<=(static_inttype_vector const& r) const noexcept { return base::operator<=(r); }
 #endif

};
#undef _STATIC_INTTYPE_VECTOR_BASE

#endif
