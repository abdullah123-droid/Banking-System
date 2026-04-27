#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
using namespace std;

// Constants
const int MAX_USERS = 100;
const int MAX_COMPANIES = 50;
const int MAX_PENDING = 50;
const int MAX_EMPLOYEES_PER_COMPANY = 20;

// Card class
class Card {
    string cardNumber;
    string pin;
    string userID;
    int failedAttempts;
    bool isBlocked;

public:
    Card(string uid = "", string p = "") : userID(uid), pin(p), failedAttempts(0), isBlocked(false) {
        cardNumber = generateCardNumber();
    }

    string generateCardNumber() {
        string num = "";
        for (int i = 0; i < 16; i++) {
            num += to_string(rand() % 10);
        }
        return num;
    }

    string getCardNumber() { return cardNumber; }
    string getPin() { return pin; }
    string getUserID() { return userID; }

    bool verifyPin(string p) {
        if (isBlocked) {
            cout << "Card is blocked!\n";
            return false;
        }
        if (p == pin) {
            failedAttempts = 0;
            return true;
        }
        failedAttempts++;
        cout << "Invalid PIN! Attempts: " << failedAttempts << "/3\n";
        if (failedAttempts >= 3) {
            isBlocked = true;
            cout << "ALERT: Card blocked due to 3 failed attempts!\n";
        }
        return false;
    }

    bool isCardBlocked() { return isBlocked; }
    void unblockCard() { isBlocked = false; failedAttempts = 0; }
    void setPin(string p) { pin = p; }
};

// Base Account class
class Account {
protected:
    string accountID;
    string password;
    double balance;
    int dailyLimit;
    int dailyWithdrawn;
    string accountType;
    bool isActive;

public:
    Account() : balance(0), dailyLimit(0), dailyWithdrawn(0), isActive(true) {}
    virtual ~Account() {}

    virtual bool deposit(double amount) {
        if (amount <= 0 || !isActive) return false;
        balance += amount;
        return true;
    }

    virtual bool withdraw(double amount) {
        if (!isActive || amount <= 0 || amount > balance || (dailyWithdrawn + amount) > dailyLimit)
            return false;
        balance -= amount;
        dailyWithdrawn += amount;
        return true;
    }

    void resetDailyLimit() { dailyWithdrawn = 0; }

    double getBalance() { return balance; }
    string getAccountID() { return accountID; }
    string getPassword() { return password; }
    string getType() { return accountType; }
    bool getActiveStatus() { return isActive; }

    void setBalance(double b) { balance = b; }
    void setAccountID(string id) { accountID = id; }
    void setPassword(string pass) { password = pass; }
    void setDailyLimit(int limit) { dailyLimit = limit; }
    void setAccountType(string type) { accountType = type; }
    void setActiveStatus(bool status) { isActive = status; }

    bool canWithdraw(double amount) {
        return isActive && amount > 0 && amount <= balance && (dailyWithdrawn + amount) <= dailyLimit;
    }
};

// UserClient class
class UserClient : public Account {
    string fullName;
    string address;
    string phone;
    string cnic;
    Card* card;
    bool approved;

public:
    UserClient() : card(nullptr), approved(false) {
        setAccountType("Bronze");
        setDailyLimit(100000);
    }

    ~UserClient() {
        if (card) delete card;
    }

    void setUserDetails(string name, string addr, string ph, string cn) {
        fullName = name;
        address = addr;
        phone = ph;
        cnic = cn;
    }

    void approveAccount(string pin) {
        approved = true;
        card = new Card(accountID, pin);
    }

    void saveCardToFile() {
        if (!card) return;
        ofstream file("cards.txt", ios::app);
        file << accountID << "," << card->getCardNumber() << "," << card->getPin() << endl;
        file.close();
    }

    void saveToFile() {
        ofstream file("users.txt", ios::app);
        file << accountID << "," << fullName << "," << address << "," << cnic << ","
            << accountID << "," << password << "," << balance << "," << (approved ? "1" : "0") << endl;
        file.close();
    }

    bool isApproved() { return approved; }
    string getName() { return fullName; }
    string getCNIC() { return cnic; }
    string getAddress() { return address; }
    string getPhone() { return phone; }

