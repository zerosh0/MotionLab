# MotionLab – Video Motion Analysis

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![C/C++](https://img.shields.io/badge/language-C%20%2F%20C%2B%2B-orange)
![License](https://img.shields.io/badge/license-GPLv3-g)
[![Latest Release](https://img.shields.io/github/v/release/zerosh0/MotionLab)](https://github.com/zerosh0/MotionLab/releases)

**MotionLab** is a modern tool for video tracking and kinematic analysis, designed for teaching and physics research. Using **OpenCV** for object tracking and the lightweight **Raylib** for the interface, it provides a fast, smooth, and intuitive alternative to traditional software.


![Trajectoire](assets/images/image1.png)

> ℹ️ MotionLab allows switching between French and English from the **Help** menu.


---

## 🚀 Why MotionLab?

MotionLab was created to fix common frustrations with so-called "legacy" tracking software, whose ergonomics and technical constraints are often out of step with modern usage.


* **Modern & Smooth** : GPU-accelerated rendering via OpenGL keeps the interface responsive at 60 FPS, even with thousands of data points.
* **Universal Video Compatibility** : Thanks to `FFmpeg`, MotionLab decodes almost any format (`.mp4`, `.mkv`, `.mov`, `.avi`) and modern codecs (H.264, H.265), unlike some software stuck with uncompressed `.avi`.
* **Drag & Drop Simplicity** : Import a video, calibrate the scale in two clicks, and start analyzing immediately.
* **No Limits** : No artificial restrictions on resolution (4K supported) or video length.
* **Cross-Platform** : Works on Linux and Windows (MacOS should work theoretically… empirically, results may vary).
* **Intelligent Assisted Tracking**: When manual pointing becomes long or tedious, MotionLab can **automatically track an object** across hundreds of frames, while still allowing you to take back manual control at any time.

* **No Telemetry, No Tracking**: MotionLab does not collect, record, or transmit any usage, statistical, or behavioral data.

> MotionLab is designed to step out of the way and let the physics analysis shine.

---

## 🖱️ Simple and Fast Pointing

MotionLab allows **immediate manual point tracking**.

All you need to do is:
1. **Set the origin and orient the axes**
2. **Define the scale** using a known distance
3. **Select the first frame** to analyze (if needed)
4. **Start pointing**, frame by frame, with a single click

![Trajectory](assets/images/showcase.gif)

## 🤖 Auto-Tracking System (CSRT)

MotionLab can use an automatic tracking module based on **OpenCV CSRT** (Channel and Spatial Reliability Tracker).

![Trajectoire](assets/images/tracking.gif)

### Smart Tracking Advantages
Unlike simple template matching, MotionLab’s tracker:
1.  **Adapts to Deformations:** : If the object rotates or changes perspective during motion, the tracker updates its visual model.
2.  **Noise-Resistant** : Real-time CLAHE (Contrast Limited Adaptive Histogram Equalization) and Gaussian blur stabilize detection.
3.  **Predicts Trajectory** : If the object is temporarily lost, the tracker uses the last known velocity vector to predict its position and try to reacquire it.

### 💡 Best practices for use

For optimal results, it is strongly recommended to start auto-tracking once the object is free of contact and actual movement begins.
>(The tracker is looking for the ball, not your hand.)

### ⚠️ Technical Limitations
Even though it performs well, auto-tracking has inherent computer vision limits:
* **Full Occlusion**: If the object disappears behind an obstacle, tracking stops.

* **Motion Blur**: Objects moving too fast for the camera shutter may become unrecognizable.

* **Low Contrast**: An object matching the background (e.g., a white ball on a white surface) may cause tracking failures.


---

## 📊 Scientific Analysis and Graphs
![Trajectoire](assets/images/curves.gif)
### Measurement Accuracy
To minimize numerical noise when computing derivatives, MotionLab uses the **central difference method**:
$$v_i = \frac{x_{i+1} - x_{i-1}}{t_{i+1} - t_{i-1}}$$


### Modeling (Best Fit)
The graphing tool includes a regression engine to overlay trend lines with automatic calculation of $R^2$ :
* **Linear** : $f(t) = a \cdot t + b$
* **Quadratic** : $f(t) = a \cdot t^2 + b \cdot t + c$

![Trajectoire](assets/images/image4.png)
![Trajectoire](assets/images/image3.png)
---

## 🏗️ Structure du Projet

```text
MotionLab/
├── include/           # Headers (.h)
├── src/               # Source code (.c, .cpp)
│   ├── video_engine.c # FFmpeg decoding engine and time management
│   ├── auto_tracker.cpp # OpenCV wrapper: CSRT logic and vision
│   ├── ui_graph.c     # Graph rendering system (OpenGL/RLGL)
│   ├── ui_canvas.c    # Interaction with video and points
│   ├── tracking.c     # Physics calculations (velocity, acceleration, calibration)
│   ├── resources.h    # Embedded assets (generated header)
│   └── ...            # UI widgets (Menus, Tables, Inputs)
# Yes, there are many files. No, they are not there by accident.
├── assets/            # Icons, fonts, system resources
└── CMakeLists.txt     # Cross-platform build script (Windows/Linux)
```
## ⚙️ Compilation

### Required Dependencies

- **Raylib** 5.0+
- **OpenCV** 4.x
- **FFmpeg** (avcodec, avformat, swscale)

### Build

Windows (VS2022 / MinGW) : 
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
Linux (Ubuntu/Debian/Fedora) :
```
sudo apt install libraylib-dev libopencv-dev libavcodec-dev libavformat-dev libswscale-dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## 📦 Asset Management
To ensure portability and ease of distribution, assets (icons, fonts) are embedded directly into the binary.

>(Because "it worked on my machine" is not a deployment strategy.)

Although source files are visible in the assets/ folder, MotionLab uses include/resources.h. This header contains binary data converted into unsigned char[] arrays.

The file is generated automatically using:
[generate_resources.py](https://gist.github.com/zerosh0/cce3d82cbc8c943db0d33abf1ea9d2a5)

## 📜 License

MotionLab is distributed under the GNU GPL v3. This means:

* You can use, modify, and redistribute the software freely.

* Any derivative work must also remain GPL (open-source).

