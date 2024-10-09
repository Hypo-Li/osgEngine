#pragma once
#include "Reflection.h"
#include "Metadata.h"

#include <array>
#include <vector>
#include <functional>

namespace xxx::refl
{
    // member function traits
    template <typename T>
    struct member_function_traits;

    template <typename Class, typename Return, typename... Args>
    struct member_function_traits<Return(Class::*)(Args...)> {
        using class_type = Class;
        using return_type = Return;
        using args_type = std::tuple<Args...>;
        static constexpr size_t args_count = std::tuple_size_v<args_type>;
        static constexpr bool is_const = false;
    };

    template <typename Class, typename Return, typename... Args>
    struct member_function_traits<Return(Class::*)(Args...) const> {
        using class_type = Class;
        using return_type = Return;
        using args_type = std::tuple<Args...>;
        static constexpr size_t args_count = std::tuple_size_v<args_type>;
        static constexpr bool is_const = true;
    };

    class Method : public MetadataBase
    {
    public:
        Method(std::string_view name) : mName(name) {}
        virtual ~Method() = default;

        std::string_view getName() const
        {
            return mName;
        }

        virtual Type* getReturnType() const = 0;
        virtual Type* getOwnerType() const = 0;
        virtual std::string_view getParameterNameByIndex(uint32_t index) const = 0;
        virtual Type* getParameterTypeByIndex(uint32_t index) const = 0;
        virtual Type* getParameterTypeByName(std::string_view name) const = 0;

        /*template <typename... Args>
        inline Any invoke(void* instance, Args&&... args) const
        {
            if constexpr (sizeof...(args) <= 6)
                return invokeImpl(instance, args...);
            else
                return invokeImpl(instance, std::vector<Argument>{(args)...});
        }

        template <typename... Args>
        inline Any invoke(const void* instance, Args&&... args) const
        {
            if constexpr (sizeof...(args) <= 6)
                return invokeImpl(instance, args...);
            else
                return invokeImpl(instance, std::vector<Argument>{(args)...});
        }*/

        template <typename... Args>
        inline Any invoke(void* instance, Args&&... args) const
        {
            return invokeImpl(instance, std::vector<void*>{(&args)...});
        }

    protected:
        /*virtual Any invokeImpl(void* instance) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5) const = 0;
        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5, Argument arg6) const = 0;
        virtual Any invokeImpl(void* instance, const std::vector<Argument>& args) const = 0;

        virtual Any invokeImpl(const void* instance) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5) const = 0;
        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5, Argument arg6) const = 0;
        virtual Any invokeImpl(const void* instance, const std::vector<Argument>& args) const = 0;*/

        virtual Any invokeImpl(void* instance, std::vector<void*> args) const = 0;

    protected:
        std::string_view mName;
    };

    template <typename T, typename = std::enable_if_t<std::is_member_function_pointer_v<T>>>
    class MethodInstance : public Method
    {
        using ClassType = typename member_function_traits<T>::class_type;
        using ReturnType = typename member_function_traits<T>::return_type;
        using ArgsType = typename member_function_traits<T>::args_type;
        template <size_t I>
        using ArgType = typename std::tuple_element_t<I, ArgsType>;
        static constexpr size_t ArgsCount = member_function_traits<T>::args_count;
        static constexpr bool IsConst = member_function_traits<T>::is_const;

        template <int64_t Index, typename T>
        void setParameterType()
        {
            if constexpr (Index >= 0)
            {
                mParameters[Index].second = Reflection::getType<ArgType<Index>>();
                setParameterType<Index - 1, T>();
            }
        }
    public:
        MethodInstance(std::string_view name, T method, std::initializer_list<std::string_view> paramNames) :
            Method(name), mMethod(method)
        {
            setParameterType<ArgsCount - 1, ArgsType>();
            size_t i = 0;
            for (auto paramName : paramNames)
                mParameters[i++].first = paramName;
        }
        virtual ~MethodInstance() = default;

        virtual Type* getReturnType() const override
        {
            return Reflection::getType<ReturnType>();
        }

        virtual Type* getOwnerType() const override
        {
            return Reflection::getType<ClassType>();
        }

        virtual std::string_view getParameterNameByIndex(uint32_t index) const override
        {
            return mParameters.at(index).first;
        }

        virtual Type* getParameterTypeByIndex(uint32_t index) const override
        {
            return mParameters.at(index).second;
        }

        virtual Type* getParameterTypeByName(std::string_view name) const override
        {
            for (auto& parameter : mParameters)
            {
                if (parameter.first == name)
                    return parameter.second;
            }
            return nullptr;
        }

    protected:
        /*virtual Any invokeImpl(void* instance) const override
        {
            return invokeMethod(std::make_index_sequence<0>{}, instance);
        }

        virtual Any invokeImpl(void* instance, Argument arg1) const override
        {
            return invokeMethod(std::make_index_sequence<1>{}, instance, arg1);
        }

        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2) const override
        {
            return invokeMethod(std::make_index_sequence<2>{}, instance, arg1, arg2);
        }

        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3) const override
        {
            return invokeMethod(std::make_index_sequence<3>{}, instance, arg1, arg2, arg3);
        }

        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4) const override
        {
            return invokeMethod(std::make_index_sequence<4>{}, instance, arg1, arg2, arg3, arg4);
        }

        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5) const override
        {
            return invokeMethod(std::make_index_sequence<5>{}, instance, arg1, arg2, arg3, arg4, arg5);
        }

        virtual Any invokeImpl(void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5, Argument arg6) const override
        {
            return invokeMethod(std::make_index_sequence<6>{}, instance, arg1, arg2, arg3, arg4, arg5, arg6);
        }

        virtual Any invokeImpl(void* instance, const std::vector<Argument>& args) const override
        {
            return invokeMethod(std::make_index_sequence<ArgsCount>{}, instance, args);
        }*/

