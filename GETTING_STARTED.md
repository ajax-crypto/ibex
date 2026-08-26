# Getting Started - Next 24 Hours

This document tells you exactly what to do in the next 24 hours with this project.

---

## ⏱️ Hour 1: Quick Overview (15 minutes)

### What to read:
1. Open and read `README.md` (5 minutes)
   - Understand what Ibex is
   - See basic syntax examples
   
2. Skim `PROJECT_SUMMARY.md` (5 minutes)
   - Get overview of what's been created
   - See statistics and highlights

3. Look at `examples/hello.ibex` (2 minutes)
   - See a real Ibex program
   - Understand the syntax

### What you'll understand:
✅ What Ibex is and why it's different
✅ What's been completed
✅ Overall project scope

---

## ⏱️ Hour 2: Detailed Learning (1 hour)

### What to read:
1. `docs/LANGUAGE_DESIGN.md` (30 minutes)
   - Read sections 1-4 first
   - Understand the syntax
   - See type system overview

2. `docs/IDE_ARCHITECTURE.md` (20 minutes)
   - Understand IDE structure
   - See what the IDE does
   - Understand type registry role

3. `ARCHITECTURE_DIAGRAMS.md` (10 minutes)
   - Visual overview
   - Pipeline diagram
   - Component status

### What you'll understand:
✅ Complete language specifications
✅ How the IDE is designed
✅ Why the vexing parse problem is solved
✅ How type registry enables refactoring

---

## ⏱️ Hour 3-4: Code Review (2 hours)

### What to review:
1. `include/lexer.h` (15 minutes)
   - Token types
   - Lexer interface
   - Understand tokenization

2. `include/ast.h` (15 minutes)
   - AST node types
   - Declaration/expression structure
   - Complete hierarchy

3. `include/type_registry.h` (15 minutes)
   - Type system interface
   - How types are stored
   - Refactoring support

4. `src/lexer.cpp` first 100 lines (15 minutes)
   - See working code
   - Understand tokenization
   - Character classification

### What you'll understand:
✅ How lexer works
✅ What AST looks like
✅ How type system is organized
✅ Working C++ code patterns

---

## ⏱️ Hour 5: Building the Project (30 minutes)

### Follow these steps:

**Windows:**
```bash
# 1. Open PowerShell in c:\Dev\ibex
cd c:\Dev\ibex

# 2. Create build directory
mkdir build
cd build

# 3. Generate build files
cmake -G "Visual Studio 17 2022" ..

# 4. Build project
cmake --build . --config Release

# 5. Test the compiler (should show success)
.\bin\ibexc.exe ..\examples\hello.ibex

# Expected output:
# Tokenized X tokens
# Parsed X declarations
# Type registry initialized
# Compilation successful (preliminary)
```

**Linux:**
```bash
# 1. Navigate to project
cd ~/Dev/ibex

# 2. Create build directory
mkdir build && cd build

# 3. Configure project
cmake -DCMAKE_BUILD_TYPE=Release ..

# 4. Build
cmake --build .

# 5. Test
./bin/ibexc ../examples/hello.ibex
```

### What to check:
- ✅ No build errors
- ✅ ibexc executable created
- ✅ Can run compiler on hello.ibex
- ✅ Get success message

---

## ⏱️ Hour 6-8: Deep Dive (2-3 hours, Optional)

Choose your interest:

### Option A: Language Design Deep Dive
1. Finish reading `docs/LANGUAGE_DESIGN.md`
2. Review all syntax examples
3. Understand type system fully
4. Read about C interoperability
5. Study vexing parse solution

**Time: 1.5-2 hours**

### Option B: Parser Implementation Deep Dive
1. Read `docs/ROADMAP.md` (Phase 2)
2. Review `src/parser.cpp` (understand structure)
3. Identify what's missing (loops, classes)
4. Plan your implementation approach

**Time: 1.5-2 hours**

### Option C: IDE Architecture Deep Dive
1. Fully read `docs/IDE_ARCHITECTURE.md`
2. Review proposed keybindings
3. Study type registry browser design
4. Plan IDE implementation phases

**Time: 1.5-2 hours**

### Option D: Type System Deep Dive
1. Fully understand `include/type_registry.h`
2. Read all of `src/type_registry.cpp`
3. Understand how types are registered
4. Plan type refactoring features

**Time: 2 hours**

---

## 📊 After 8 Hours, You Will Know:

✅ **The Language**: Complete syntax, type system, memory model
✅ **The Design**: Why vexing parse is solved, how type registry works
✅ **The Architecture**: Compiler pipeline, IDE structure
✅ **The Code**: How lexer works, AST structure, type system
✅ **The Build System**: How to compile and run
✅ **The Roadmap**: What to implement next

---

## 🎯 First Day Summary

By the end of the first day, you should be able to:

1. ✅ Build and run the compiler
2. ✅ Explain the language syntax
3. ✅ Understand the type registry
4. ✅ Describe the IDE architecture
5. ✅ Identify next implementation steps
6. ✅ Review the parser code
7. ✅ Understand compilation pipeline

---

## 📚 Reading Checklist (Day 1)

