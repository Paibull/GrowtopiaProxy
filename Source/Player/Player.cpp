#include "Player.hpp"
#include "../Packets/Packets.hpp"
#include "../Network/Handler.hpp"
#include "../Network/Network.hpp"
#include "../Items/Items.hpp"

PlayerState Player;



// ---------------- Inventory ----------------
int PlayerState::Inventory::GetCount(int id) const {
    auto it = items.find(id);
    if (it != items.end()) return it->second;
    return 0;
}

bool PlayerState::Inventory::HasItem(int id) const {
    return items.find(id) != items.end();
}

void PlayerState::Inventory::AddItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it != items.end()) {
        int newCount = it->second + count;
        if (newCount > MAX_STACK) newCount = MAX_STACK;
        it->second = newCount;
        return;
    }
    if (items.size() >= static_cast<size_t>(size)) return;
    if (count > MAX_STACK) count = MAX_STACK;
    items[id] = count;
}

void PlayerState::Inventory::RemoveItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it == items.end()) return;
    int newCount = it->second - count;
    if (newCount > 200) newCount = 200;
    if (newCount <= 0) items.erase(it);
    else it->second = newCount;
}

void PlayerState::Inventory::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& itemsVec) {
    items.clear();
    size = newSize;
    for (const auto& [id, count] : itemsVec) AddItem(id, count);
}


// ---------------- PlayerState public methods ----------------
std::string PlayerState::InventoryToString() const {
    std::ostringstream oss;
    for (const auto& [id, count] : inventory.items) { 
        if (const ItemDefinition* d = Items.GetItemByID(id)) oss << count << "x " << d->Name << "\n";
    }
    return oss.str();
}

int PlayerState::GetItemCount(int id) const {
    return inventory.GetCount(id);
}

bool PlayerState::HasItem(int id) const {
    return inventory.HasItem(id);
}

void PlayerState::AddItem(int id, int count) {
    inventory.AddItem(id, count);
}

void PlayerState::RemoveItem(int id, int count) {
    inventory.RemoveItem(id, count);
}

void PlayerState::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items) {
    inventory.ResetInventory(newSize, items);
}



// ---------------- DialogSkipper ----------------
bool PlayerState::GetDropFlag() const {
    return dialogSkipper.DROP;
}

void PlayerState::SetDropFlag(bool flag) {
    dialogSkipper.DROP = flag;
}



// ---------------- Functions ----------------
void PlayerState::Drop(int id, int count, std::string name, ENetPeer* CLIENT, ENetPeer* SERVER) {
    if (Player.HasItem(id) && Player.GetItemCount(id) >= count) {
        std::string firstPacket = "action|drop\n|itemID|" + std::to_string(id) + "\n";
        SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, firstPacket.c_str(), static_cast<int>(firstPacket.length()));

        Player.SetDropFlag(true);

        PlayerMovingPacket firstMoving;
        firstMoving
            .PacketType(Network.PACKET_SET_ICON_STATE)
            .NetID(Player.netID)
            .Punch(2, 0)
            .Send(SERVER);

        PlayerMovingPacket secondMoving;
        secondMoving
            .PacketType(Network.PACKET_SET_ICON_STATE)
            .NetID(Player.netID)
            .Send(SERVER);

        std::string secondPacket = "action|dialog_return\ndialog_name|drop_item\nitemID|" + std::to_string(id) + "|\ncount|" + std::to_string(count) + "\n";
        SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, secondPacket.c_str(), static_cast<int>(secondPacket.length()));

        {
            GamePacket<OnConsoleMessage> p;
            p.text = "Dropping " + std::to_string(count) + " " + name;
            SendGamePacket(CLIENT, p);
        }
    }
}