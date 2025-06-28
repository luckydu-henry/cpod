# CPOD
cpod is a very C++-like "serialization library" (it can also be a configuration library).
should be pronounced as 'see-pod'.

# FEATURES
It aims to be very fast and efficient and can combined deeply with C++ code.

Now this is just the first stable version, and this version can only (de)serialize C++ STL and basic types, but I think it's already enough to use it in an actual program and so here it is.

It supports both 'text' and 'binary' form of serialization and can easily convert them. It uses C++ code to describe serialized data (I think this is the coolest part) and thanks to C++'s static type system, deserialization can be exteremely fast and simple (I haven't done benchmark yet but it should be faster than JSON theriotically).

Also, this library is very small so you can modify the source code or the serialized binary file to suit your own special needs.

To summarize, **this library is 'small flexible yet very fast'**.

# HOW TO USE
I have provided three 'tests' to show you how to work with it and this library itself is header only and contains only ~1000 lines of codes.
To use it just simply put 'cpod.hpp' into your own project and enable C++20 compiler support and you are ready to go.

# LICENSE
MIT license, check *LICENSE* and each source file for more details.