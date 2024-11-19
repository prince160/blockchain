#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <sstream>
#include <unordered_map>
#include <openssl/sha.h>
#include <iomanip>
#include <algorithm>

// Fonction pour calculer le hash SHA-256
std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Fonction pour générer un nonce basé sur une preuve de travail
int calculateNonce(const std::string& data) {
    int nonce = 0;
    std::string hash;
    do {
        std::stringstream ss;
        ss << data << nonce;
        hash = sha256(ss.str());
        nonce++;
    } while (hash.substr(0, 4) != "0000"); // Preuve de travail : le hash doit commencer par "0000"
    return nonce;
}

// Structure pour représenter un bloc
struct Block {
    std::string id;
    std::string previous_id;
    std::vector<std::string> transactions;
    std::time_t timestamp;
    int nonce;

    // Constructeur du bloc
    Block(const std::string& previous_id, const std::vector<std::string>& transactions)
        : previous_id(previous_id), transactions(transactions), timestamp(std::time(nullptr)), nonce(0) {
        id = generateId();
    }

    // Générer un ID basé sur les données du bloc avec preuve de travail
    std::string generateId() {
        std::string hash;
        do {
            std::stringstream ss;
            ss << previous_id << timestamp << nonce;
            for (const auto& transaction : transactions) {
                ss << transaction;
            }
            hash = sha256(ss.str());
            nonce++;
        } while (hash.substr(0, 4) != "0000"); // Preuve de travail : hash doit commencer par "0000"
        return hash;
    }

    // Affichage du bloc
    void display() const {
        std::cout << "-------------------------\n";
        std::cout << "Bloc ID: " << id << "\n";
        std::cout << "Previous ID: " << previous_id << "\n";
        std::cout << "Timestamp: " << timestamp << "\n";
        std::cout << "Nonce: " << nonce << "\n";
        std::cout << "Transactions (Votes cryptes):\n";
        for (const auto& transaction : transactions) {
            std::cout << "  " << transaction << "\n";
        }
        std::cout << "-------------------------\n";
    }
};

// Classe pour gérer la blockchain
class Blockchain {
private:
    std::vector<Block> chain;

public:
    Blockchain() {
        chain.emplace_back("0", std::vector<std::string>()); // Bloc génésis
    }

    void addBlock(const std::vector<std::string>& transactions) {
        std::string previous_id = chain.back().id;
        chain.emplace_back(previous_id, transactions);
    }

    const std::vector<Block>& getChain() const {
        return chain;
    }

    void display() const {
        for (const auto& block : chain) {
            block.display();
        }
    }
};

// Fonction pour enregistrer un vote crypté
std::string registerVote(const std::string& voter_id, const std::string& vote) {
    std::string vote_data = voter_id + ":" + vote;  // Combiner l'ID du votant et son vote
    return sha256(vote_data); // Retourner un vote crypté
}

// Fonction pour afficher les informations détaillées d'un votant
void displayVoterTransaction(int voter_number, const std::string& voter_id_hashed, const std::string& vote) {
    std::cout << "\n=== Votant " << voter_number << " ===\n";
    std::cout << "ID crypte : " << voter_id_hashed << "\n";
    std::cout << "Vote : " << vote << "\n";

    // Calculer le nonce pour chaque transaction individuelle
    std::string transaction_data = voter_id_hashed + ":" + vote;
    int nonce = calculateNonce(transaction_data);
    std::cout << "Nonce individuel : " << nonce << "\n";

    // Afficher la transaction cryptée
    std::cout << "Transaction cryptee : " << sha256(transaction_data + std::to_string(nonce)) << "\n";

    std::cout << "\n"; // Ajout d'une ligne vide
}

// Fonction pour compter les résultats des votes
std::unordered_map<std::string, int> countVotes(const Blockchain& blockchain) {
    std::unordered_map<std::string, int> results;

    for (const auto& block : blockchain.getChain()) {
        for (const auto& transaction : block.transactions) {
            // Ajouter votre logique pour déterminer quel vote correspond à quel candidat
            if (transaction.find("bleu") != std::string::npos) {
                results["bleu"]++;
            }
            else if (transaction.find("rouge") != std::string::npos) {
                results["rouge"]++;
            }
        }
    }
    return results;
}

int main() {
    Blockchain blockchain;

    std::cout << "=== Systeme de vote anonyme ===\n";
    std::cout << "Chaque votant peut choisir une couleur : bleu ou rouge.\n";
    std::cout << "Les votes sont cryptes et ajoutes a une blockchain.\n\n";

    int votants = 3; // Nombre de votants
    std::vector<std::string> votes;
    std::vector<std::pair<std::string, std::string>> voters; // Stocke les ID cryptés et les votes

    for (int i = 1; i <= votants; ++i) {
        std::string voter_id;
        std::cout << "Votant " << i << ", entrez votre numero d'identifiant (numero unique) : ";
        std::cin >> voter_id;
        std::string voter_id_hashed = sha256(voter_id);

        std::string vote;
        std::cout << "Entrez votre choix (bleu/rouge) : ";
        std::cin >> vote;

        // Validation du vote
        std::transform(vote.begin(), vote.end(), vote.begin(), ::tolower);
        while (vote != "bleu" && vote != "rouge") {
            std::cout << "Vote invalide ! Veuillez saisir 'bleu' ou 'rouge' : ";
            std::cin >> vote;
            std::transform(vote.begin(), vote.end(), vote.begin(), ::tolower);
        }

        // Enregistrer le vote crypté et les informations du votant
        votes.push_back(registerVote(voter_id_hashed, vote));
        voters.push_back({ voter_id_hashed, vote });

        std::cout << "Votre ID crypte est : " << voter_id_hashed << "\n\n"; // Ajout d'une ligne vide
    }

    // Ajouter un bloc avec les votes à la blockchain
    blockchain.addBlock(votes);

    // Afficher les informations unitaires de chaque votant
    std::cout << "\n=== Details des transactions par votant ===\n";
    for (size_t i = 0; i < voters.size(); ++i) {
        displayVoterTransaction(i + 1, voters[i].first, voters[i].second);
    }

    // Afficher les résultats des votes
    std::unordered_map<std::string, int> results = countVotes(blockchain);

    std::cout << "\n=== Resultats des votes ===\n";
    for (const auto& result : results) {
        std::cout << "Couleur " << result.first << " : " << result.second << " vote(s)\n";
    }

    // Afficher la blockchain
    std::cout << "\n=== Blockchain ===\n";
    blockchain.display();

    return 0;
}