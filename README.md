# Ibex Programming Language

Ibex is a modern, systems-level programming language designed to provide explicit control, high performance, and strong typing without the traditional object-oriented bloat. It is built to be a robust alternative to C and C++, focusing on modern architectures, compile-time metaprogramming, and strict structural enforcement.

## Project Goals
- **Systems-Level Control**: Manual memory management, pointers, and explicit data layout.
- **Modern Semantics**: Built-in slices, references, and a clean namespace-driven module system.
- **Zero OOP Bloat**: No `class`, `public`, `private`, or inheritance hierarchies. Structs, enums, and functions form the core.
- **Compile-Time Power**: Function bindings, strong type aliases, and a rich attribute system `[[attr]]` for custom compilation behaviors.

## Language Feature Overview
Ibex features C-like syntax with modernized declarations and a robust type system. 

### Basic Example
```ibex
package math {
    [[strong]] using Unit = f32;

    export const PI := 3.14159;

    export calculate_area : (radius: Unit) -> Unit {
        return PI * radius * radius;
    }
}

module geometry {
    export package math;
}
```

```ibex
// main.ibex
import geometry.math as m;

main : () -> i32 {
    const rad := 10.0 as m.Unit;
    const area := m.calculate_area(rad);
    return 0;
}
```
