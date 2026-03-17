# STM32 DCF77 Library
### A high-performance porting of Udo Klein's Arduino DCF77 Library

This repository contains the STM32 porting of the renowned DCF77 library originally developed by **Udo Klein**. This implementation leverages the power of 32-bit architecture to decode the DCF77 time signal with exceptional noise immunity, even in challenging reception conditions.

## 🚀 Projects & Supported Boards

The library is organized into three distinct projects developed using **STM32CubeIDE**:

1.  **Nucleo-L010RB**: Optimized for the Low-Power L0 series.
2.  **Bluepill (STM32F103C8)**: The classic adaptation for the F1 series (same hardware used in the original Arduino version).
3.  **Bluepill USB (CDC)**: A specialized version that outputs time data directly via the native USB port (Virtual COM Port).

---

## 🛠️ Features

* **Statistical Noise Filtering**: Retains Udo Klein's brilliant algorithm for synchronizing under heavy interference.
* **Precision Timing**: Re-engineered to use STM32 Hardware Timers for microsecond-accurate pulse edge detection.
* **Modular Design**: Easily adaptable to other STM32 families via CubeMX.

---

## 🙏 Special Thanks & Credits

I would like to express my deepest gratitude and compliments to **Udo Klein** ([blinkenlight.net](http://blog.blinkenlight.net/)). His original work on the DCF77 library is a masterpiece of signal processing on microcontrollers. This porting wouldn't have been possible without his extensive research and high-quality code. 

Thank you, Udo, for your contribution to the maker community!

---

## 🔧 Contributions & Bug Reports

This project is open for improvement! 
* If you find any **bugs** or issues in the STM32 adaptation, please open an **Issue**.
* If you wish to suggest **modifications** or support for new boards, feel free to submit a **Pull Request**.

Feedback from the community is highly appreciated to make this porting more robust.

---

## 📄 License
The original logic is property of Udo Klein. This porting is released under the [Insert your license, e.g., MIT] License.