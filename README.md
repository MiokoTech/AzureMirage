<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24&height=220&section=header&text=AVM2%20Mini%20VM&fontSize=56&fontColor=ffffff&fontAlignY=38&desc=Standalone%20ActionScript%203%20bytecode%20runtime%20for%20C%2B%2B17&descSize=18&descAlignY=60&animation=fadeIn" alt="AVM2 Mini VM">
</p>

<p align="center">
  <img src="assets/logo.svg" alt="Azure Mirage - Chord of Lyra" width="480">
</p>

<p align="center">
  <img src="https://readme-typing-svg.demolab.com?font=Fira+Code&weight=500&size=18&pause=1200&color=C77DFF&center=true&vCenter=true&width=620&lines=Run+DoABC+bytecode+without+Flash+Player;50%2B+core+AVM2+opcodes;Native+C%2B%2B+host+bridge;Binary+cache+loading+in+under+1+ms" alt="Typing SVG">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-C77DFF?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/CMake-supported-7AA2F7?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/AVM2-DoABC-FF6FA5?style=for-the-badge&logo=adobe&logoColor=white" alt="AVM2 DoABC">
  <img src="https://img.shields.io/badge/License-MIT-2EC4B6?style=for-the-badge&logo=opensourceinitiative&logoColor=white" alt="MIT License">
</p>

<p align="center">
  <img src="https://count.getloli.com/@MiokoTech?theme=booru-lewd&padding=6&offset=0&align=bottom&scale=1.1&pixelated=1&darkmode=auto" alt="Visitor Count" />
</p>
---

## Overview

A lightweight and fast standalone Virtual Machine to execute pure **ActionScript 3 (AVM2 / DoABC)** bytecode in C++17. 

Specially designed for games that need to run modder custom BVN fighter scripts from Adobe Flash/Animate (`.swf`) files **directly from binary cache without the involvement of Flash Player or Adobe AIR runtimes**.

---

## Problem Background

In the game Bleach vs Naruto / BVN, modders often create unique attack logic, gravity suction, wall bounce, damage reflection, and counters using ActionScript 3 scripts inside character MovieClips (`.swf`).

**AVM2 Mini VM** solves this problem!
Directly executes the original DoABC instruction bytecode via an efficient C++ stack machine. No syntactic assumptions or transpilation bugs.

---

## Key Features

- **Standalone & Embeddable**: Simply include the `avm2_vm.h` header and `avm2_vm.cpp` into your C++ project.
- **Support for 50+ Core AVM2 Opcodes**:
  - Stack Manipulation: `dup`, `swap`, `pop`, `kill`.
  - Constant Loading: `pushbyte`, `pushshort`, `pushint`, `pushuint`, `pushdouble`, `pushstring`, `pushtrue`, `pushfalse`, `pushnull`, `pushundefined`.
  - Local Registers: `getlocal_0` to `getlocal_3`, `setlocal_0` to `setlocal_3`, `getlocal n`, `setlocal n`.
  - Branching & Control Flow: `jump`, `iftrue`, `iffalse`, `ifeq`, `ifne`, `iflt`, `ifle`, `ifgt`, `ifge`, `ifnle`.
  - Arithmetic & Bitwise: `add`, `subtract`, `multiply`, `divide`, `modulo`, `negate`, `increment`, `decrement`, `bitand`, `bitor`, `bitxor`, `bitnot`.
  - Objects & Dynamic Properties: `getproperty`, `setproperty`, `initproperty`, `getlex`, `findpropstrict`, `newobject`, `newarray`, `newactivation`, `getslot`, `setslot`, `in`, `construct`, `constructprop`.
  - Function & Method Calls: `callproperty`, `callpropvoid`, `returnvoid`, `returnvalue`, `newfunction` (closure).
  - Dynamic Indexing: Supports `<MultinameL>` and `<RTQNameL>` for runtime-based array and dictionary access (`obj[key]`).
  - Coercion & Casting: `convert_s`, `convert_i`, `convert_u`, `convert_d`, `convert_b`, `coerce`, `coerce_a`, `coerce_s`.
- **C++ Host Interop / Bridge**: Through the virtual base class `Avm2Object`, C++ hosts can expose variables, coordinates, game methods, and callbacks to the AS3 script environment with native performance.
- **Built-in Simulation**: Default mock math objects (`Math.min`, `Math.max`, `Math.abs`, `Math.floor`, `Math.ceil`, `Math.random`) and `Dictionary` handling.

---

## Component Structure

| Component | Description |
|---|---|
| `Avm2Value` | Dynamic tagged-union representing AS3 values (`Undefined`, `Null`, `Bool`, `Int`, `Double`, `String`, `Object`, `Array`). |
| `Avm2Object` | Dynamic base object class to hold hashmap properties, array elements, and C++ method calls. |
| `Avm2ConstantPool` | Container for integer, unsigned integer, double, string, and multiname constants extracted from SWF DoABC tags. |
| `Avm2MethodBody` | Stores method metadata (`maxStack`, `localCount`, `initScopeDepth`, `maxScopeDepth`) and instruction bytecode buffer. |
| `Avm2Interpreter` | Virtual stack machine execution engine. |

---

## How to Use (Quickstart)

### 1. C++ Bridge Definition (Connecting Game Objects to AS3)

To expose C++ objects (such as a game character entity) to ActionScript 3 scripts, inherit your class from `Avm2Object`:

