
# Quickproto

This repo contains the source code for a simple toy programming language written in C++23. The goal of this project is to study and understand how Sea of Nodes (SoN) can be used as an alternative to SSA for intermediate representation. Compilation goes through the following steps:

```cpp
Source code > Lexing > (Parsing + SoN generation + optimizing SoN) > Dead code elimination
```