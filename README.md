# 🏓 Pong Game (C & ncurses)

A classic Pong game implemented in **C** using the **ncurses** library.
Play in the terminal with smooth controls, increasing difficulty, and both single-player and local multiplayer modes.

---

## 🎮 Features

* 🧠 Single-player mode (AI opponent)
* 👥 Two-player local mode
* 📈 Dynamic difficulty (ball speed increases over time)
* 🎯 Score tracking system
* 🖥️ Terminal-based UI using ncurses
* ⏸️ Pause menu and game states

---

## 🛠️ Requirements

* GCC (C compiler)
* ncurses library
* Make (optional, if using Makefile)

---

## ⚙️ Installation & Run

### 1. Clone the repository

```bash
git clone https://github.com/MustafaSaktas0/Pong.git
cd Pong
```

### 2. Compile

With Makefile:

```bash
make
```

Or manually:

```bash
gcc pong.c -o pong -lncurses
```

### 3. Run

```bash
./pong
```

---

## 🎹 Controls

### Player 1

* `W` → Move Up
* `S` → Move Down

### Player 2

* `↑` → Move Up
* `↓` → Move Down

### General

* `P` → Pause
* `Q` → Quit

---

## 🧠 How it works

* Game loop handles rendering and input in real-time
* Ball physics include direction and speed changes
* AI predicts ball position (basic logic)
* Score system resets round with slight delay

---

## 🚀 Future Improvements

* Sound effects 🔊
* Better AI difficulty levels 🤖
* GUI version (SDL / OpenGL) 🎨
* Online multiplayer 🌐

---

## 📄 License

This project is licensed under the MIT License.

---

## 👨‍💻 Author

**Mustafa Saktaş**

---

## ⭐ Support

If you like this project, consider giving it a star ⭐ on GitHub!
