#pragma once

#include <type_traits>

#include "ECS/ConceptHelpers.h"

#include "ECSwrapper.h"

// GET COMPONENT POINTER
//
template <Component T>
auto GetComponentPtr(const ECSwrapper* ECSwrapper_ptr)
{
    componentID this_component_ID = T::component_ID;
    ComponentBaseClass* this_component_base_class = ECSwrapper_ptr->GetComponentByID(this_component_ID);

    if constexpr (std::is_const<T>::value)
        return std::pair(std::false_type(), static_cast<const T *>(this_component_base_class));
    else
        return std::pair(std::false_type(), static_cast<T *>(this_component_base_class));
}

template <CompEntity T>
auto GetComponentPtr(const ECSwrapper* ECSwrapper_ptr)
{
    componentID this_component_ID = decltype(T::GetComponentType())::component_ID;
    ComponentBaseClass* this_component_base_class = ECSwrapper_ptr->GetComponentByID(this_component_ID);

    if constexpr (std::is_const<T>::value)
        return std::pair(std::true_type(), static_cast<const decltype(T::GetComponentType()) *>(this_component_base_class));
    else
        return std::pair(std::true_type(), static_cast<decltype(T::GetComponentType()) *>(this_component_base_class));
}


// GET TUPLE OF PARAMETERS
//
template <typename front_Arg, typename ...Args>
auto GetComponentPtrsOfArgumentsRecursion(const ECSwrapper* ECSwrapper_ptr)
{
    typedef typename std::remove_pointer<front_Arg>::type front_Arg_no_ptr;
    typedef typename std::remove_reference<front_Arg_no_ptr>::type front_Arg_no_ptr_no_ref;
    auto this_argument = GetComponentPtr<front_Arg_no_ptr_no_ref>(ECSwrapper_ptr);

    if constexpr (sizeof...(Args) > 0)
    {
        auto recursion_result = GetComponentPtrsOfArgumentsRecursion<Args...>(ECSwrapper_ptr);

        return std::tuple_cat(std::make_tuple(this_argument), recursion_result);
    }
    else
    {
        return std::make_tuple(this_argument);
    }
}

template <typename Ret, typename Class, typename ...Args>
auto GetComponentPtrsOfArguments(const ECSwrapper* ECSwrapper_ptr, Ret(Class::* mf)(Args...))
{
    if constexpr (sizeof...(Args) > 0)
    {
        return GetComponentPtrsOfArgumentsRecursion<Args...>(ECSwrapper_ptr);
    }
    else
    {
        return std::tuple<>();
    }    
}


// TRANSLATE COMPONENT PTRS INTO ARGUMENTS
//
template <typename BOOL, typename T>
constexpr auto TranslateComponentPtrToCompEntityRef(const Entity entity, const std::pair<BOOL,T>& pair)
{
    if constexpr (BOOL::value == true)
    {
        return std::ref(pair.second->GetComponentEntity(entity));
    }
    else
    {
        return pair.second;
    }
}

template <unsigned i, typename Tuple>
constexpr auto TranslateComponentPtrsIntoArgumentsRecursion(const Entity entity, const Tuple& tuple)
{
    if constexpr (i < std::tuple_size<Tuple>::value)
    {
        auto recursion_result = TranslateComponentPtrsIntoArgumentsRecursion<i + 1>(entity, tuple);
        auto this_translation = TranslateComponentPtrToCompEntityRef(entity, std::get<i>(tuple));

        return std::tuple_cat(std::make_tuple(this_translation), recursion_result);
    }
    else
    {
        return std::tuple<>();
    }
}

template <typename Tuple>
auto TranslateComponentPtrsIntoArguments(const Entity entity, const Tuple& tuple)
{
    return TranslateComponentPtrsIntoArgumentsRecursion<0>(entity, tuple);
}


// CONCEPT TRIVIAL UPDATE
//
template <typename T>
concept Updateable = requires() {&T::Update;} && CompEntity<T>;

template <typename front_Arg, typename ...Args>
constexpr bool AreArgumentsTrivial()
{
    if constexpr (not TrivialUpdateParameter<front_Arg>)
    {
        return false;
    }
    else if constexpr (sizeof...(Args) > 0)
    {
        return AreArgumentsTrivial<Args...>();
    }
    else
    {
        return true;
    }
} 

template <typename Ret, typename Class, typename ...Args>
constexpr bool IsMethodTrivialUpdateable(Ret(Class::* mf)(Args...))
{
    if constexpr (sizeof...(Args) > 0)
    {
        return AreArgumentsTrivial<Args...>();
    }
    else
    {
        return true;
    }
}

template <typename T>
concept TrivialUpdatable = Updateable<T> && IsMethodTrivialUpdateable(&T::Update);