- [ ] README.md (15 min)
- [ ] PROJECT_SUMMARY.md (10 min)
- [ ] examples/hello.ibex (5 min)
- [ ] docs/LANGUAGE_DESIGN.md (30 min)
- [ ] docs/IDE_ARCHITECTURE.md (20 min)
- [ ] ARCHITECTURE_DIAGRAMS.md (10 min)
- [ ] include/lexer.h (15 min)
- [ ] include/ast.h (15 min)
- [ ] include/type_registry.h (15 min)
- [ ] src/lexer.cpp (first 100 lines, 15 min)
- [ ] Build project (30 min)
- [ ] Deep dive option A/B/C/D (2-3 hours)

**Total: 4.5-5.5 hours of reading + building**

---

## 🚀 What to Do on Day 2+

### Option 1: Start Implementation
1. Read `docs/ROADMAP.md` fully (Phase 2)
2. Complete parser for loops/classes
3. Implement semantic analyzer
4. Write tests for your changes

### Option 2: Deepen Understanding
1. Study all implementation files
2. Understand design patterns
3. Plan optimization strategies
4. Interface with C libraries

### Option 3: Plan IDE Implementation
1. Review `docs/IDE_ARCHITECTURE.md` again
2. Plan component structure
3. Research UI framework (Qt, ImGui, etc.)
4. Design type registry UI

### Option 4: Comprehensive Analysis
1. Complete all of `ROADMAP.md`
2. Estimate resource requirements
3. Create detailed project plan
4. Design implementation phases

---

## 💡 Key Insights to Understand

### 1. Vexing Parse Problem
C++ can't tell if `T obj();` is a declaration or function.
**Ibex solves it** with `var obj: T;` (declaration) vs `func obj() -> T;` (function)

### 2. Type Registry
All types in one place with unique IDs, enabling auto-refactoring.
When you rename `Point` → `Vertex`, all uses automatically update.

### 3. Manual Memory
No GC = predictable performance and direct C linking.
Explicit `alloc()` and `free()` give full control.

### 4. IDE from the Start
The type registry is designed for IDE support from day one.
Rich metadata enables powerful refactoring and visualization.

---

## 🎓 Knowledge by the End of Day 1

After working through this guide, you'll understand:

### LANGUAGE DESIGN ⭐
- Complete syntax specification
- Why it's easier to parse than C++
- 12 primitive types and type system
- Memory management model
- C interoperability approach

### ARCHITECTURE ⭐
- 4-stage compiler pipeline
- Type registry centralization
- IDE architecture with 3 views
- How refactoring works
- Why design is scalable

### IMPLEMENTATION STATUS ⭐
- What's complete (lexer, parser 95%, type system)
- What's ready to implement (semantic analyzer, code gen)
- What's designed but not started (IDE)
- Resource estimates (15-20 weeks)

### CODE STRUCTURE ⭐
- Header/implementation organization
- AST node hierarchy (50+ types)
- Type registry interface
- Lexer and parser framework
- CMake build system

---

## ❓ Questions to Answer After Day 1

After reading and building, you should be able to answer:

1. **What problem does Ibex solve?**
   - Modern language, clear syntax, type registry for IDE

2. **How does it solve vexing parse?**
   - Explicit keywords (func, var, let, struct)

3. **What's special about the type registry?**
   - Centralized storage enables program-wide refactoring

4. **How does IDE get type information?**
   - Registry provides rich metadata

5. **What's the compilation pipeline?**
   - Lexer → Parser → Type Registry → Semantic Analysis → Code Gen

6. **What's implemented?**
   - Lexer (complete), Parser (95%), Type System (complete)

7. **What's ready to implement?**
   - Semantic analyzer, code generator, IDE components

8. **How long to completion?**
   - 15-20 weeks estimated

---

## 📖 Document Navigation for Quick Reference

**Start Here:**
- README.md - Project overview

**Quick Ref:**
- QUICK_REFERENCE.md - One-page overview
- ARCHITECTURE_DIAGRAMS.md - Visual explanations

**Language Design:**
- docs/LANGUAGE_DESIGN.md - Full specification

**IDE Design:**
- docs/IDE_ARCHITECTURE.md - Complete architecture

**Implementation:**
- docs/ROADMAP.md - What to do next
- docs/BUILD_GUIDE.md - How to build

**Code Reference:**
- FILE_LISTING.md - Where everything is
- include/*.h - API documentation

**Project Status:**
- COMPLETION_CHECKLIST.md - What's done
- PROJECT_SUMMARY.md - Complete overview

---

## ✨ By End of Today You'll Have:

1. ✅ Working build system (Windows/Linux)
2. ✅ Running compiler executable
3. ✅ Complete understanding of language design
4. ✅ Clear picture of IDE architecture
5. ✅ Knowledge of implementation priorities
6. ✅ Foundation for next steps

---

## 🎉 After Today

You will have the knowledge to:

**Manage the Project**
- Understand scope and timeline
- Estimate resources
- Plan phases

**Implement Features**
- Complete the parser
- Build semantic analyzer
- Implement code generator

**Extend the IDE**
- Design visual components
- Integrate with compiler
- Build refactoring tools

**Contribute Effectively**
- Follow architecture
- Understand design decisions
- Write consistent code

---

## 📞 Help References

Can't find something?
- Check `INDEX.md` - Master documentation index
- Check `FILE_LISTING.md` - Where every file is
- Check `QUICK_REFERENCE.md` - One-page overview

---

**Ready? Start with README.md right now!**

Estimated time to complete this guide:
- Light review: 4-5 hours (reading + building)
- Medium review: 6-8 hours (+ architecture deep dive)
- Comprehensive: 8-10+ hours (+ all deep dives)

**Your first 24 hours start now!** 🚀
