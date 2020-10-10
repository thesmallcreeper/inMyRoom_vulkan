#pragma once

#include <concepts>
#include <type_traits>


#include "ECSwrapper.h"
#include "is_template_base_of.h"

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/CompEntityBase.h"

//
// CONCEPTS
template <class T>
concept Component = is_template_base_of<ComponentBaseWrappedClass, T>::value;

template <class T>
concept CompEntity = is_template_base_of<CompEntityBase, T>::value;

//
// GET COMPONENT POINTER
template <Component T>
auto GetComponentPtr(ECSwrapper* ECSwrapper_ptr)
{
    componentID this_component_ID = T::component_ID;
    ComponentBaseClass* this_component_base_class = ECSwrapper_ptr->GetComponentByID(this_component_ID);

    if constexpr (std::is_const<T>::value)
        return static_cast<const T *>(this_component_base_class);
    else
        return static_cast<T *>(this_component_base_class);
}

template <CompEntity T>
auto GetComponentPtr(ECSwrapper* ECSwrapper_ptr)
{
    if constexpr (std::is_const<T>::value)
        return GetComponentPtr<const decltype(T::GetComponentType())>(ECSwrapper_ptr);
    else
        return GetComponentPtr<decltype(T::GetComponentType())>(ECSwrapper_ptr);   
}

template <typename T>
auto GetComponentPtr(ECSwrapper* ECSwrapper_ptr)
{
    typedef typename std::remove_pointer<T>::type T_no_ptr;
    typedef typename std::remove_reference<T_no_ptr>::type T_no_ptr_no_ref;
    return GetComponentPtr<T_no_ptr_no_ref>(ECSwrapper_ptr);
}

//
// GET TUPLE OF PARAMETERS
inline auto GetTupleOfParametersRecursion(ECSwrapper* ECSwrapper_ptr)
{
    return std::tuple<>();
}

template <typename Dest, typename front_Arg, typename ...Args>
auto GetTupleOfParametersRecursion(ECSwrapper* ECSwrapper_ptr)
{
    auto recursion_result = GetTupleOfParametersRecursion<Dest, Args...>(ECSwrapper_ptr);

    auto this_parameter = GetComponentPtr<front_Arg>(ECSwrapper_ptr);
    return std::tuple_cat(std::tie(this_parameter), recursion_result);
}

template <typename Dest, typename Ret, typename Class, typename ...Args>
auto GetTupleOfParameters(ECSwrapper* ECSwrapper_ptr, Ret(Class::* mf)(Args...))
{
    return GetTupleOfParametersRecursion<Dest, Args...>(ECSwrapper_ptr);
}
