# PixelPort Media Player for Sony PlayStation 1

An enhanced **Audio CD Player** for the original PlayStation (PS1) built using the **Psy-Q SDK**.  
It offers more features than the stock player and serves as an introduction to PS1 development.

**Tip:** Hold the **Start** button to display the control layout.

---

## ✨ Features
- Plays standard **Audio CDs** on the PS1.
- **Custom UI** with track information and a progress bar.
- **Two shuffle modes**:
  1. **Standard Shuffle** – plays tracks in random order without repeats until all have played.
  2. **Custom Shuffle** – manually select the next track, allowing repeats of the same track if desired.
- **Stereo Balance Adjustment**.
- On-screen **control guide**.
- All debug messages (`printf`) are redirected to the connected PC via the **PS1USB** device.

---

## 🛠 Development Notes
This project was created as a learning exercise for:
- **PS1 hardware control** using the Psy-Q SDK.
- Understanding **CD drive control functions** built into the SDK.
- Rendering graphics using **TIM** images.
- Redirecting debug output to a PC over **PS1USB**.

The **progress bar** at the bottom of the player dynamically updates for each track based on playback position.

---

## 📦 Acknowledgements
Special thanks to the following projects and tools:
- [**OrionSoft PS1USB**](https://www.orionsoft.games/retroshop/ps1usb.htm) – for debugging over USB.
- [**Hello CDDA example by ABelliqueux**](https://github.com/ABelliqueux/nolibgs_hello_worlds/blob/main/hello_cdda/hello_cdda.c) – for basic CD audio handling.
- [**TIM Example on PSXDev**](https://www.psxdev.net/forum/viewtopic.php?t=313) – for TIM image rendering guidance.

---

## 🖼 Screenshots

<p align="center">
  <img src="/images/display.png" alt="Main Player Screen">
</p>

<p align="center">
  <img src="/images/shufflemode1.png" alt="Shuffle Mode On">
</p>

<p align="center">
  <img src="/images/shufflemode2.png" alt="Shuffle Mode Indicator">
</p>

<p align="center">
  <img src="/images/balance.png" alt="Stereo Balance Adjustment">
</p>

<p align="center">
  <img src="/images/startButton.png" alt="Control Guide (Start Button)">
</p>

---

## 📜 License
This project is released for educational and personal use.  
Refer to included licenses of third-party resources where applicable.