    Card* getCard() { return card; }

    bool verifyCardPin(string pin) {
        if (!card) return false;
        return card->verifyPin(pin);
    }

    bool hasBlockedCard() {
        return card && card->isCardBlocked();
    }

    void unblockCard() {
        if (card) card->unblockCard();
    }
};

// CompanyClient class
class CompanyClient : public Account {
    string companyName;
    string companyAddress;
    string taxNumber;
    string employeeIDs[MAX_EMPLOYEES_PER_COMPANY];
    int employeeCount;
    bool loanRequested;

public:
    CompanyClient() : employeeCount(0), loanRequested(false) {
        setAccountType("Business");
        setDailyLimit(20000000);
    }

    void setCompanyDetails(string name, string addr, string tax) {
        companyName = name;
        companyAddress = addr;
        taxNumber = tax;
    }

    bool addEmployee(string empID) {
        if (employeeCount >= MAX_EMPLOYEES_PER_COMPANY) return false;
        employeeIDs[employeeCount++] = empID;
        return true;
    }

    bool isEmployee(string empID) {
        for (int i = 0; i < employeeCount; i++) {
            if (employeeIDs[i] == empID) return true;
        }
        return false;
    }

    void requestLoan() {
        loanRequested = true;
    }

    bool hasLoanRequest() { return loanRequested; }
    void clearLoanRequest() { loanRequested = false; }

    void saveToFile() {
        ofstream file("companies.txt", ios::app);
        file << accountID << "," << companyName << "," << companyAddress << ","
            << taxNumber << "," << accountID << "," << password << "," << balance << ",1" << endl;
        file.close();
    }

    void saveEmployeesToFile() {
        ofstream file("companies_employees.txt", ios::app);
        for (int i = 0; i < employeeCount; i++) {
            file << employeeIDs[i] << "," << accountID << endl;
        }
        file.close();
    }

    string getName() { return companyName; }
    string getTaxNumber() { return taxNumber; }
};

// Banking System - COMPLETE VERSION
class BankingSystem {
    UserClient* users[MAX_USERS];
    CompanyClient* companies[MAX_COMPANIES];
    UserClient* pendingUsers[MAX_PENDING];
    int userCount;
    int companyCount;
    int pendingCount;

    string getCurrentDate() {
        time_t t = time(0);
        char buffer[20];
        strftime(buffer, 20, "%Y-%m-%d", localtime(&t));
        return string(buffer);
    }

    string generateID() {
        return to_string(rand() % 90000 + 10000);
    }

    bool idExists(string id) {
        for (int i = 0; i < userCount; i++) {
            if (users[i]->getAccountID() == id) return true;
        }
        for (int i = 0; i < companyCount; i++) {
            if (companies[i]->getAccountID() == id) return true;
        }
        for (int i = 0; i < pendingCount; i++) {
            if (pendingUsers[i]->getAccountID() == id) return true;
        }
        return false;
    }

    void recordTransaction(string clientID, double amount, string type, string targetID = "") {
        ofstream file("transactions.txt", ios::app);
        file << getCurrentDate() << "," << clientID << "," << amount << "," << type;
        if (targetID != "") file << "," << targetID;
        file << endl;
        file.close();
    }

    UserClient* findUserByID(string id) {
        for (int i = 0; i < userCount; i++) {
            if (users[i]->getAccountID() == id) return users[i];
        }
        return nullptr;
    }

    CompanyClient* findCompanyByID(string id) {
        for (int i = 0; i < companyCount; i++) {
            if (companies[i]->getAccountID() == id) return companies[i];
        }
        return nullptr;
    }

    bool isEmployeeOfCompany(string userID, string companyID) {
        ifstream file("companies_employees.txt");
        string line;
        while (getline(file, line)) {
            size_t pos = line.find(',');
            if (pos != string::npos) {
                string empID = line.substr(0, pos);
                string compID = line.substr(pos + 1);
                if (empID == userID && compID == companyID) {
                    file.close();
                    return true;
                }
            }
        }
        file.close();
        return false;
    }

    void createAdminFile() {
        ofstream file("admin.txt");
        file << "admin,admin123" << endl;
        file.close();
    }

