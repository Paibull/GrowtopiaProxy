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
- [x] Change Settings from Config.hpp file
- [x] Fetching the real Growtopia server credentials
- [x] In-Built HTTP Server
- [x] In-Built Network Server and Client:
    - [x] Debugging all the packets
    - [x] Modifying the real-time packets
    - [x] Creating and sending custom packets
- [ ] Custom Socks5 Support
- [x] Advanced log system

---

## Info:
- [x] Slash command  (/proxy) : Shows the proxy feautures.
- [x] Slash command  (/news) : Shows the real Growtopia news.
- [x] Slash command  (/ping) : Shows your current Client and Server pings.

## Customize:
- [x] Slash command  (/ui) : Opens UI customizer menu.
- [x] Slash command  (/country code) : Changes your country flag.

## Main Features:
- [x] Slash command  (/farm) : Starts auto farming (legacy, anti-ban).
- [x] Slash command  (/spam) : Opens Spam Settings menu.
- [x] Slash command  (/wrench) : Activates/Deactivates the auto wrench (left click = pull, right click = kick).

## Information:
- [x] Slash command  (/balance) : Shows your current lock balance.
- [x] Slash command  (/account) : Opens Account Informations menu.
- [x] Slash command  (/inventory) : Opens Invrntory menu.

## Shortcuts:
- [x] Slash command  (/drop) : Activates/Deactivates the Fast-Drop.
- [x] Slash command  (/trash) : Activates/Deactivates the Fast-Trash.
- [x] Slash command  (/pullall) : Pulls everyone in the world.
- [x] Slash command  (/kickall) : Kicks everyone in the world.
- [x] Slash command  (/banall) : Bans everyone in the world.

## Drop Feautures:
- [x] Slash command  (/cd amount) : Drops the amount of locks.
- [x] Slash command  (/wd amount) : Drops the amount of World Locks.
- [x] Slash command  (/dd amount) : Drops the amount of Diamond Locks.
- [x] Slash command  (/bd amount) : Drops the amount of Blue Gem Locks.
- [x] Slash command  (/daw) : Drops your all locks.
- [x] Slash command  (/warp name) : Warps to the chozen world.
- [x] Slash command   (/back) : Warps to the last world.
- [x] Slash command   (/setsave name) : Set the save world.
- [x] Slash command   (/save) : Warps to the save world.
- [x] Slash command   (/relog) : Leaves and joins to the same world.

## Anti Lag:
- [x] Slash command  (/hideclothes) : Hides the clothes of everyone and removes the effects.

## Extra
- [x] Fast system : Removes delay in GamePackets.
- [x] Roulette Verifier : Shows if the roulette shows the real log.

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
2. Edit the (`Main/Config.hpp`) file
3. Build the solution (`Ctrl + Shift + B`)  
4. Run the `Source.exe` from (`x64/Release`)

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
