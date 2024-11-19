// blockchain.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <sstream>
#include <unordered_map>
#include <openssl/sha.h>
#include <iomanip>
#include <cmath>

// Fonction pour calculer le hash SHA-256
std::string sha256(const std::string& str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}


// Structure pour représenter un nœud de l'arbre de Merkle
struct MerkleNode
{
    std::string hash; // Hachage du nœud
    std::vector<MerkleNode> children; // Enfants du nœud

    MerkleNode(const std::string& hash) : hash(hash) {}
};

// Structure de  Merkle
struct MerkleTree
{
    MerkleNode root;

    MerkleTree(const std::vector<std::string>& transactionHashes) : root(computeMerkleRoot(transactionHashes)) {}



    // Fonction pour calculer la racine de Merkle et construire l'arbre
    MerkleNode computeMerkleRoot(std::vector<std::string> transactionHashes) 
    {
        if (transactionHashes.empty())
            return MerkleNode("");

        std::vector<MerkleNode> nodes;

        // Créer des nœuds de feuille pour les hachages de transactions
        for (const auto& hash : transactionHashes) 
        {
            nodes.emplace_back(hash);
        }

        while (nodes.size() > 1) 
        {
            if (nodes.size() % 2 != 0)
            {
                nodes.push_back(nodes.back()); // Dupliquer le dernier hachage si impair
            }

            std::vector<MerkleNode> newLevel;
            for (size_t i = 0; i < nodes.size(); i += 2) 
            {
                std::string combinedHash = sha256(nodes[i].hash + nodes[i + 1].hash);
                MerkleNode parent(combinedHash);
                parent.children.push_back(nodes[i]); // Ajouter le premier enfant
                parent.children.push_back(nodes[i + 1]); // Ajouter le deuxième enfant
                newLevel.push_back(parent);
            }
            nodes = newLevel; // Passer au niveau suivant
        }

        return nodes[0]; // Retourner la racine de Merkle
    }

    // Méthode d'affichage pour l'arbre de Merkle
    void display() const 
    {
        std::cout << "Merkle Root: " << root.hash << "\n";
    }
};




// Structure pour représenter un bloc
struct Block
{
    std::string id;
    std::string previous_id;
    std::vector<std::string> transactions;
    std::time_t timestamp;
    int nonce;
    std::unordered_map<std::string, int> balances;
    std::string merkleRoot;

    // Constructeur du bloc
    Block(const std::string& previous_id, const std::vector<std::string>& transactions, const std::unordered_map<std::string, int>& previous_balances)
        : previous_id(previous_id), transactions(transactions), timestamp(std::time(nullptr)), nonce(0), balances(previous_balances)
    {
        for (const auto& transaction : transactions)
        {
            applyTransaction(transaction);
        }
        id = generateId();
    }
    // Récupérer les hachages des transactions
    std::vector<std::string> getTransactionHashes(const std::vector<std::string>& transactions) {
        std::vector<std::string> transactionHashes;
        for (const auto& transaction : transactions)
        {
            transactionHashes.push_back(sha256(transaction)); // Hachage de chaque transaction
        }
        return transactionHashes;
    }
    // Appliquer une transaction et mettre à jour les soldes
    void applyTransaction(const std::string& transaction)
    {
        std::istringstream ss(transaction);
        std::string from, to, amountStr;
        int amount;

        // Parsing de la transaction au format "Expéditeur->Destinataire:Montant"
        std::getline(ss, from, '-');
        ss.ignore(1);
        std::getline(ss, to, ':');
        std::getline(ss, amountStr);
        amount = std::stoi(amountStr);

        // Vérification que le montant est positif
        if (amount <= 0)
        {
            std::cerr << "Erreur : le montant de la transaction doit être positif.\n";
            return;
        }

        // Vérification que l'expéditeur a des fonds suffisants
        if (balances[from] < amount)
        {
            std::cerr << "Erreur : fonds insuffisants pour le compte " << from << ".\n";
            return;
        }

        // Effectuer la transaction si les vérifications sont satisfaites
        balances[from] -= amount;
        balances[to] += amount;

        // Affichage de la transaction réussie
        std::cout << "Transaction réussie : " << from << " -> " << to << " : " << amount << "\n";
    }


    // Générer un ID basé sur les données du bloc avec preuve de travail
    std::string generateId()
    {
        std::string hash;
        do
        {
            std::stringstream ss;
            ss << previous_id << timestamp << nonce;
            for (const auto& transaction : transactions)
            {
                ss << transaction;
            }
            hash = sha256(ss.str());
            nonce++;
        } while (hash.substr(0, 4) != "0000");

        return hash;
    }

    // Affichage du bloc
    void display() const
    {
        std::cout << "Previous ID: " << previous_id << "\n";
        std::cout << "Timestamp: " << timestamp << "\n";
        std::cout << "Nonce: " << nonce << "\n";
        std::cout << "Bloc ID: " << id << "\n";
        std::cout << "Merkle Root: " << merkleRoot << "\n";
        std::cout << "Transactions:\n";
        for (const auto& transaction : transactions)
        {
            std::cout << "  " << transaction << "\n";
        }
        std::cout << "Balances:\n";
        for (auto it = balances.begin(); it != balances.end(); ++it) // Modifié pour compatibilité C++11
        {
            std::cout << "  " << it->first << " : " << it->second << "\n";
        }
        std::cout << "\n";
    }
};
// Classe pour gérer la blockchain
class Blockchain
{
private:
    std::vector<Block> chain;
    std::string merkleRoot;
public:
    Blockchain()
    {
        std::unordered_map<std::string, int> genesisBalances = { {"A", 0}, {"B", 10}, {"C", 0} };
        chain.emplace_back("0", std::vector<std::string>(), genesisBalances);
    }

    void addBlock(const std::vector<std::string>& transactions)
    {
        std::string previous_id = chain.back().id;
        const auto& previous_balances = chain.back().balances;
        chain.emplace_back(previous_id, transactions, previous_balances);
    }

    void display() const
    {
        for (const auto& block : chain)
        {
            block.display();
        }
    }
};

int main()
{
    Blockchain blockchain;

    blockchain.addBlock({ "B->A:5", "B->C:3" });
    blockchain.addBlock({ "A->B:2", "C->A:1" });
    blockchain.addBlock({ "C->B:4", "A->C:2" });

    blockchain.display();

    return 0;
}

// Exécuter le programme : Ctrl+F5 ou menu Déboguer > Exécuter sans débogage
// Déboguer le programme : F5 ou menu Déboguer > Démarrer le débogage

// Astuces pour bien démarrer : 
//   1. Utilisez la fenêtre Explorateur de solutions pour ajouter des fichiers et les gérer.
//   2. Utilisez la fenêtre Team Explorer pour vous connecter au contrôle de code source.
//   3. Utilisez la fenêtre Sortie pour voir la sortie de la génération et d'autres messages.
//   4. Utilisez la fenêtre Liste d'erreurs pour voir les erreurs.
//   5. Accédez à Projet > Ajouter un nouvel élément pour créer des fichiers de code, ou à Projet > Ajouter un élément existant pour ajouter des fichiers de code existants au projet.
//   6. Pour rouvrir ce projet plus tard, accédez à Fichier > Ouvrir > Projet et sélectionnez le fichier .sln.