    bool validateAdmin(string id, string pass) {
        ifstream file("admin.txt");
        string line;
        while (getline(file, line)) {
            size_t pos = line.find(',');
            if (pos != string::npos) {
                string adminID = line.substr(0, pos);
                string adminPass = line.substr(pos + 1);
                if (adminID == id && adminPass == pass) {
                    file.close();
                    return true;
                }
            }
        }
        file.close();
        return false;
    }

public:
    BankingSystem() : userCount(0), companyCount(0), pendingCount(0) {
        for (int i = 0; i < MAX_USERS; i++) users[i] = nullptr;
        for (int i = 0; i < MAX_COMPANIES; i++) companies[i] = nullptr;
        for (int i = 0; i < MAX_PENDING; i++) pendingUsers[i] = nullptr;
        createAdminFile();
    }

    ~BankingSystem() {
        for (int i = 0; i < userCount; i++) delete users[i];
        for (int i = 0; i < companyCount; i++) delete companies[i];
        for (int i = 0; i < pendingCount; i++) delete pendingUsers[i];
    }

    void registerUser() {
        if (pendingCount >= MAX_PENDING) {
            cout << "Cannot create more accounts. Limit reached.\n";
            return;
        }

        string name, addr, phone, cnic, pass;
        int estLimit;

        cout << "=== USER REGISTRATION ===\n";
        cout << "Full Name: "; cin.ignore(); getline(cin, name);
        cout << "Address: "; getline(cin, addr);
        cout << "Phone: "; getline(cin, phone);
        cout << "CNIC: "; getline(cin, cnic);
        cout << "Password: "; getline(cin, pass);
        cout << "Estimated Daily Limit: "; cin >> estLimit;

        // Validate CNIC (simple check)
        if (cnic.length() != 13) {
            cout << "Invalid CNIC! Must be 13 digits.\n";
            return;
        }

        UserClient* newUser = new UserClient();
        newUser->setUserDetails(name, addr, phone, cnic);

        string id;
        do { id = generateID(); } while (idExists(id));

        newUser->setAccountID(id);
        newUser->setPassword(pass);

        // Set account type based on limit
        if (estLimit <= 100000) {
            newUser->setAccountType("Bronze");
            newUser->setDailyLimit(100000);
        }
        else {
            newUser->setAccountType("Gold");
            newUser->setDailyLimit(500000);
        }

        pendingUsers[pendingCount++] = newUser;
        newUser->saveToFile();

        cout << "Account created! ID: " << id << " - Waiting for admin approval.\n";
    }

    void registerCompany() {
        if (companyCount >= MAX_COMPANIES) {
            cout << "Cannot create more company accounts. Limit reached.\n";
            return;
        }

        string name, addr, tax, pass;
        int estLimit;

        cout << "=== COMPANY REGISTRATION ===\n";
        cout << "Company Name: "; cin.ignore(); getline(cin, name);
        cout << "Address: "; getline(cin, addr);
        cout << "Tax Number: "; getline(cin, tax);
        cout << "Password: "; getline(cin, pass);
        cout << "Estimated Daily Limit: "; cin >> estLimit;

        CompanyClient* newCompany = new CompanyClient();
        newCompany->setCompanyDetails(name, addr, tax);

        string id;
        do { id = generateID(); } while (idExists(id));

        newCompany->setAccountID(id);
        newCompany->setPassword(pass);

        companies[companyCount++] = newCompany;
        newCompany->saveToFile();

        cout << "Company account created! ID: " << id << endl;
    }

    void userLogin() {
        string id, pass;
        cout << "User ID: "; cin >> id;
        cout << "Password: "; cin >> pass;

        UserClient* user = findUserByID(id);
        if (user && user->getPassword() == pass && user->isApproved() && user->getActiveStatus()) {
            cout << "Login successful! Welcome " << user->getName() << endl;
            userMenu(user);
        }
        else {
            cout << "Invalid credentials or account not approved/active!\n";
        }
    }

