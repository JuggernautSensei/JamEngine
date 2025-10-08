#pragma once
#include "StringUtilities.h"

namespace jam::class_db
{

// 클래스 등록
template<typename ClassT>
static void ReflectClass(const std::string_view _classTag)
{
    entt::meta<ClassT>().type(HashOf(_classTag));
}

// 메서드 등록
template<typename ClassT, auto Func, typename Policy = entt::as_is_t>
static void ReflectMethod(const std::string_view _methodTag)
{
    entt::meta<ClassT>().template func<Func, Policy>(HashOf(_methodTag));
}

// 생성자 등록 (매개변수 타입 기반)
template<typename ClassT, typename... Args>
static void ReflectConstructor()
{
    entt::meta<ClassT>().template ctor<Args...>();
}

// 데이터 멤버/변수 등록
template<typename ClassT, auto Data, typename Policy = entt::as_is_t>
static void ReflectData(const std::string_view _dataTag)
{
    entt::meta<ClassT>().template data<Data, Policy>(HashOf(_dataTag));
}

// 객체 생성
static entt::meta_any CreateClass(
    const std::string_view                _classTag,
    const std::span<const entt::meta_any> _args = {})
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        return type.construct(_args.data(), _args.size());
    }
    return {};
}

// 모든 등록된 타입들 획득
static decltype(auto) GetReflectTypes()
{
    return entt::resolve();
}

// 메서드 호출
static entt::meta_any InvokeMethod(
    entt::meta_any&                       _instance,
    const std::string_view                _classTag,
    const std::string_view                _methodTag,
    const std::span<const entt::meta_any> _args = {})
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        entt::meta_func func = type.func(HashOf(_methodTag));
        if (func)
        {
            return func.invoke(_instance, _args.data(), _args.size());
        }
    }
    return {};
}

// 메서드 객체 가져오기
static entt::meta_func GetMethod(
    const std::string_view _classTag,
    const std::string_view _methodTag)
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        return type.func(HashOf(_methodTag));
    }
    return {};
}

// 데이터 멤버 접근 (getter)
static entt::meta_any GetData(
    entt::meta_any&        _instance,
    const std::string_view _classTag,
    const std::string_view _dataTag)
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        entt::meta_data data = type.data(HashOf(_dataTag));
        if (data)
        {
            return data.get(_instance);
        }
    }
    return {};
}

// 데이터 멤버 설정 (setter)
static bool SetData(
    entt::meta_any&        _instance,
    const std::string_view _classTag,
    const std::string_view _dataTag,
    entt::meta_any&        _value)
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        entt::meta_data data = type.data(HashOf(_dataTag));
        if (data)
        {
            return data.set(_instance, _value);
        }
    }
    return false;
}

static bool IsClassReflected(const std::string_view _classTag)
{
    return static_cast<bool>(entt::resolve(HashOf(_classTag)));
}

static bool IsMethodReflected(const std::string_view _classTag, const std::string_view _methodTag)
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        return static_cast<bool>(type.func(HashOf(_methodTag)));
    }
    return false;
}

static bool IsDataReflected(const std::string_view _classTag, const std::string_view _dataTag)
{
    entt::meta_type type = entt::resolve(HashOf(_classTag));
    if (type)
    {
        return static_cast<bool>(type.data(HashOf(_dataTag)));
    }
    return false;
}

}   // namespace jam::class_db