```cpp
#include "avm2_vm.h"
#include <iostream>

class PlayerEntityBridge : public azure_mirage::Avm2Object {
public:
    int hp = 1000;
    int facing = 1; // 1 = right, -1 = left

    // Access property: player.hp, player.direct
    azure_mirage::Avm2Value getProperty(const std::string& name) override {
        if (name == "hp") return azure_mirage::Avm2Value::integer(hp);
        if (name == "direct") return azure_mirage::Avm2Value::integer(facing);
        return azure_mirage::Avm2Object::getProperty(name);
    }

    // Modify property from AS3: player.hp = 800; player.direct = -1;
    void setProperty(const std::string& name, const azure_mirage::Avm2Value& val) override {
        if (name == "hp") {
            hp = val.toInt();
            return;
        }
        if (name == "direct") {
            facing = (val.toInt() < 0) ? -1 : 1;
            return;
        }
        azure_mirage::Avm2Object::setProperty(name, val);
    }

    // Call method from AS3: player.loseHp(50);
    azure_mirage::Avm2Value callMethod(const std::string& name, 
                                       const std::vector<azure_mirage::Avm2Value>& args, 
                                       void* caller) override {
        if (name == "loseHp" && !args.empty()) {
            int dmg = args[0].toInt();
            hp -= dmg;
            std::cout << "[C++] Player took damage: " << dmg << " (Remaining HP: " << hp << ")\n";
            return azure_mirage::Avm2Value::undefined();
        }
        return azure_mirage::Avm2Object::callMethod(name, args, caller);
    }
};
```

### 2. Loading Constant Pool & Bytecode Methods

Constant Pool data and bytecode methods can be extracted directly from SWF DoABC tags (Tag Code 82) during asset compilation:

```cpp
azure_mirage::Avm2ConstantPool pool;
pool.strings = {"", "Math", "min", "hp", "loseHp", "direct"};
pool.multinames = {"", "Math", "min", "hp", "loseHp", "direct"};

azure_mirage::Avm2MethodBody reflectMethod;
reflectMethod.name = "reflectDamage";
reflectMethod.maxStack = 8;
reflectMethod.localCount = 4;

// Raw AVM2 bytecode instructions (DoABC code slice)
reflectMethod.code = {
    // 0xD0: getlocal_0 (this)
    // 0x24 0x32: pushbyte 50
    // 0x4F ... : callpropvoid loseHp, 1
    0xD0, 0x24, 0x32, 0x4F, 0x04, 0x01, 0x47
};
```

### 3. Executing Scripts from C++

```cpp
auto player = std::make_shared<PlayerEntityBridge>();

// Execute AVM2 method directly
azure_mirage::Avm2Interpreter::getInstance().execute(
    reflectMethod,
    pool,
    player.get()
);

std::cout << "Player final HP: " << player->hp << "\n";
```

---

## Standalone Binary Cache Serialization Format

For runtimes without SWF, the Constant Pool and Method Bodies can be serialized into a sequential cache binary:

```
[4 Byte] Magic: "AVM2" (0x324D5641)
--- CONSTANT POOL ---
[4 Byte] Int Count       -> [4 Byte * Count]  int32_t
[4 Byte] UInt Count      -> [4 Byte * Count]  uint32_t
[4 Byte] Double Count    -> [8 Byte * Count]  double
[4 Byte] String Count    -> [2 Byte Len + Data] strings
[4 Byte] Multiname Count -> [2 Byte Len + Data] multinames
--- METHOD BODIES ---
[2 Byte] Method Count
  Per Method:
    [2 Byte Len + Data] Method Name
    [2 Byte] maxStack
    [2 Byte] localCount
    [2 Byte] initScopeDepth
    [2 Byte] maxScopeDepth
    [4 Byte] Code Length
    [N Byte] Raw Bytecode Buffer
```

This format is very fast to read (`< 1ms`) and requires no XML parsing or decompilation while the game is running.

---

## Real-world Test Results (Real-world Benchmarks)

This library has been tested and verified against real ActionScript 3 scripts from complex SWF fighters:

1. **Accelerator**:
   - `reflect()`: Reflected damage calculation formula `Math.min(defenseHit.power * defenseHit.powerRate * 0.2, 150)`, HP reduction via `target.loseHp()`, and KO trigger `checkFighterDie()`.
   - `suMoveTarget()`: Opponent state detection `getTargetState()`, coordinate suction `moveTarget({followmc: "zxd", speed: 10})`, and event unhooking `removeEventListener(Event.ENTER_FRAME)`.
   - `setTargetHitWall()`: Wall collision detection and delayed closure dispatch.

2. **Ryougi Shiki**:
   - `changeDirect()`: Natural sprite facing direction reversal.
   - `changeDirectByAD()`: Real-time input reading of joystick/keyboard `$key.moveLEFT()` and `$key.moveRIGHT()`.
   - `customSlowDown()`: Frame slow-motion hit trigger.
   - `intoAction()`: Action state and hitstun override.

---

## Compilation & Build

Add the source file to your project's `CMakeLists.txt`:

```cmake
add_library(avm2_vm STATIC
    avm2_vm.cpp
)

target_include_directories(avm2_vm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

Or compile directly using your C++17 compiler of choice:

```bash
clang++ -std=c++17 -O3 -c avm2_vm.cpp -o avm2_vm.o
```

---

## License

This project is released under the **MIT License**. Free to use for commercial, open source, and community modding purposes.


<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24&height=120&section=footer" alt="">
</p>
