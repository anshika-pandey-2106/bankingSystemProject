#include <iostream>
#include <pthread.h>
#include <vector>
#include <queue>
#include <map>
#include <unistd.h>
#include <algorithm>

using namespace std;

// BankAccount class representing each account
class BankAccount {
public:
    int id;  // Account ID
    int balance;  // Current balance of the account
    pthread_mutex_t mutex;  // Mutex for thread synchronization
    queue<pair<string, pair<int, int>>> transactionQueue; // Queue to hold transactions (type, {amount, destination if transfer})

    // Constructor to initialize account with given ID and initial balance
    BankAccount(int account_id, int initial_balance) {
        id = account_id;
        balance = initial_balance;
        pthread_mutex_init(&mutex, NULL);  // Initialize the mutex for the account
    }

    // Deposit method to add money to the account
    void deposit(int amount) {
        pthread_mutex_lock(&mutex);  // Lock account to ensure no other thread accesses it simultaneously
        balance += amount;  // Add amount to balance
        cout << "Account " << id << ": Deposited ₹" << amount
             << ", New Balance: ₹" << balance << endl;
        pthread_mutex_unlock(&mutex);  // Unlock account after operation
    }

    // Withdraw method to subtract money from the account
    void withdraw(int amount) {
        pthread_mutex_lock(&mutex);  // Lock account to ensure no other thread accesses it simultaneously
        if (balance >= amount) {
            balance -= amount;  // Subtract amount from balance if sufficient funds are available
            cout << "Account " << id << ": Withdrew ₹" << amount
                 << ", New Balance: ₹" << balance << endl;
        } else {
            cout << "Account " << id << ": Insufficient funds for withdrawal of ₹"
                 << amount << ", Balance: ₹" << balance << endl;
        }
        pthread_mutex_unlock(&mutex);  // Unlock account after operation
    }
};

// A map to store all bank accounts indexed by their ID
map<int, BankAccount*> accounts;

// Handles the transfer of money between two accounts safely
void transfer(int from_id, int to_id, int amount) {
    if (from_id == to_id) return;  // If source and destination are the same, no transfer needed

    BankAccount* from = accounts[from_id];  // Get the source account
    BankAccount* to = accounts[to_id];  // Get the destination account

    // Lock accounts in a consistent order to avoid deadlock
    BankAccount* first = from_id < to_id ? from : to;
    BankAccount* second = from_id < to_id ? to : from;

    pthread_mutex_lock(&first->mutex);  // Lock the first account
    pthread_mutex_lock(&second->mutex);  // Lock the second account

    // Check if the source account has enough balance to make the transfer
    if (from->balance >= amount) {
        from->balance -= amount;  // Subtract amount from source account
        to->balance += amount;  // Add amount to destination account
        cout << "Transferred ₹" << amount << " from Account " << from_id << " to Account " << to_id
             << ". New Balances: ₹" << from->balance << " = Account " << from_id
             << ", ₹" << to->balance << " = Account " << to_id << endl;
    } else {
        cout << "Transfer failed: Insufficient funds in Account " << from_id << endl;  // Insufficient funds
    }

    pthread_mutex_unlock(&second->mutex);  // Unlock the second account
    pthread_mutex_unlock(&first->mutex);  // Unlock the first account
}

// Thread function that processes transactions for each account
void* processTransactions(void* arg) {
    BankAccount* account = (BankAccount*) arg;  // Get the account from the passed argument
    while (!account->transactionQueue.empty()) {  // Process each transaction in the queue
        auto txn = account->transactionQueue.front();  // Get the first transaction
        account->transactionQueue.pop();  // Remove the transaction from the queue

        string type = txn.first;  // Transaction type (deposit, withdraw, or transfer)
        int amount = txn.second.first;  // Transaction amount
        int target = txn.second.second;  // Target account for transfer (if applicable)

        // Execute the appropriate action based on transaction type
        if (type == "deposit") {
            account->deposit(amount);  // Call deposit method if it's a deposit
        } else if (type == "withdraw") {
            account->withdraw(amount);  // Call withdraw method if it's a withdrawal
        } else if (type == "transfer") {
            transfer(account->id, target, amount);  // Call transfer method if it's a transfer
        }
    }
    return NULL;  // Return from the thread function
}

int main() {
    int n;
    cout << "Enter number of bank accounts: ";
    cin >> n;

    // Input account details
    for (int i = 0; i < n; i++) {
        int acc_id, initial_balance;
        cout << "Enter account ID and initial balance for Account " << (i + 1) << ": ";
        cin >> acc_id >> initial_balance;
        accounts[acc_id] = new BankAccount(acc_id, initial_balance);  // Create new account and add it to the map
    }

    int t;
    cout << "\nEnter number of transactions: ";
    cin >> t;

    // Input transactions to be performed
    for (int i = 0; i < t; i++) {
        int acc_id, amount, dest = -1;
        string type;

        cout << "Transaction " << (i + 1) << " - Enter account ID, type (deposit/withdraw/transfer), amount";
        cin >> acc_id >> type >> amount;

        // Check if the account ID is valid
        if (accounts.find(acc_id) == accounts.end()) {
            cout << "Invalid Account ID. Skipping this transaction.\n";
            continue;
        }

        // For transfers, also ask for destination account ID
        if (type == "transfer") {
            cout << "Enter destination account ID: ";
            cin >> dest;
            if (accounts.find(dest) == accounts.end()) {
                cout << "Invalid Destination Account ID. Skipping this transaction.\n";
                continue;
            }
        }

        // Add the transaction to the account's transaction queue
        accounts[acc_id]->transactionQueue.push({type, {amount, dest}});
    }

    // Create threads for each account to process transactions in parallel
    vector<pthread_t> threads;
    for (auto& acc : accounts) {
        pthread_t tid;
        pthread_create(&tid, NULL, processTransactions, acc.second);  // Create a thread for each account
        threads.push_back(tid);  // Store the thread ID
    }

    // Wait for all threads to complete
    for (auto& tid : threads) {
        pthread_join(tid, NULL);  // Wait for each thread to finish
    }

    // Output the final balances of all accounts
    cout << "\nFinal Account Balances:\n";
    for (auto& acc : accounts) {
        cout << "Account " << acc.first << ": ₹" << acc.second->balance << endl;
        delete acc.second;  // Clean up memory by deleting the account object
    }

    return 0;  // Exit the program
}
