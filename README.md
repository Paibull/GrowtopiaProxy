# 📋 Growtopia Proxy
Growtopia Bridge Proxy is a powerful tool that sits between the Growtopia client and server, allowing full control over network traffic. With this proxy, you can monitor, modify, and inject packets to customize your gameplay experience.

---

## 🛠 Features  
- **Bidirectional traffic handling:**
Client -> Proxy -> Server
and
Server -> Proxy -> Client

- **Packet logging for monitoring and debugging** 
- **Packet modification to alter game behavior in real-time** 
- **Custom packet injection to send your own crafted packets** 
---

## 🚀 Tech Stack  

- **Language:** ISO C++23 Standard (Visual Studio 2026 Community)    
- **Compiler Standards:**  
  - C++23 (`/std:c++23preview`)  
  - C17 (`/std:c17`)  
  - Target: `x64/Release`

---

## 🎮 Roadmap  

- [x] Fetching the real Growtopia server credentials
- [x] In-Built HTTP Server
- [x] In-Built Network Server and Client:
    - [x] Debugging all the packets
    - [x] Modifying the real-time packets
    - [x] Creating and sending custom packets
 - [x] Advanced log system

--- 

---

## 🛠 Feautures

- [x] Slash command /ping : Shows your current Client->Proxy and Proxy->Server pings
- [x] Slash command /wd <amount> : Drops the amount of World Locks
- [x] Slash command /dd <amount> : Drops the amount of Diamond Locks
- [x] Slash command /bd <amount> : Drops the amount of Blue Gem Locks

--- 

## Getting Started

### 1. Install Git
Download and install Git:  
https://github.com/git-for-windows/git/releases/download/v2.50.1.windows.1/Git-2.50.1-64-bit.exe  

### 2. Setup VCPKG & Dependencies
Open **CMD** and run:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.bootstrap-vcpkg.bat
.vcpkg install openssl:x64-windows
.vcpkg integrate install
```

### 3. Configure in Visual Studio
- Go to Project → Properties  
- Select Configuration Properties → vcpkg  
- Set Use Vcpkg to "Yes"  

## Building the Game
1. Open the project in **Visual Studio Community**  
2. Build the solution (`Ctrl + Shift + B`)  
3. Run the executables from (`x64/Release`)
4. `Source.exe`

Note: If you want to use Debug, use 'Local Windows Debugger' while 'x64/Release' is selected.
Note: It will ask permission to restart the visual studio with administrator mode, allow it!

## License
This project is licensed under the **MIT License** – see the LICENSE file for details.  

## Contributing
Pull requests are welcome! Feel free to open issues for bugs, feature requests, or ideas.  

## Credits
- C++               – Project language  
- ENet Networking   – Low-latency networking library  
- Http Extension    - A Library that allows you to host HTTP/HTTPS servers and modify the packets.
