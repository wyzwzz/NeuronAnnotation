//
// Created by wyz on 2021/5/2.
//

#include<type_traits>
#include<iostream>
#include<functional>
int mfunction(const int& number)
{
    std::cout<< number <<std::endl;
    return number;
}
class Test{
    int operator()(int x){
        return 1;
    }
};

template <typename T>
struct Helper;

//template <typename T>
//struct HelperImpl : Helper<decltype( &T::operator() )>
//{
//};
//
//template <typename T>
//struct Helper : HelperImpl<typename std::decay<T>::type>
//{
//};

template <typename Ret, typename Cls, typename... Args>
struct Helper<Ret ( Cls::* )( Args... )>
{
    using return_type = Ret;
    using argument_type = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct Helper<R( Args... )>
{
    using return_type = R;
    using argument_type = std::tuple<Args...>;
};
template <typename R, typename... Args>
struct Helper<R ( *)( Args... )>
{
    using return_type = R;
    using argument_type = std::tuple<Args...>;
};
template <typename Ret, typename Cls, typename... Args>
struct Helper<Ret ( Cls::* )( Args... ) const>
{
    using return_type = Ret;
    using argument_type = std::tuple<Args...>;
};

int main(){
//    Helper<decltype(Test())>::return_type t;
    Helper<decltype(mfunction)>::return_type tt;
//    int x;
//    auto fn=[&x]()->int{
//        return 1;
//    };
//    Helper<decltype(fn)>::return_type tt;

    std::cout<<std::is_same<int, std::invoke_result<decltype(mfunction)*,const int&>::type>::value;
}