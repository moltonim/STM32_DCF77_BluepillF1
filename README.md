# STM32 DCF77 Library
### A high-performance porting of Udo Klein's Arduino DCF77 Library

This repository contains the STM32 porting of the renowned DCF77 library originally developed by **Udo Klein**. This implementation leverages the power of 32-bit architecture to decode the DCF77 time signal with exceptional noise immunity, even in challenging reception conditions.
As Linus Torvalds thinks, I **hate** C++ because of lack of understanding.

## 🚀 Projects & Supported Boards

The library is organized into three distinct projects developed using **STM32CubeIDE**:

1.  **Nucleo-L010RB**: Optimized for the Low-Power L0 series. **This can be a start point for any other STM32 microcontroller**.
2.  **Bluepill (STM32F103C8)**: The classic adaptation for the F1 series (same hardware used in the original Arduino version).
3.  **Bluepill USB (CDC)**: A specialized version that outputs time data directly via the native USB port (Virtual COM Port).

I decided to split it in a 3 divverent direcory, give the .hex file to upload it without recompile. Inside the direcotry there is some mre info concerning pins and tips.
In any **new** project: after create a standard C project, with main.c and other periferals, add all the necessary .cpp and .h file. Don't forget to change the type of project by right clicking on the project name in CubeIDE and select "Change to C++".
The core of the project is always the timer TIMx set to 1 MHz. In my example I use a 8 Mhz Xtal and use it as a main Clock (HSE). Also APB1 Timer clock is 8 MHZ. If you wish to use a highter clock you **must** modify "parameters settings" inside the clock page (in CubeMX); such as Prescaler, counter period etc. It **must** be a 1MHz clock.

---

## ⚠️ A Note on BluePill F1 Clones

When I started this project, I wasn't aware of the extensive market for **STM32F103 clones** (and other series). While these boards might look identical to the original, they often house chips like the CS32, CKS, or other rebranded silicon.

**Important Compatibility Issues:**
* **Programming:** You might still be able to program them via Arduino IDE or the legacy STM32 ST-LINK Utility.
* **Debugging:** They generally **will not work** for real-time debugging within STM32CubeIDE, as the IDE looks for a genuine ST-Link/Chip ID.
* **Binaries:** The provided `.hex` files might not run on these clones.

Identifying a genuine STM32 chip has become increasingly difficult. If you encounter issues during the flashing or debugging phase, please verify the authenticity of your BluePill MCU.


---

## 🛠️ Features

* **Statistical Noise Filtering**: Retains Udo Klein's brilliant algorithm for synchronizing under heavy interference. Udo's algorithm uses a statistical binning technique (Bayesian filtering) to reconstruct the signal even in the presence of very strong electrical noise. Udo Klein's library is a masterpiece of signal engineering, but it's written with extensive use of C++ Templates (and hardest to undertand and convert to C!).
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
* If you wish to suggest **modifications** or support for new boards, feel free to submit a **Pull Request**. _I can't promise to follow all the requests, but I'll try_.

Feedback from the community is highly appreciated to make this porting more robust.

---

## 👨‍💻 About Me

I am a veteran Electronic Engineer with over 30 years of experience in firmware development. 

In the maker world, I go by the alias **Raptus** (or **TheRaptus**). This nickname carries a bit of history: it all started back in 1983 with my first **Sinclair ZX Spectrum**. I had bought it in England and brought it back to Italy personally (let's call it an "unofficial import"... I'm not a smuggler!). 

While tinkering with that machine, I managed to crack a famous game called *Atic Atac*, and I needed a handle to sign my work. "Raptus" was the first name that came to mind, and it has stuck with me for over 43 years!

---

## 📄 License
The original logic is property of Udo Klein. This porting is released under the same Udo's License.
