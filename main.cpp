#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

// Типы NPC
enum class NPCType { PRINCESS, DRAGON, KNIGHT };

class NPC {
public:
    NPC(const std::string& name, int x, int y) : name(name), x(x), y(y), alive(true) {}
    virtual ~NPC() = default;

    virtual NPCType getType() const = 0;
    virtual void accept(class Visitor& visitor) = 0;

    std::string getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
    void markDead() { alive = false; }

    double distanceTo(const NPC& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    }

private:
    std::string name;
    int x, y;
    bool alive;
};

class Princess : public NPC {
public:
    Princess(const std::string& name, int x, int y) : NPC(name, x, y) {}
    NPCType getType() const override { return NPCType::PRINCESS; }
    void accept(Visitor& visitor) override;
};

class Dragon : public NPC {
public:
    Dragon(const std::string& name, int x, int y) : NPC(name, x, y) {}
    NPCType getType() const override { return NPCType::DRAGON; }
    void accept(Visitor& visitor) override;
};

class Knight : public NPC {
public:
    Knight(const std::string& name, int x, int y) : NPC(name, x, y) {}
    NPCType getType() const override { return NPCType::KNIGHT; }
    void accept(Visitor& visitor) override;
};

// для логирования
class Observer {
public:
    virtual ~Observer() = default;
    virtual void onKill(const NPC& killer, const NPC& victim) = 0;
};

class ConsoleObserver : public Observer {
public:
    void onKill(const NPC& killer, const NPC& victim) override {
        std::cout << killer.getName() << " убил(а) " << victim.getName() << std::endl;
    }
};

class FileObserver : public Observer {
    std::ofstream logFile;
public:
    FileObserver() : logFile("log.txt", std::ios::app) {}
    void onKill(const NPC& killer, const NPC& victim) override {
        if (logFile.is_open()) {
            logFile << killer.getName() << " убил(а) " << victim.getName() << std::endl;
        }
    }
};

//обработка сражений
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(Princess& princess) = 0;
    virtual void visit(Dragon& dragon) = 0;
    virtual void visit(Knight& knight) = 0;
};

class BattleVisitor : public Visitor {
    NPC* other;
    double range;
    Observer& observer;
public:
    BattleVisitor(double range, Observer& observer) : other(nullptr), range(range), observer(observer) {}

    void setOther(NPC* npc) { other = npc; }

    void visit(Princess& princess) override {
    }

    void visit(Dragon& dragon) override {
        if (other && other->isAlive() && dragon.distanceTo(*other) <= range) {
            if (other->getType() == NPCType::PRINCESS) {
                other->markDead();
                observer.onKill(dragon, *other);
            }
        }
    }

    void visit(Knight& knight) override {
        if (other && other->isAlive() && knight.distanceTo(*other) <= range) {
            if (other->getType() == NPCType::DRAGON) {
                other->markDead();
                observer.onKill(knight, *other);
            }
        }
    }
};

void Princess::accept(Visitor& visitor) { visitor.visit(*this); }
void Dragon::accept(Visitor& visitor) { visitor.visit(*this); }
void Knight::accept(Visitor& visitor) { visitor.visit(*this); }

class NPCFactory {
public:
    static std::unique_ptr<NPC> createNPC(NPCType type, const std::string& name, int x, int y) {
        switch (type) {
            case NPCType::PRINCESS: return std::unique_ptr<NPC>(new Princess(name, x, y));
            case NPCType::DRAGON: return std::unique_ptr<NPC>(new Dragon(name, x, y));
            case NPCType::KNIGHT: return std::unique_ptr<NPC>(new Knight(name, x, y));
        }
        return nullptr;
    }

