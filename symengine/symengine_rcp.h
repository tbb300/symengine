#ifndef SYMENGINE_RCP_H
#define SYMENGINE_RCP_H

#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <string>

#include <symengine/symengine_config.h>
#include <symengine/symengine_assert.h>


#if defined(WITH_SYMENGINE_RCP)

#else

// Include all Teuchos headers here:
#include "Teuchos_RCP.hpp"
#include "Teuchos_TypeNameTraits.hpp"

#endif

namespace SymEngine {


#if defined(WITH_SYMENGINE_RCP)


/* Ptr */

// Ptr is always pointing to a valid object (can never be nullptr).

template<class T>
class Ptr {
public:
    inline explicit Ptr( T *ptr ) : ptr_(ptr) {
        SYMENGINE_ASSERT(ptr_ != nullptr)
    }
    inline Ptr(const Ptr<T>& ptr) : ptr_(ptr.ptr_) {}
    template<class T2> inline Ptr(const Ptr<T2>& ptr) : ptr_(ptr.get()) {}
    Ptr<T>& operator=(const Ptr<T>& ptr) { ptr_ = ptr.get(); return *this; }
    inline Ptr(Ptr&&) = default;
    Ptr<T>& operator=(Ptr&&) = default;
    inline T* operator->() const { return ptr_; }
    inline T& operator*() const { return *ptr_; }
    inline T* get() const { return ptr_; }
    inline T* getRawPtr() const { return get(); }
    inline const Ptr<T> ptr() const { return *this; }
private:
    T *ptr_;
};

template<typename T> inline
Ptr<T> outArg( T& arg )
{
    return Ptr<T>(&arg);
}

/* RCP */

enum ENull { null };

// RCP can be null. Functionally it should be equivalent to Teuchos::RCP.

template<class T>
class RCP {
public:
    RCP(ENull null_arg = null) : ptr_(nullptr) {}
    explicit RCP(T *p) : ptr_(p) {
        SYMENGINE_ASSERT(ptr_ != nullptr)
        (ptr_->refcount_)++;
    }
    // Copy constructor
    RCP(const RCP<T> &rp) : ptr_(rp.ptr_) {
        if (!is_null()) (ptr_->refcount_)++;
    }
    // Copy constructor
    template<class T2> RCP(const RCP<T2>& r_ptr) : ptr_(r_ptr.get()) {
        if (!is_null()) (ptr_->refcount_)++;
    }
    // Move constructor
    RCP(RCP<T> &&rp) : ptr_(rp.ptr_) {
        rp.ptr_ = nullptr;
    }
    // Move constructor
    template<class T2> RCP(RCP<T2>&& r_ptr) : ptr_(r_ptr.get()) {
        r_ptr._set_null();
    }
    ~RCP() {
        if (ptr_ != nullptr && --(ptr_->refcount_) == 0) delete ptr_;
    }
    T* operator->() const {
        SYMENGINE_ASSERT(ptr_ != nullptr)
        return ptr_;
    }
    T& operator*() const {
        SYMENGINE_ASSERT(ptr_ != nullptr)
        return *ptr_;
    }
    T* get() const { return ptr_; }
    Ptr<T> ptr() const { return Ptr<T>(get()); }
    bool is_null() const { return ptr_ == nullptr; }
    template<class T2> bool operator==(const RCP<T2> &p2) {
        return ptr_ == p2.ptr_;
    }
    template<class T2> bool operator!=(const RCP<T2> &p2) {
        return ptr_ != p2.ptr_;
    }
    // Copy assignment
    RCP<T>& operator=(const RCP<T> &r_ptr) {
        T *r_ptr_ptr_ = r_ptr.ptr_;
        if (!r_ptr.is_null()) (r_ptr_ptr_->refcount_)++;
        if (!is_null() && --(ptr_->refcount_) == 0) delete ptr_;
        ptr_ = r_ptr_ptr_;
        return *this;
    }
    // Move assignment
    RCP<T>& operator=(RCP<T> &&r_ptr) {
        std::swap(ptr_, r_ptr.ptr_);
        return *this;
    }
    void reset() {
        if (!is_null() && --(ptr_->refcount_) == 0) delete ptr_;
        ptr_ = nullptr;
    }
    // Don't use this function directly:
    void _set_null() { ptr_ = nullptr; }
private:
    T *ptr_;
};



template<class T>
inline RCP<T> rcp(T* p)
{
    return RCP<T>(p);
}

template<class T2, class T1>
inline RCP<T2> rcp_static_cast(const RCP<T1>& p1)
{
    // Make the compiler check if the conversion is legal
    T2 *check = static_cast<T2*>(p1.get());
    return RCP<T2>(check);
}

template<class T2, class T1>
inline RCP<T2> rcp_dynamic_cast(const RCP<T1>& p1)
{
    if (!p1.is_null()) {
        T2 *p = nullptr;
        // Make the compiler check if the conversion is legal
        p = dynamic_cast<T2*>(p1.get());
        if (p) {
            return RCP<T2>(p);
        }
    }
    throw std::runtime_error("rcp_dynamic_cast: cannot convert.");
}

template<class T2, class T1>
inline RCP<T2> rcp_const_cast(const RCP<T1>& p1)
{
  // Make the compiler check if the conversion is legal
  T2 *check = const_cast<T2*>(p1.get());
  return RCP<T2>(check);
}


template<class T>
inline bool operator==(const RCP<T> &p, ENull)
{
  return p.get() == nullptr;
}


template<typename T>
std::string typeName(const T &t)
{
    return "RCP<>";
}

void print_stack_on_segfault();


#else

using Teuchos::RCP;
using Teuchos::Ptr;
using Teuchos::outArg;
using Teuchos::rcp;
using Teuchos::rcp_dynamic_cast;
using Teuchos::rcp_static_cast;
using Teuchos::rcp_const_cast;
using Teuchos::typeName;
using Teuchos::null;
using Teuchos::print_stack_on_segfault;

#endif


class EnableRCPFromThis {
// Public interface
public:
    unsigned int use_count() const {
#if defined(WITH_SYMENGINE_RCP)
        return refcount_;
#else
        return weak_self_ptr_.strong_count();
#endif
    }




// Everything below is private interface
public:
#if defined(WITH_SYMENGINE_RCP)

