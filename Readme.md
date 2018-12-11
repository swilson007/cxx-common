# C++-common
Just some common C++ stuff used in misc projects

## Design Constraints 
* C++14
  - Header only. Well maybe... I'm not happy about having OS includes in any header.
  - Even though it's 2019/2018, C++17 isn't fully there yet as of this writing. (Esp MacOS)
  - Consider... it may just be STL, so possibly allow --std=c++17, but restrict certain STL 
    problem-children like 'filesystem' class
  
## C++ Style / Best Practices
* Full-warnings enabled from day 1. -Wall, -pedantic, -Wextra, etc. 
* clang-lint enabled builds
* clang-format using google style with minor mods 
* Using google style / practices as basis
* gtest from day 1. Unit tests where it makes sense
* Naming
  - Variables: lowerCamelCase (per-google & swilson)
  - Member Vars: camelCase_ (per-google)
  - Struct Variables (public): lower_snake_case (per-google)
  - Enum Values: kHello: kWorld (per-google)
  - Constants: kCamelCase: kPi (per-google)
  - Globals constants: kCamelCase (per-google)
  - Static constants: kCamelCase (per-google)
  - Globals (non constants): gCamelCase: gHello (per-swilson)
  - Statics (non constants): sCamelCase: sIndex (per-swilson)
  - Types: UpperCamelCase (per-google)
  - Macros: UPPER_SNAKE_CASE (per-google)
  - Functions: lowerCamelCase (per-google)
  - Namespaces: lower_snake_case (per-google)