    static std::unique_ptr<NPC> loadFromStream(std::istream& in) {
        std::string typeStr, name;
        int x, y;
        if (in >> typeStr >> name >> x >> y) {
            NPCType type;
            if (typeStr == "PRINCESS") type = NPCType::PRINCESS;
            else if (typeStr == "DRAGON") type = NPCType::DRAGON;
            else if (typeStr == "KNIGHT") type = NPCType::KNIGHT;
            else return nullptr;

            if (x < 0 || x > 500 || y < 0 || y > 500) return nullptr;
            return createNPC(type, name, x, y);
        }
        return nullptr;
    }
};

// Класс подземелья
class Dungeon {
    std::vector<std::unique_ptr<NPC>> npcs;
    ConsoleObserver consoleObserver;
    FileObserver fileObserver;

public:
    void addNPC(NPCType type, const std::string& name, int x, int y) {
        if (x < 0 || x > 500 || y < 0 || y > 500) {
            std::cout << "Неверные координаты!" << std::endl;
            return;
        }
        npcs.push_back(NPCFactory::createNPC(type, name, x, y));
    }

    void print() const {
        for (const auto& npc : npcs) {
            if (!npc->isAlive()) continue;
            std::string typeStr;
            switch (npc->getType()) {
                case NPCType::PRINCESS: typeStr = "Принцесса"; break;
                case NPCType::DRAGON: typeStr = "Дракон"; break;
                case NPCType::KNIGHT: typeStr = "Рыцарь"; break;
            }
            std::cout << typeStr << " " << npc->getName() << " at (" << npc->getX() << ", " << npc->getY() << ")" << std::endl;
        }
    }

    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        for (const auto& npc : npcs) {
            if (!npc->isAlive()) continue;
            switch (npc->getType()) {
                case NPCType::PRINCESS: file << "PRINCESS "; break;
                case NPCType::DRAGON: file << "DRAGON "; break;
                case NPCType::KNIGHT: file << "KNIGHT "; break;
            }
            file << npc->getName() << " " << npc->getX() << " " << npc->getY() << std::endl;
        }
    }

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        npcs.clear();
        std::unique_ptr<NPC> npc;
        while ((npc = NPCFactory::loadFromStream(file))) {
            npcs.push_back(std::move(npc));
        }
    }

    void battle(double range) {
        BattleVisitor visitor(range, consoleObserver);
        for (size_t i = 0; i < npcs.size(); ++i) {
            if (!npcs[i]->isAlive()) continue;
            for (size_t j = i + 1; j < npcs.size(); ++j) {
                if (!npcs[j]->isAlive()) continue;
                visitor.setOther(npcs[j].get());
                npcs[i]->accept(visitor);
                visitor.setOther(npcs[i].get());
                npcs[j]->accept(visitor);
            }
        }
        // Удаляем мёртвых NPC
        npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(), [](const std::unique_ptr<NPC>& npc) { return !npc->isAlive(); }),
            npcs.end()
        );
    }
};

int main() {
    Dungeon dungeon;
    std::string command;

    while (std::cout << "Выберите команду: " && std::cin >> command) {
        if (command == "add") {
            std::string type, name;
            int x, y;
            std::cin >> type >> name >> x >> y;
            NPCType npcType;
            if (type == "princess") npcType = NPCType::PRINCESS;
            else if (type == "dragon") npcType = NPCType::DRAGON;
            else if (type == "knight") npcType = NPCType::KNIGHT;
            else {
                std::cout << "Неизвестный NPC" << std::endl;
                continue;
            }
            dungeon.addNPC(npcType, name, x, y);
        } else if (command == "print") {
            dungeon.print();
        } else if (command == "save") {
            std::string filename;
            std::cin >> filename;
            dungeon.saveToFile(filename);
        } else if (command == "load") {
            std::string filename;
            std::cin >> filename;
            dungeon.loadFromFile(filename);
        } else if (command == "battle") {
            double range;
            std::cin >> range;
            dungeon.battle(range);
        } else if (command == "exit") {
            break;
        } else {
            std::cout << "Неизвестная команда!" << std::endl;
        }
    }

    return 0;
}