    void companyLogin() {
        string id, pass;
        cout << "Company ID: "; cin >> id;
        cout << "Password: "; cin >> pass;

        CompanyClient* company = findCompanyByID(id);
        if (company && company->getPassword() == pass && company->getActiveStatus()) {
            cout << "Login successful! Welcome " << company->getName() << endl;
            companyMenu(company);
        }
        else {
            cout << "Invalid credentials or account not active!\n";
        }
    }

    void employeeCompanyAccess() {
        string userID, companyID;
        cout << "Your User ID: "; cin >> userID;
        cout << "Company ID: "; cin >> companyID;

        UserClient* user = findUserByID(userID);
        CompanyClient* company = findCompanyByID(companyID);

        if (user && company && isEmployeeOfCompany(userID, companyID)) {
            cout << "Access granted to company account!\n";
            companyMenu(company);
        }
        else {
            cout << "Access denied! Not an employee or invalid IDs.\n";
        }
    }

    void userMenu(UserClient* user) {
        while (true) {
            cout << "\n=== USER MENU ===\n";
            cout << "1. Deposit\n2. Withdraw\n3. Transfer\n4. View Balance\n";
            cout << "5. Transaction History\n6. Access Company Account\n7. Logout\n";
            cout << "Choice: ";
            int choice;
            cin >> choice;

            switch (choice) {
            case 1: {
                double amt;
                cout << "Amount: "; cin >> amt;
                if (user->deposit(amt)) {
                    recordTransaction(user->getAccountID(), amt, "Deposit");
                    cout << "Deposit successful!\n";
                }
                else {
                    cout << "Deposit failed!\n";
                }
                break;
            }
            case 2: {
                if (user->hasBlockedCard()) {
                    cout << "Card is blocked! Contact admin.\n";
                    break;
                }
                string pin;
                cout << "Enter PIN: "; cin >> pin;
                if (!user->verifyCardPin(pin)) {
                    cout << "PIN verification failed!\n";
                    break;
                }
                double amt;
                cout << "Amount: "; cin >> amt;
                if (user->withdraw(amt)) {
                    recordTransaction(user->getAccountID(), amt, "Withdraw");
                    cout << "Withdrawal successful!\n";
                }
                else {
                    cout << "Withdrawal failed! Check balance/daily limit.\n";
                }
                break;
            }
            case 3: {
                if (user->hasBlockedCard()) {
                    cout << "Card is blocked! Contact admin.\n";
                    break;
                }
                string pin;
                cout << "Enter PIN: "; cin >> pin;
                if (!user->verifyCardPin(pin)) {
                    cout << "PIN verification failed!\n";
                    break;
                }
                string targetID;
                double amt;
                cout << "Recipient ID: "; cin >> targetID;
                cout << "Amount: "; cin >> amt;

                UserClient* recipient = findUserByID(targetID);
                if (!recipient || !recipient->getActiveStatus()) {
                    cout << "Invalid recipient!\n";
                    break;
                }

                if (user->withdraw(amt)) {
                    recipient->deposit(amt);
                    recordTransaction(user->getAccountID(), amt, "Transfer", targetID);
                    cout << "Transfer successful!\n";
                }
                else {
                    cout << "Transfer failed! Check balance/daily limit.\n";
                }
                break;
            }
            case 4:
                cout << "Balance: " << user->getBalance() << endl;
                break;
            case 5:
                viewUserTransactions(user->getAccountID());
                break;
            case 6:
                employeeCompanyAccess();
                break;
            case 7:
                return;
            default:
                cout << "Invalid choice!\n";
            }
        }
    }

