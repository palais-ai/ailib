#ifndef ANY_H
#define ANY_H

#pragma once

#include "ai_global.h"

BEGIN_NS_AILIB

template<class T> struct remove_reference
{
    typedef T type;
};

template<class T> struct remove_reference<T&>
{
    typedef T type;
};

// For C++11 builds.
template<class T> struct remove_reference<T&&>
{
    typedef T type;
};

namespace detail
{
//
// 03/26/2015 PS: Removed streaming operator support.
// 03/16/2015 PS: Added support for comparison operators (operator==, operator!=).
//
// CREDITS: Based on: boost::detail::sp_typeinfo
// ORIGINAL LICENSE:
//
//  detail/sp_typeinfo.hpp
//
//  Copyright 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
class sp_typeinfo
{
private:

    sp_typeinfo( sp_typeinfo const& );
    sp_typeinfo& operator=( sp_typeinfo const& );

    char const * name_;

public:

    explicit sp_typeinfo( char const * name ): name_( name )
    {
    }

    bool operator==( sp_typeinfo const& rhs ) const
    {
        return this == &rhs;
    }

    bool operator!=( sp_typeinfo const& rhs ) const
    {
        return this != &rhs;
    }

    bool before( sp_typeinfo const& rhs ) const
    {
        return std::less< sp_typeinfo const* >()( this, &rhs );
    }

    char const* name() const
    {
        return name_;
    }
};

template<class T> struct sp_typeid_
{
    static sp_typeinfo ti_;

    static char const * name()
    {
        return __func__;
    }
};

template<class T> sp_typeinfo sp_typeid_< T >::ti_(sp_typeid_< T >::name());

template<class T> struct sp_typeid_< T & >: sp_typeid_< T >
{
};

template<class T> struct sp_typeid_< T const >: sp_typeid_< T >
{
};

template<class T> struct sp_typeid_< T volatile >: sp_typeid_< T >
{
};

template<class T> struct sp_typeid_< T const volatile >: sp_typeid_< T >
{
};

#define AI_SP_TYPEID(T) (ailib::detail::sp_typeid_<T>::ti_)

} // NAMESPACE DETAIL
END_NS_AILIB

#include <stdexcept>
#include <algorithm>
#include <iosfwd>

BEGIN_NS_AILIB

namespace detail
{
// CREDITS: Based on boost::ailib::detail::bool_ (bool.hpp and bool_fwd.hpp)
// ORIGINAL LICENSE:
//
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
template< bool C_ > struct bool_
{
    const static bool value = C_;

    typedef bool_ type;
    typedef bool value_type;
    operator bool() const
    {
        return this->value;
    }
};

// shorcuts
typedef bool_<true> true_;
typedef bool_<false> false_;
}

