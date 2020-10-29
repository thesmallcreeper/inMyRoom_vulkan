#pragma once

#include <concepts>

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/CompEntityBaseWrappedClass.h"


// CONCEPTS OF TYPES
//
template <class T>
concept Component = std::is_base_of<ComponentBaseClass, T>::value;

template <class T>
concept CompEntity = std::is_base_of<CompEntityBaseClass, T>::value;

template <class T>
concept TrivialUpdateParameter = Component<typename std::remove_pointer<T>::type> || CompEntity<typename std::remove_reference<T>::type>;

template <class T>
concept HasInitMethod = CompEntity<T> && requires (T comp_entity){comp_entity.Init();};