    void companyMenu(CompanyClient* company) {
        while (true) {
            cout << "\n=== COMPANY MENU ===\n";
            cout << "1. Deposit\n2. Withdraw\n3. Transfer to User\n4. View Balance\n";
            cout << "5. Transaction History\n6. Request Loan\n7. Add Employee\n8. Logout\n";
            cout << "Choice: ";
            int choice;
            cin >> choice;

            switch (choice) {
            case 1: {
                double amt;
                cout << "Amount: "; cin >> amt;
                if (company->deposit(amt)) {
                    recordTransaction(company->getAccountID(), amt, "Deposit");
                    cout << "Deposit successful!\n";
                }
                else {
                    cout << "Deposit failed!\n";
                }
                break;
            }
            case 2: {
                double amt;
                cout << "Amount: "; cin >> amt;
                if (company->withdraw(amt)) {
                    recordTransaction(company->getAccountID(), amt, "Withdraw");
                    cout << "Withdrawal successful!\n";
                }
                else {
                    cout << "Withdrawal failed! Check balance/daily limit.\n";
                }
                break;
            }
            case 3: {
                string userID;
                double amt;
                cout << "User ID: "; cin >> userID;
                cout << "Amount: "; cin >> amt;

                UserClient* recipient = findUserByID(userID);
                if (!recipient || !recipient->getActiveStatus()) {
                    cout << "Invalid user account!\n";
                    break;
                }

                if (company->withdraw(amt)) {
                    recipient->deposit(amt);
                    recordTransaction(company->getAccountID(), amt, "Transfer", userID);
                    cout << "Transfer successful!\n";
                }
                else {
                    cout << "Transfer failed! Check balance/daily limit.\n";
                }
                break;
            }
            case 4:
                cout << "Balance: " << company->getBalance() << endl;
                break;
            case 5:
                viewUserTransactions(company->getAccountID());
                break;
            case 6:
                company->requestLoan();
                cout << "Loan request submitted for admin approval.\n";
                break;
            case 7: {
                string empID;
                cout << "Employee User ID: "; cin >> empID;
                if (findUserByID(empID)) {
                    if (company->addEmployee(empID)) {
                        company->saveEmployeesToFile();
                        cout << "Employee added successfully!\n";
                    }
                    else {
                        cout << "Cannot add more employees!\n";
                    }
                }
                else {
                    cout << "User ID not found!\n";
                }
                break;
            }
            case 8:
                return;
            default:
                cout << "Invalid choice!\n";
            }
        }
    }

    void adminMenu() {
        string id, pass;
        cout << "Admin ID: "; cin >> id;
        cout << "Password: "; cin >> pass;

        if (!validateAdmin(id, pass)) {
            cout << "Invalid admin credentials!\n";
            return;
        }

        while (true) {
            cout << "\n=== ADMIN MENU ===\n";
            cout << "1. View All Accounts\n2. Approve User Accounts\n3. View Loan Requests\n";
            cout << "4. Approve/Reject Loans\n5. Freeze/Unfreeze Accounts\n6. View Transactions\n7. Logout\n";
            cout << "Choice: ";
            int choice;
            cin >> choice;

            switch (choice) {
            case 1:
                viewAllAccounts();
                break;
            case 2:
                approveUserAccounts();
                break;
            case 3:
                viewLoanRequests();
                break;
            case 4:
                processLoanRequests();
                break;
            case 5:
                manageAccountStatus();
                break;
            case 6: {
                string accID;
                cout << "Account ID: "; cin >> accID;
                viewUserTransactions(accID);
                break;
            }
            case 7:
                return;
            default:
                cout << "Invalid choice!\n";
            }
        }
    }

    void viewAllAccounts() {
        cout << "\n=== USER ACCOUNTS ===\n";
        for (int i = 0; i < userCount; i++) {
            cout << "ID: " << users[i]->getAccountID() << " | Name: " << users[i]->getName()
                << " | Balance: " << users[i]->getBalance() << " | Type: " << users[i]->getType()
                << " | Status: " << (users[i]->getActiveStatus() ? "Active" : "Frozen") << endl;
        }

        cout << "\n=== COMPANY ACCOUNTS ===\n";
        for (int i = 0; i < companyCount; i++) {
            cout << "ID: " << companies[i]->getAccountID() << " | Name: " << companies[i]->getName()
                << " | Balance: " << companies[i]->getBalance() << " | Status: "
                << (companies[i]->getActiveStatus() ? "Active" : "Frozen") << endl;
        }

        cout << "\n=== PENDING USER ACCOUNTS ===\n";
        for (int i = 0; i < pendingCount; i++) {
            cout << "ID: " << pendingUsers[i]->getAccountID() << " | Name: " << pendingUsers[i]->getName() << endl;
        }
    }