/*
   CREDITS: Based on: boost::spirit::hold_any
   ORIGINAL LICENSE:
   =============================================================================
    Copyright (c) 2007-2011 Hartmut Kaiser
    Copyright (c) Christopher Diggins 2005
    Copyright (c) Pablo Aguilar 2005
    Copyright (c) Kevlin Henney 2001

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    The class boost::spirit::hold_any is built based on the any class
    published here: http://www.codeproject.com/cpp/dynamic_typing.asp. It adds
    support for std streaming operator<<() and operator>>().
    ==============================================================================
*/

    typedef ailib::detail::sp_typeinfo local_typeinfo;
    typedef ailib::detail::true_ local_true;
    typedef ailib::detail::false_ local_false;

    class bad_any_cast : public std::exception
    {
    public:
        bad_any_cast(local_typeinfo const& src,
                     local_typeinfo const& dest) :
            from(src.name()),
            to(dest.name())
        {
            ;
        }

        virtual const char* what() const throw()
        {
            return "bad any cast";
        }

        const char* from;
        const char* to;
    };

    namespace detail
    {
        // function pointer table
        template <typename Char>
        struct fxn_ptr_table
        {
            local_typeinfo const& (*get_type)();
            void (*static_delete)(void**);
            void (*destruct)(void**);
            void (*clone)(void* const*, void**);
            void (*move)(void* const*, void**);
            bool (*equals)(void* const*, void* const*);
        };

        // static functions for small value-types
        template <typename Small>
        struct fxns;

        template <>
        struct fxns<local_true>
        {
            template<typename T, typename Char>
            struct type
            {
                static local_typeinfo const& get_type()
                {
                    return AI_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void destruct(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    new (dest) T(*reinterpret_cast<T const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    reinterpret_cast<T*>(dest)->~T();
                    *reinterpret_cast<T*>(dest) =
                        *reinterpret_cast<T const*>(src);
                }
                static bool equals(void* const* lv, void* const* rv)
                {
                    return *reinterpret_cast<T const*>(lv) == *reinterpret_cast<T const*>(rv);
                }
            };
        };

        // static functions for big value-types (bigger than a void*)
        template <>
        struct fxns<local_false>
        {
            template<typename T, typename Char>
            struct type
            {
                static local_typeinfo const& get_type()
                {
                    return AI_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    // destruct and free memory
                    delete (*reinterpret_cast<T**>(x));
                }
                static void destruct(void** x)
                {
                    // destruct only, we'll reuse memory
                    (*reinterpret_cast<T**>(x))->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    *dest = new T(**reinterpret_cast<T* const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    (*reinterpret_cast<T**>(dest))->~T();
                    **reinterpret_cast<T**>(dest) =
                        **reinterpret_cast<T* const*>(src);
                }
                static bool equals(void* const* lv, void* const* rv)
                {
                    return **reinterpret_cast<T* const*>(lv) == **reinterpret_cast<T* const*>(rv);
                }
            };
        };

        template <typename T>
        struct get_table
        {
            typedef ailib::detail::bool_<(sizeof(T) <= sizeof(void*))> is_small;

            template <typename Char>
            static fxn_ptr_table<Char>* get()
            {
                static fxn_ptr_table<Char> static_table =
                {
                    fxns<is_small>::template type<T, Char>::get_type,
                    fxns<is_small>::template type<T, Char>::static_delete,
                    fxns<is_small>::template type<T, Char>::destruct,
                    fxns<is_small>::template type<T, Char>::clone,
                    fxns<is_small>::template type<T, Char>::move,
                    fxns<is_small>::template type<T, Char>::equals
                };
                return &static_table;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        struct empty {
            inline bool operator==(const empty& other) const
            {
                UNUSED(other);
                return true;
            }
        };

        typedef ailib::detail::empty local_empty;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    class basic_hold_any
    {
    public:
        // constructors
        template <typename T>
        explicit basic_hold_any(T const& x) :
            table(ailib::detail::get_table<T>::template get<Char>()), object(0)
        {
            if (ailib::detail::get_table<T>::is_small::value)
                new (&object) T(x);
            else
                object = new T(x);
        }

        basic_hold_any()
          : table(ailib::detail::get_table<detail::local_empty>::template get<Char>()),
            object(0)
        {
        }

        basic_hold_any(basic_hold_any const& x)
          : table(ailib::detail::get_table<detail::local_empty>::template get<Char>()),
            object(0)
        {
            assign(x);
        }

        ~basic_hold_any()
        {
            table->static_delete(&object);
        }

        // assignment
        basic_hold_any& assign(basic_hold_any const& x)
        {
            if (&x != this) {
                // are we copying between the same type?
                if (table == x.table) {
                    // if so, we can avoid reallocation
                    table->move(&x.object, &object);
                }
                else {
                    reset();
                    x.table->clone(&x.object, &object);
                    table = x.table;
                }
            }
            return *this;
        }

        template <typename T>
        basic_hold_any& assign(T const& x)
        {
            // are we copying between the same type?
            ailib::detail::fxn_ptr_table<Char>* x_table =
                ailib::detail::get_table<T>::template get<Char>();
            if (table == x_table) {
            // if so, we can avoid deallocating and re-use memory
                table->destruct(&object);    // first destruct the old content
                if (ailib::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    new (&object) T(x);
                }
                else {
                    // create copy on-top of old version
                    new (object) T(x);
                }
            }
            else {
                if (ailib::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    table->destruct(&object); // first destruct the old content
                    new (&object) T(x);
                }
                else {
                    reset();                  // first delete the old content
                    object = new T(x);
                }
                table = x_table;      // update table pointer
            }
            return *this;
        }

        // assignment operator
        template <typename T>
        basic_hold_any& operator=(T const& x)
        {
            return assign(x);
        }

        // utility functions
        basic_hold_any& swap(basic_hold_any& x)
        {
            std::swap(table, x.table);
            std::swap(object, x.object);
            return *this;
        }

        local_typeinfo const& type() const
        {
            return table->get_type();
        }

        template <typename T>
        T const& cast() const
        {
            if (type() != AI_SP_TYPEID(T))
              throw bad_any_cast(type(), AI_SP_TYPEID(T));

            return ailib::detail::get_table<T>::is_small::value ?
                *reinterpret_cast<T const*>(&object) :
                *reinterpret_cast<T const*>(object);
        }

        // automatic casting operator
        template <typename T>
        operator T const& () const { return cast<T>(); }

        bool empty() const
        {
            return table == ailib::detail::get_table<detail::local_empty>::template get<Char>();
        }

        void reset()
        {
            if (!empty())
            {
                table->static_delete(&object);
                table = ailib::detail::get_table<detail::local_empty>::template get<Char>();
                object = 0;
            }
        }

        template <typename Char_>
        inline bool
        operator==(basic_hold_any<Char_> const& other) const
        {
            return type() == other.type() && table->equals(&object, &other.object);
        }

        template <typename Char_>
        inline bool
        operator!=(basic_hold_any<Char_> const& other) const
        {
            return !(*this == other);
        }

    private: // types
        template <typename T, typename Char_>
        friend T* any_cast(basic_hold_any<Char_> *);
#if FALSE
    public: // types (public so any_cast can be non-friend)
#endif
        // fields
        ailib::detail::fxn_ptr_table<Char>* table;
        void* object;
    };

    // boost::any-like casting
    template <typename T, typename Char>
    inline T* any_cast (basic_hold_any<Char>* operand)
    {
        if (operand && operand->type() == AI_SP_TYPEID(T)) {
            return ailib::detail::get_table<T>::is_small::value ?
                reinterpret_cast<T*>(&operand->object) :
                reinterpret_cast<T*>(operand->object);
        }
        return 0;
    }

    template <typename T, typename Char>
    inline T const* any_cast(basic_hold_any<Char> const* operand)
    {
        return any_cast<T>(const_cast<basic_hold_any<Char>*>(operand));
    }

    template <typename T, typename Char>
    T any_cast(basic_hold_any<Char>& operand)
    {
        typedef typename remove_reference<T>::type nonref;

        nonref* result = any_cast<nonref>(&operand);
        if(!result)
        {
            throw bad_any_cast(operand.type(), AI_SP_TYPEID(T));
        }
        return *result;
    }

    template <typename T, typename Char>
    T const& any_cast(basic_hold_any<Char> const& operand)
    {
        typedef typename remove_reference<T>::type nonref;

        return any_cast<nonref const&>(const_cast<basic_hold_any<Char> &>(operand));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // backwards compatibility
    typedef basic_hold_any<char> hold_any;
    typedef basic_hold_any<wchar_t> whold_any;

    namespace traits
    {
        template <typename T>
        struct is_hold_any : local_false {};

        template <typename Char>
        struct is_hold_any<basic_hold_any<Char> > : local_true {};
    }

END_NS_AILIB

#endif // ANY_H
