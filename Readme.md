# C++-common
Just some common C++ stuff used in misc projects

## Design Constraints 
* C++14
  - Header only. Well maybe... I'm not happy about having OS includes in any header.
  - Even though it's 2019/2018, C++17 isn't fully there yet as of this writing. (Esp MacOS)
  
## C++ Style / Best Practices
* Full-warnings enabled from day 1. -Wall, -pedantic, -Wextra, etc. 
* clang-lint enabled builds
* clang-format using google style with minor mods 
* gtest from day 1. Unit tests where it makes sense
* Naming
  - Variables: lowerCamelCase (per-google & swilson)
  - Member Vars: `camelCase_` (per-google)  Note to self - switch this to the more 
    visually appealing `_camelCase`. Having an underscore mashed with other operators 
    like . or -> doesn't provide a clear enough visual cue. 
    Thus `foo_->bar()` vs `_foo->bar()` just looks cleaner.
    And no, it's a not a language standard violation!
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
  - No Hungarian. But, storage class prefix/suffixes where indicated 