        virtual Any invokeImpl(void* instance, std::vector<void*> args) const override
        {
            return invokeMethod(std::make_index_sequence<ArgsCount>{}, instance, args);
        }

        /*virtual Any invokeImpl(const void* instance) const override
        {
            return invokeMethod(std::make_index_sequence<0>{}, instance);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1) const override
        {
            return invokeMethod(std::make_index_sequence<1>{}, instance, arg1);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2) const override
        {
            return invokeMethod(std::make_index_sequence<2>{}, instance, arg1, arg2);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3) const override
        {
            return invokeMethod(std::make_index_sequence<3>{}, instance, arg1, arg2, arg3);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4) const override
        {
            return invokeMethod(std::make_index_sequence<4>{}, instance, arg1, arg2, arg3, arg4);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5) const override
        {
            return invokeMethod(std::make_index_sequence<5>{}, instance, arg1, arg2, arg3, arg4, arg5);
        }

        virtual Any invokeImpl(const void* instance, Argument arg1, Argument arg2, Argument arg3, Argument arg4, Argument arg5, Argument arg6) const override
        {
            return invokeMethod(std::make_index_sequence<6>{}, instance, arg1, arg2, arg3, arg4, arg5, arg6);
        }

        virtual Any invokeImpl(const void* instance, const std::vector<Argument>& args) const override
        {
            return invokeMethod(std::make_index_sequence<ArgsCount>{}, instance, args);
        }*/

    protected:
        T mMethod;
        std::array<std::pair<std::string_view, Type*>, ArgsCount> mParameters;

        /*template <std::size_t... Indices, typename... Args>
        Any invokeMethod(std::index_sequence<Indices...>, void* instance, const Args&... args) const
        {
            if constexpr (ArgsCount != sizeof...(args))
            {
                return Any{};
            }
            else if constexpr (std::is_same_v<ReturnType, void>)
            {
                (static_cast<ClassType*>(instance)->*mMethod)((args.getValue<ArgType<Indices>>())...);
                return Any{};
            }
            else
            {
                return (static_cast<ClassType*>(instance)->*mMethod)((args.getValue<ArgType<Indices>>())...);
            }
        }

        template <std::size_t... Indices>
        Any invokeMethod(std::index_sequence<Indices...>, void* instance, const std::vector<Argument>& args) const
        {
            if (ArgsCount != args.size())
            {
                return Any{};
            }
            else
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    (static_cast<ClassType*>(instance)->*mMethod)((args[Indices].getValue<ArgType<Indices>>())...);
                    return Any{};
                }
                else
                {
                    return (static_cast<ClassType*>(instance)->*mMethod)((args[Indices].getValue<ArgType<Indices>>())...);
                }
            }
        }*/

        template <std::size_t... Indices>
        Any invokeMethod(std::index_sequence<Indices...>, void* instance, const std::vector<void*>& args) const
        {
            if (ArgsCount != args.size())
            {
                return Any{};
            }
            else
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    (static_cast<ClassType*>(instance)->*mMethod)((*static_cast<std::remove_reference_t<ArgType<Indices>>*>(args[Indices]))...);
                    return Any{};
                }
                else
                {
                    return (static_cast<ClassType*>(instance)->*mMethod)((*static_cast<std::remove_reference_t<ArgType<Indices>>*>(args[Indices]))...);
                }
            }
        }

        //template <std::size_t... Indices, typename... Args>
        //Any invokeMethod(std::index_sequence<Indices...>, const void* instance, const Args&... args) const
        //{
        //    if constexpr (IsConst)
        //    {
        //        if constexpr (ArgsCount != sizeof...(args))
        //        {
        //            return Any{};
        //        }
        //        else if constexpr (std::is_same_v<ReturnType, void>)
        //        {
        //            (static_cast<const ClassType*>(instance)->*mMethod)((args.getValue<ArgType<Indices>>())...);
        //            return Any{};
        //        }
        //        else
        //        {
        //            return (static_cast<const ClassType*>(instance)->*mMethod)((args.getValue<ArgType<Indices>>())...);
        //        }
        //    }
        //    else
        //    {
        //        // a const instance try to call a non-const method.
        //        throw std::bad_function_call{};
        //    }
        //}

        //template <std::size_t... Indices>
        //Any invokeMethod(std::index_sequence<Indices...>, const void* instance, const std::vector<Argument>& args) const
        //{
        //    if constexpr (IsConst)
        //    {
        //        if (ArgsCount != args.size())
        //        {
        //            return Any{};
        //        }
        //        else
        //        {
        //            if constexpr (std::is_same_v<ReturnType, void>)
        //            {
        //                (static_cast<const ClassType*>(instance)->*mMethod)((args[Indices].getValue<ArgType<Indices>>())...);
        //                return Any{};
        //            }
        //            else
        //            {
        //                return (static_cast<const ClassType*>(instance)->*mMethod)((args[Indices].getValue<ArgType<Indices>>())...);
        //            }
        //        }
        //    }
        //    else
        //    {
        //        // a const instance try to call a non-const method.
        //        throw std::bad_function_call{};
        //    }
        //}
    };
}