    //! Public variables if defined with SYMENGINE_RCP
    // The reference counter is defined either as "unsigned int" (faster, but
    // not thread safe) or as std::atomic<unsigned int> (slower, but thread
    // safe). Semantically they are almost equivalent, except that the
    // pre-decrement operator `operator--()` returns a copy for std::atomic
    // instead of a reference to itself.
    // The refcount_ is defined as mutable, because it does not change the
    // state of the instance, but changes when more copies
    // of the same instance are made.
#  if defined(WITH_SYMENGINE_THREAD_SAFE)
    mutable std::atomic<unsigned int> refcount_; // reference counter
#  else
    mutable unsigned int refcount_; // reference counter
#  endif // WITH_SYMENGINE_THREAD_SAFE
    EnableRCPFromThis() : refcount_(0) {}

#else
    mutable RCP<const EnableRCPFromThis> weak_self_ptr_;
#endif // WITH_SYMENGINE_RCP

    //! Get RCP<T> pointer to self (it will cast the pointer to T)
    template <class T>
    inline RCP<T> get_rcp_cast() const {
#if defined(WITH_SYMENGINE_RCP)
        return rcp(static_cast<T*>(this));
#else
        return rcp_static_cast<T>(weak_self_ptr_.create_strong());
#endif
    }
};


template<typename T, typename ...Args>
inline RCP<T> make_rcp( Args&& ...args )
{
#if defined(WITH_SYMENGINE_RCP)
    return rcp( new T( std::forward<Args>(args)... ) );
#else
    RCP<T> p = rcp( new T( std::forward<Args>(args)... ) );
    p->weak_self_ptr_ = p.create_weak();
    return p;
#endif
}


} // SymEngine


#endif
