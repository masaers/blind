## blind

Late partial binding for templated function objects in C++. There are two interpretations of `blind`: one is that it blindly binds arguments to a function, the other is that idenfitifcation of the version of the fucntion being called happens at call time (late), and `blind` is more readable than `lbind` for late bind. Don't worry if you havn't studied enough linguistics to understand the last one. It has been designed to work as a seamless replacement for `bind` (yes, your `std::placeholders` will work just fine).


### Raison d'être

Let's say we want to use the standard library `sort` function to sort a vector different ways. We can easily define a general purpose comaprison function for reverse sorting using generic lambdas:
```c++
const auto gt = [](auto&& a, auto&& b) { return a > b; };
vector<int> vec{ 1, 2, 3, 4 };
sort(begin(vec), enc(vec), gt);
```
To sort `vec` in the natural order, we just leave out the ordering predicate:
```c++
sort(begin(vec), end(vec));
```

Now, if we wanted to have a function that sorted `vec` given a predicate (or in natural order if no predicate is provided), we might think that we could `bind` the first two arguments of sort to the `begin` and `end` of `vec`:
```c++
auto sort_vec = bind(sort, begin(vec), end(vec));
```
This will, however, not work, since `bind` needs to know the precis function to call, which isn't well defined until we know which sorting predicate we want to use. The precise function isn't know until call time. We can try to work around this using a variadic lambda that forwards its parameters to `sort`, then bind `begin` and `end` of `vec` to this new function, and call the bound function with our custom sorting predicate:
```c++
auto my_sort = [](auto&&... args) { sort(std::forward<decltype(args)>(args)); };
auto sort_vec = bind(my_sort, begin(vec), end(vec));
sort_vec(gt);
```
I can't tell from the standard whether this is supposed to work or not, but both GCC 7 and LLVM chokes on it; the former with a compilation error, and the latter with unexpected behavior (so probably we are moving in undefined territory here). The expectation that this should work, however, is reasonable, although it may not be under the assumptions that `bind` was construed.

This is why we need *late partial binding*, where the idea is that we bind arguments to a named function without checking whether the arguments are valid for that function until it is called (as late as possible). This is where `blind` comes in:
```c++
auto my_sort = [](auto&&... args) { sort(std::forward<decltype(args)>(args)); };
auto sort_vec = blind(my_sort, begin(vec), end(vec));
sort_vec(gt);
```
... will compile (under C++14) and work just as expected. And there is a convenient preprocessor macro to go from `sort` to `my_sort` more easily:
```c++
auto sort_vec = blind(BLIND_FUNC(sort), begin(vec), end(vec));
sort_vec(gt); // vec is now sorted largest to smallest (gt order).
sort_vec();   // vec is now sorted in natural order (smallest to largest).
```
The function `blind` and the preprocessing macro BLIND_FUNC are the main contributions of this single header library.


### Gotchas

Calling `blind` will (just like calling `bind`) store a copy of the bound arguments (and of the function (pointer)). If you need a reference for semantic reasons (for example because the standard streams cannot be copied), or for efficiency reasons (for example bacause passing a constant reference to a huge object is much faster than makign a copy), you can use [`ref`](http://www.cplusplus.com/reference/functional/ref) and [`cref`](http://www.cplusplus.com/reference/functional/cref) to instead store a reference (`_wrapper`, which both `bind` and `blind` can handle).
```c++
// Contrived example
vector<int> vec(10);
auto it = begin(vec);
auto write = blind([](auto& it, int val) { *it++ = val }, ref(it));
for (int i = 0; i < 10; ++i) {
  write(i);
}
// vec now contains the numbers 0 to 9.
// If we hand't bound it by reference, it would contain the number 9 and 9 undefined ints.
```


### Requirements

* C++14
