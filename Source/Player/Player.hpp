#pragma once

#include <string>
#include <unordered_map>
#include "ENET/enet.h"

class PlayerState {
public:
    int netID = -1;
    int userID = -1;
    std::string tankIDName = "";


    struct Position {
        float x = 0.0f;
        float y = 0.0f;
    }; Position position;


    struct Inventory {
        static constexpr int MAX_STACK = 200;
        int size = 16;
        std::unordered_map<int, int> items;

        int GetCount(int id) const;
        bool HasItem(int id) const;
        void AddItem(int id, int count);
        void RemoveItem(int id, int count);
        void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);
    };

    struct DialogSkipper {
        bool DROP = false;
    };

private:
    Inventory inventory;
    DialogSkipper dialogSkipper;

public:
    int GetItemCount(int id) const;
    bool HasItem(int id) const;
    void AddItem(int id, int count);
    void RemoveItem(int id, int count);
    void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);

    bool GetDropFlag() const;
    void SetDropFlag(bool flag);

public:
    void Drop(int id, int count, std::string name, ENetPeer* CLIENT, ENetPeer* SERVER);
};

extern PlayerState Player;