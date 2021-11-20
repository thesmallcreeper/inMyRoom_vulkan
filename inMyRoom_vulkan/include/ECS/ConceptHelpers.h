#pragma once

#include <concepts>
#include <chrono>

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/CompEntityBaseWrappedClass.h"


// CONCEPTS OF TYPES
//
template <class T>
concept Component = std::is_base_of<ComponentBaseClass, T>::value;

template <class T>
concept CompEntity = std::is_base_of<CompEntityBaseClass, T>::value;

template <class T>
concept DeltaTime = std::is_same<std::chrono::duration<float>, T>::value;

template <class T>
concept EntitiesHandlerType = std::is_same<class EntitiesHandler, T>::value;

template <class T>
concept TrivialUpdateParameter = Component<typename std::remove_pointer<T>::type>
        || CompEntity<typename std::remove_reference<T>::type>
        || DeltaTime<T>
        || EntitiesHandlerType<typename std::remove_pointer<T>::type>;
