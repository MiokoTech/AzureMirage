<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24&height=220&section=header&text=AVM2%20Mini%20VM&fontSize=56&fontColor=ffffff&fontAlignY=38&desc=Standalone%20ActionScript%203%20bytecode%20runtime%20for%20C%2B%2B17&descSize=18&descAlignY=60&animation=fadeIn" alt="AVM2 Mini VM">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-C77DFF?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/CMake-supported-7AA2F7?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/AVM2-DoABC-FF6FA5?style=for-the-badge&logo=adobe&logoColor=white" alt="AVM2 DoABC">
  <img src="https://img.shields.io/badge/License-MIT-2EC4B6?style=for-the-badge&logo=opensourceinitiative&logoColor=white" alt="MIT License">
</p>

<p align="center">
  <img src="https://readme-typing-svg.demolab.com?font=Fira+Code&weight=500&size=18&pause=1200&color=C77DFF&center=true&vCenter=true&width=620&lines=Run+DoABC+bytecode+without+Flash+Player;130%2B+core+AVM2+opcodes;Native+C%2B%2B+host+bridge;Binary+cache+loading+in+under+1+ms" alt="Typing SVG">
</p>

---

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

### Objective

My goal is to expand the game's compatibility with fighters from other engines, especially [BVN](https://github.com/5DPLAY-Game-Studio/BleachVsNaruto) and MUGEN, with [Ikemen-GO](https://github.com/ikemen-engine/Ikemen-GO) as its main source, in a single runtime with high performance even on low-end devices.
Ensure that both fighters from different engines can run properly as they originally do and fight each other in a single game.

---

## Documentation

The SWF compiler engine and full VM will be available after **Azure Mirage - Chord of Lyra** is released. This library is open source for developers who plan to do the same thing.

---

<p align="center">
  <img src="assets/logo.svg" alt="Azure Mirage - Chord of Lyra" width="480">
</p>


## License

This project is released under the **MIT License**. Free to use for commercial, open source, and community modding purposes.


<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24&height=120&section=footer" alt="">
</p>