    void approveUserAccounts() {
        if (pendingCount == 0) {
            cout << "No pending accounts!\n";
            return;
        }

        for (int i = 0; i < pendingCount; i++) {
            cout << i << ". ID: " << pendingUsers[i]->getAccountID()
                << " | Name: " << pendingUsers[i]->getName()
                << " | CNIC: " << pendingUsers[i]->getCNIC() << endl;
        }

        int idx;
        cout << "Select account to approve: "; cin >> idx;
        if (idx < 0 || idx >= pendingCount) {
            cout << "Invalid selection!\n";
            return;
        }

        string pin;
        cout << "Set PIN for new card: "; cin >> pin;

        pendingUsers[idx]->approveAccount(pin);
        pendingUsers[idx]->saveCardToFile();

        // Move to active users
        users[userCount++] = pendingUsers[idx];
        for (int i = idx; i < pendingCount - 1; i++) {
            pendingUsers[i] = pendingUsers[i + 1];
        }
        pendingCount--;

        cout << "Account approved and card issued!\n";
    }

    void viewLoanRequests() {
        bool found = false;
        cout << "\n=== LOAN REQUESTS ===\n";
        for (int i = 0; i < companyCount; i++) {
            if (companies[i]->hasLoanRequest()) {
                cout << "Company: " << companies[i]->getName()
                    << " | ID: " << companies[i]->getAccountID()
                    << " | Balance: " << companies[i]->getBalance() << endl;
                found = true;
            }
        }
        if (!found) cout << "No loan requests.\n";
    }

    void processLoanRequests() {
        viewLoanRequests();
        string compID;
        cout << "Enter Company ID to process: "; cin >> compID;

        CompanyClient* company = findCompanyByID(compID);
        if (!company || !company->hasLoanRequest()) {
            cout << "Invalid company or no loan request!\n";
            return;
        }

        cout << "1. Approve Loan\n2. Reject Loan\nChoice: ";
        int choice;
        cin >> choice;

        if (choice == 1) {
            double amount;
            cout << "Loan amount: "; cin >> amount;
            company->deposit(amount);
            recordTransaction(company->getAccountID(), amount, "Loan");
            cout << "Loan approved and disbursed!\n";
        }
        else {
            cout << "Loan rejected!\n";
        }
        company->clearLoanRequest();
    }

    void manageAccountStatus() {
        string accID;
        cout << "Account ID: "; cin >> accID;

        UserClient* user = findUserByID(accID);
        CompanyClient* company = findCompanyByID(accID);

        if (user) {
            bool current = user->getActiveStatus();
            user->setActiveStatus(!current);
            cout << "User account " << (current ? "frozen" : "unfrozen") << "!\n";
        }
        else if (company) {
            bool current = company->getActiveStatus();
            company->setActiveStatus(!current);
            cout << "Company account " << (current ? "frozen" : "unfrozen") << "!\n";
        }
        else {
            cout << "Account not found!\n";
        }
    }

    void viewUserTransactions(string accID) {
        ifstream file("transactions.txt");
        string line;
        cout << "\n=== TRANSACTIONS FOR " << accID << " ===\n";
        bool found = false;
        while (getline(file, line)) {
            // Check if line contains the account ID
            size_t pos1 = line.find("," + accID + ",");
            size_t pos2 = line.find("," + accID + ",", pos1 + 1);
            if (pos1 != string::npos || pos2 != string::npos) {
                cout << line << endl;
                found = true;
            }
        }
        if (!found) cout << "No transactions found.\n";
        file.close();
    }

    void mainMenu() {
        srand(time(0));

        while (true) {
            cout << "\n=== ABC BANK MANAGEMENT SYSTEM ===\n";
            cout << "1. User Registration\n2. Company Registration\n3. User Login\n";
            cout << "4. Company Login\n5. Admin Login\n6. Exit\n";
            cout << "Choice: ";
            int choice;
            cin >> choice;

            switch (choice) {
            case 1: registerUser(); break;
            case 2: registerCompany(); break;
            case 3: userLogin(); break;
            case 4: companyLogin(); break;
            case 5: adminMenu(); break;
            case 6:
                cout << "Thank you for using ABC Bank!\n";
                return;
            default:
                cout << "Invalid choice!\n";
            }
        }
    }
};

int main() {
    BankingSystem system;
    system.mainMenu();
    return 0;
}