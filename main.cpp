// ============================================================================
//  CLINIC APPOINTMENT SYSTEM  -  OOPS (C++) PBL / Mini Project
//  Author  : Mbafor Joel Afese   (UID 25BCD10002, Section 25BCD-3B)
//  College : Chandigarh University
//  Unique feature : SMART SLOT SUGGESTION
//      If a doctor's slot is already booked, the program suggests the next
//      free slots of that doctor and other doctors of the same specialization.
// ============================================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
using namespace std;

const int SLOTS_PER_DAY = 16;          // 09:00, 09:30 ... 16:30

// ----------------------------------------------------------------------------
//  Small helper functions
// ----------------------------------------------------------------------------
bool allDigits(const string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++)
        if (!isdigit((unsigned char)s[i])) return false;
    return true;
}

// removes the characters | and ; because they are used as separators in the files
string cleanText(string s) {
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == '|' || s[i] == ';') s[i] = ' ';
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// splits "a|b|c" into a vector {"a","b","c"} (keeps empty fields)
vector<string> splitText(const string& s, char sep) {
    vector<string> parts;
    string current;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == sep) { parts.push_back(current); current = ""; }
        else current += s[i];
    }
    parts.push_back(current);
    return parts;
}

string money(double x) {
    ostringstream os;
    os << fixed << setprecision(2) << x;
    return os.str();
}

// slot number (0..15) <-> time text ("09:00".."16:30")
string slotToTime(int slot) {
    int minutes = 9 * 60 + slot * 30;
    ostringstream os;
    os << setfill('0') << setw(2) << minutes / 60 << ":" << setw(2) << minutes % 60;
    return os.str();
}

int timeToSlot(const string& t) {          // returns -1 if the time is not a valid slot
    if (t.size() != 5 || t[2] != ':') return -1;
    string hh = t.substr(0, 2), mm = t.substr(3, 2);
    if (!allDigits(hh) || !allDigits(mm)) return -1;
    int minutes = stoi(hh) * 60 + stoi(mm) - 9 * 60;
    if (minutes < 0 || minutes % 30 != 0 || minutes / 30 >= SLOTS_PER_DAY) return -1;
    return minutes / 30;
}

// ----------------------------------------------------------------------------
//  Class Date : a calendar date (day, month, year)
// ----------------------------------------------------------------------------
class Date {
private:
    int day, month, year;

    static bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
    static int daysInMonth(int m, int y) {
        int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && isLeap(y)) return 29;
        return days[m - 1];
    }

public:
    Date() : day(1), month(1), year(2026) {}
    Date(int d, int m, int y) : day(d), month(m), year(y) {}

    bool isValid() const {
        if (year < 2000 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        return day >= 1 && day <= daysInMonth(month, year);
    }

    void nextDay() {                        // moves to the next day (handles month / year end)
        day++;
        if (day > daysInMonth(month, year)) { day = 1; month++; }
        if (month > 12) { month = 1; year++; }
    }

    string toString() const {               // YYYY-MM-DD
        ostringstream os;
        os << setfill('0') << setw(4) << year << "-" << setw(2) << month << "-" << setw(2) << day;
        return os.str();
    }

    // reads "YYYY-MM-DD"; returns false if the text is not a real date
    static bool parse(const string& s, Date& result) {
        if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
        string y = s.substr(0, 4), m = s.substr(5, 2), d = s.substr(8, 2);
        if (!allDigits(y) || !allDigits(m) || !allDigits(d)) return false;
        Date temp(stoi(d), stoi(m), stoi(y));
        if (!temp.isValid()) return false;
        result = temp;
        return true;
    }

    bool operator==(const Date& other) const {      // operator overloading
        return day == other.day && month == other.month && year == other.year;
    }
};

// ----------------------------------------------------------------------------
//  Class Person : abstract base class for Patient and Doctor
// ----------------------------------------------------------------------------
class Person {
protected:
    int id;
    string name;
    string phone;

public:
    Person(int id, const string& name, const string& phone) {
        if (name.empty())
            throw invalid_argument("Name cannot be empty.");
        if (phone.size() != 10 || !allDigits(phone))
            throw invalid_argument("Phone number must be exactly 10 digits.");
        this->id = id;
        this->name = name;
        this->phone = phone;
    }
    virtual ~Person() {}

    int getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }

    // pure virtual functions: every derived class must define them
    virtual string describe() const = 0;
    virtual string toFileLine() const = 0;
};

// operator overloading: cout << person   (works for Patient and Doctor)
ostream& operator<<(ostream& os, const Person& p) {
    os << p.describe();
    return os;
}

// ----------------------------------------------------------------------------
//  Class Patient (inherits Person)
// ----------------------------------------------------------------------------
class Patient : public Person {
private:
    int age;
    char gender;
    string bloodGroup;
    vector<string> history;                 // one line for every completed visit

public:
    Patient(int id, const string& name, const string& phone, int age, char gender,
            const string& bloodGroup)
        : Person(id, name, phone) {
        if (age < 0 || age > 120)
            throw invalid_argument("Age must be between 0 and 120.");
        gender = toupper((unsigned char)gender);
        if (gender != 'M' && gender != 'F' && gender != 'O')
            throw invalid_argument("Gender must be M, F or O.");
        string groups[] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};
        bool ok = false;
        for (int i = 0; i < 8; i++)
            if (bloodGroup == groups[i]) ok = true;
        if (!ok)
            throw invalid_argument("Blood group must be one of A+ A- B+ B- AB+ AB- O+ O-.");
        this->age = age;
        this->gender = gender;
        this->bloodGroup = bloodGroup;
    }

    void addHistory(const string& entry) { history.push_back(entry); }
    const vector<string>& getHistory() const { return history; }

    string describe() const {
        ostringstream os;
        os << "[Patient #" << id << "] " << name << " | " << phone << " | " << age
           << " yrs | " << gender << " | " << bloodGroup;
        return os.str();
    }

    string toFileLine() const {             // id|name|phone|age|gender|blood|history
        ostringstream os;
        os << id << "|" << name << "|" << phone << "|" << age << "|" << gender << "|"
           << bloodGroup << "|";
        for (size_t i = 0; i < history.size(); i++) {
            if (i > 0) os << ";";
            os << history[i];
        }
        return os.str();
    }
};

// ----------------------------------------------------------------------------
//  Class Doctor (inherits Person)
// ----------------------------------------------------------------------------
class Doctor : public Person {
private:
    string specialization;
    double fee;

public:
    Doctor(int id, const string& name, const string& phone, const string& specialization,
           double fee)
        : Person(id, name, phone) {
        if (specialization.empty())
            throw invalid_argument("Specialization cannot be empty.");
        if (fee < 0)
            throw invalid_argument("Consultation fee cannot be negative.");
        this->specialization = specialization;
        this->fee = fee;
    }

    string getSpecialization() const { return specialization; }
    double getFee() const { return fee; }

    string describe() const {
        return "[Doctor  #" + to_string(id) + "] Dr. " + name + " | " + specialization +
               " | Fee Rs." + money(fee) + " | " + phone;
    }

    string toFileLine() const {             // id|name|phone|specialization|fee
        return to_string(id) + "|" + name + "|" + phone + "|" + specialization + "|" + money(fee);
    }
};

// ----------------------------------------------------------------------------
//  Class Appointment
// ----------------------------------------------------------------------------
class Appointment {
private:
    int id, patientId, doctorId;
    Date date;
    int slot;
    string status;                          // Scheduled, Completed or Cancelled
    string reason;

public:
    Appointment(int id, int patientId, int doctorId, const Date& date, int slot,
                const string& status, const string& reason) {
        this->id = id;
        this->patientId = patientId;
        this->doctorId = doctorId;
        this->date = date;
        this->slot = slot;
        this->status = status;
        this->reason = reason;
    }

    int getId() const { return id; }
    int getPatientId() const { return patientId; }
    int getDoctorId() const { return doctorId; }
    Date getDate() const { return date; }
    int getSlot() const { return slot; }
    string getStatus() const { return status; }
    string getReason() const { return reason; }
    void setStatus(const string& s) { status = s; }

    string toFileLine() const {             // id|patientId|doctorId|date|slot|status|reason
        return to_string(id) + "|" + to_string(patientId) + "|" + to_string(doctorId) + "|" +
               date.toString() + "|" + to_string(slot) + "|" + status + "|" + reason;
    }
};

// ----------------------------------------------------------------------------
//  Class Bill : charges of one completed visit
// ----------------------------------------------------------------------------
class Bill {
private:
    int id, apptId, patientId;
    double consult, medicine, lab, paid;

public:
    Bill(int id, int apptId, int patientId, double consult, double medicine, double lab,
         double paid) {
        this->id = id;
        this->apptId = apptId;
        this->patientId = patientId;
        this->consult = consult;
        this->medicine = medicine;
        this->lab = lab;
        this->paid = paid;
    }

    int getId() const { return id; }
    int getApptId() const { return apptId; }
    int getPatientId() const { return patientId; }
    double getConsult() const { return consult; }
    double getMedicine() const { return medicine; }
    double getLab() const { return lab; }
    double getPaid() const { return paid; }

    double total() const { return consult + medicine + lab; }
    double balance() const { return total() - paid; }

    string status() const {                 // Pending, Partial or Paid
        if (balance() <= 0.005) return "Paid";
        if (paid <= 0.005) return "Pending";
        return "Partial";
    }

    void pay(double amount) {
        if (amount <= 0)
            throw invalid_argument("Payment must be greater than zero.");
        if (amount > balance() + 0.005)
            throw invalid_argument("Payment is more than the balance due (Rs." + money(balance()) + ").");
        paid += amount;
    }

    string toFileLine() const {             // id|apptId|patientId|consult|medicine|lab|paid
        return to_string(id) + "|" + to_string(apptId) + "|" + to_string(patientId) + "|" +
               money(consult) + "|" + money(medicine) + "|" + money(lab) + "|" + money(paid);
    }
};

// ----------------------------------------------------------------------------
//  Class Clinic : keeps all the records and does all the work
// ----------------------------------------------------------------------------
class Clinic {
private:
    vector<Patient> patients;
    vector<Doctor> doctors;
    vector<Appointment> appointments;
    vector<Bill> bills;
    int nextPatientId, nextDoctorId, nextApptId, nextBillId;

    // ---- finders: they throw an error if the ID does not exist ----
    Patient& getPatient(int id) {
        for (size_t i = 0; i < patients.size(); i++)
            if (patients[i].getId() == id) return patients[i];
        throw runtime_error("No patient with ID " + to_string(id) + ".");
    }
    Doctor& getDoctor(int id) {
        for (size_t i = 0; i < doctors.size(); i++)
            if (doctors[i].getId() == id) return doctors[i];
        throw runtime_error("No doctor with ID " + to_string(id) + ".");
    }
    Appointment& getAppointment(int id) {
        for (size_t i = 0; i < appointments.size(); i++)
            if (appointments[i].getId() == id) return appointments[i];
        throw runtime_error("No appointment with ID " + to_string(id) + ".");
    }
    Bill& getBill(int id) {
        for (size_t i = 0; i < bills.size(); i++)
            if (bills[i].getId() == id) return bills[i];
        throw runtime_error("No bill with ID " + to_string(id) + ".");
    }

    // is the doctor free on this date at this slot? (cancelled bookings do not count)
    bool isSlotFree(int doctorId, const Date& date, int slot) const {
        for (size_t i = 0; i < appointments.size(); i++) {
            const Appointment& a = appointments[i];
            if (a.getDoctorId() == doctorId && a.getDate() == date && a.getSlot() == slot &&
                a.getStatus() != "Cancelled")
                return false;
        }
        return true;
    }

    // does the patient already have an appointment at that time?
    bool patientBusy(int patientId, const Date& date, int slot) const {
        for (size_t i = 0; i < appointments.size(); i++) {
            const Appointment& a = appointments[i];
            if (a.getPatientId() == patientId && a.getDate() == date && a.getSlot() == slot &&
                a.getStatus() != "Cancelled")
                return true;
        }
        return false;
    }

    // ------------------------------------------------------------------
    //  UNIQUE FEATURE : SMART SLOT SUGGESTION
    //  1) next 3 free slots of the same doctor (rest of the day, then the
    //     next 7 days from 09:00)
    //  2) up to 2 other doctors of the same specialization who are free
    //     at the originally requested time
    // ------------------------------------------------------------------
    void suggestSlots(const Doctor& doc, const Date& date, int slot) const {
        cout << "  Smart suggestions:\n";
        int found = 0;
        Date day = date;
        for (int offset = 0; offset <= 7 && found < 3; offset++) {
            int start = (offset == 0) ? slot + 1 : 0;      // today: only later slots
            for (int s = start; s < SLOTS_PER_DAY && found < 3; s++) {
                if (isSlotFree(doc.getId(), day, s)) {
                    cout << "    Same doctor   Dr. " << doc.getName() << " (ID " << doc.getId()
                         << ")  " << day.toString() << " " << slotToTime(s) << "\n";
                    found++;
                }
            }
            day.nextDay();
        }
        if (found == 0)
            cout << "    No free slot for this doctor in the next 7 days.\n";

        int others = 0;
        for (size_t i = 0; i < doctors.size() && others < 2; i++) {
            const Doctor& other = doctors[i];
            if (other.getId() != doc.getId() &&
                other.getSpecialization() == doc.getSpecialization() &&
                isSlotFree(other.getId(), date, slot)) {
                cout << "    Other doctor  Dr. " << other.getName() << " (ID " << other.getId()
                     << ")  " << date.toString() << " " << slotToTime(slot) << "\n";
                others++;
            }
        }
    }

public:
    Clinic() : nextPatientId(1), nextDoctorId(101), nextApptId(1), nextBillId(1) {
        load();
    }

    // ---------------------------------------------------------------- add
    int addPatient(const string& name, const string& phone, int age, char gender,
                   const string& blood) {
        Patient p(nextPatientId, name, phone, age, gender, blood);   // checks the values
        patients.push_back(p);
        return nextPatientId++;
    }

    int addDoctor(const string& name, const string& phone, const string& spec, double fee) {
        for (size_t i = 0; i < doctors.size(); i++)
            if (doctors[i].getPhone() == phone)
                throw invalid_argument("A doctor with this phone number already exists.");
        Doctor d(nextDoctorId, name, phone, spec, fee);
        doctors.push_back(d);
        return nextDoctorId++;
    }

    // ---------------------------------------------------------- appointments
    // returns the new appointment ID, or 0 if the slot is taken (suggestions are shown)
    int bookAppointment(int patientId, int doctorId, const Date& date, int slot,
                        const string& reason) {
        getPatient(patientId);                              // error if not found
        Doctor& doc = getDoctor(doctorId);

        if (!isSlotFree(doctorId, date, slot)) {
            cout << "  Dr. " << doc.getName() << " is already booked on " << date.toString()
                 << " at " << slotToTime(slot) << ".\n";
            suggestSlots(doc, date, slot);                  // <-- unique feature
            return 0;
        }
        if (patientBusy(patientId, date, slot))
            throw runtime_error("The patient already has another appointment at that time.");

        appointments.push_back(Appointment(nextApptId, patientId, doctorId, date, slot,
                                           "Scheduled", reason));
        return nextApptId++;
    }

    void cancelAppointment(int apptId) {
        Appointment& a = getAppointment(apptId);
        if (a.getStatus() != "Scheduled")
            throw runtime_error("Only Scheduled appointments can be cancelled (this one is " +
                                a.getStatus() + ").");
        a.setStatus("Cancelled");
    }

    // marks the visit completed, adds the diagnosis to the history and creates the bill
    int completeVisit(int apptId, const string& diagnosis, const string& treatment,
                      double medicine, double lab) {
        if (diagnosis.empty())
            throw invalid_argument("Diagnosis cannot be empty.");
        if (medicine < 0 || lab < 0)
            throw invalid_argument("Charges cannot be negative.");

        Appointment& a = getAppointment(apptId);
        if (a.getStatus() != "Scheduled")
            throw runtime_error("Only Scheduled appointments can be completed (this one is " +
                                a.getStatus() + ").");
        Doctor& doc = getDoctor(a.getDoctorId());
        Patient& pat = getPatient(a.getPatientId());

        a.setStatus("Completed");
        pat.addHistory(a.getDate().toString() + ": " + diagnosis + " (" + treatment + ")");
        bills.push_back(Bill(nextBillId, a.getId(), pat.getId(), doc.getFee(), medicine, lab, 0));
        return nextBillId++;
    }

    void payBill(int billId, double amount) {
        Bill& b = getBill(billId);
        b.pay(amount);
        cout << "  Payment recorded. Bill #" << billId << " status: " << b.status() << "\n";
    }

    void printBill(int billId) {
        Bill& b = getBill(billId);
        cout << "  Bill #" << b.getId() << ": consultation " << money(b.getConsult())
             << " + medicine " << money(b.getMedicine()) << " + lab " << money(b.getLab())
             << " = " << money(b.total()) << " (" << b.status() << ")\n";
    }

    // ---------------------------------------------------------------- views
    void showDirectory() {
        // polymorphism: one list holds both doctors and patients
        vector<Person*> everyone;
        for (size_t i = 0; i < doctors.size(); i++) everyone.push_back(&doctors[i]);
        for (size_t i = 0; i < patients.size(); i++) everyone.push_back(&patients[i]);
        if (everyone.empty()) cout << "  (nobody registered yet)\n";
        for (size_t i = 0; i < everyone.size(); i++)
            cout << "  " << *everyone[i] << "\n";
    }

    void showPatientHistory(int patientId) {
        Patient& p = getPatient(patientId);
        cout << "Medical history of " << p << "\n";
        if (p.getHistory().empty()) cout << "  (no completed visits)\n";
        for (size_t i = 0; i < p.getHistory().size(); i++)
            cout << "  - " << p.getHistory()[i] << "\n";
    }

    void showSchedule(int doctorId, const Date& date) {
        Doctor& d = getDoctor(doctorId);
        cout << "Schedule of Dr. " << d.getName() << " (" << d.getSpecialization() << ") on "
             << date.toString() << "\n";
        bool any = false;
        for (int s = 0; s < SLOTS_PER_DAY; s++) {
            for (size_t i = 0; i < appointments.size(); i++) {
                const Appointment& a = appointments[i];
                if (a.getDoctorId() == doctorId && a.getDate() == date && a.getSlot() == s) {
                    cout << "  " << slotToTime(s) << "  Appt #" << left << setw(3) << a.getId()
                         << setw(16) << getPatient(a.getPatientId()).getName() << setw(11)
                         << a.getStatus() << a.getReason() << "\n";
                    any = true;
                }
            }
        }
        if (!any) cout << "  (no appointments)\n";
    }

    void showReports() {
        cout << "REPORT 1: Doctor workload\n";
        cout << left << setw(20) << "Doctor" << setw(18) << "Specialization" << setw(11)
             << "Scheduled" << setw(11) << "Completed" << "Cancelled\n";
        for (size_t i = 0; i < doctors.size(); i++) {
            int sch = 0, comp = 0, canc = 0;
            for (size_t j = 0; j < appointments.size(); j++) {
                if (appointments[j].getDoctorId() != doctors[i].getId()) continue;
                if (appointments[j].getStatus() == "Scheduled") sch++;
                else if (appointments[j].getStatus() == "Completed") comp++;
                else canc++;
            }
            cout << left << setw(20) << ("Dr. " + doctors[i].getName()) << setw(18)
                 << doctors[i].getSpecialization() << setw(11) << sch << setw(11) << comp
                 << canc << "\n";
        }

        cout << "\nREPORT 2: Revenue of each doctor (billed / collected, Rs.)\n";
        for (size_t i = 0; i < doctors.size(); i++) {
            double billed = 0, collected = 0;
            for (size_t j = 0; j < bills.size(); j++) {
                if (getAppointment(bills[j].getApptId()).getDoctorId() == doctors[i].getId()) {
                    billed += bills[j].total();
                    collected += bills[j].getPaid();
                }
            }
            cout << left << setw(20) << ("Dr. " + doctors[i].getName()) << right << setw(10)
                 << money(billed) << setw(12) << money(collected) << "\n";
        }

        cout << "\nREPORT 3: Outstanding bills\n";
        bool any = false;
        for (size_t i = 0; i < bills.size(); i++) {
            if (bills[i].balance() <= 0.005) continue;
            any = true;
            cout << "  Bill #" << bills[i].getId() << "  " << left << setw(14)
                 << getPatient(bills[i].getPatientId()).getName() << setw(9) << bills[i].status()
                 << " total " << right << setw(8) << money(bills[i].total()) << "  paid "
                 << setw(8) << money(bills[i].getPaid()) << "  due " << setw(8)
                 << money(bills[i].balance()) << "\n";
        }
        if (!any) cout << "  (nothing outstanding)\n";
    }

    // ---------------------------------------------------------- file handling
    void save() {
        ofstream f1("patients.txt");
        for (size_t i = 0; i < patients.size(); i++) f1 << patients[i].toFileLine() << "\n";
        ofstream f2("doctors.txt");
        for (size_t i = 0; i < doctors.size(); i++) f2 << doctors[i].toFileLine() << "\n";
        ofstream f3("appointments.txt");
        for (size_t i = 0; i < appointments.size(); i++) f3 << appointments[i].toFileLine() << "\n";
        ofstream f4("bills.txt");
        for (size_t i = 0; i < bills.size(); i++) f4 << bills[i].toFileLine() << "\n";
    }

    void load() {
        string line;
        int lineNo;

        ifstream f1("patients.txt");
        lineNo = 0;
        while (getline(f1, line)) {
            lineNo++;
            if (line.empty()) continue;
            try {
                vector<string> f = splitText(line, '|');
                Patient p(stoi(f[0]), f[1], f[2], stoi(f[3]), f[4][0], f[5]);
                if (!f[6].empty()) {
                    vector<string> h = splitText(f[6], ';');
                    for (size_t i = 0; i < h.size(); i++) p.addHistory(h[i]);
                }
                patients.push_back(p);
                if (p.getId() >= nextPatientId) nextPatientId = p.getId() + 1;
            } catch (const exception&) {
                cout << "Warning: patients.txt line " << lineNo << " is damaged and was skipped.\n";
            }
        }

        ifstream f2("doctors.txt");
        lineNo = 0;
        while (getline(f2, line)) {
            lineNo++;
            if (line.empty()) continue;
            try {
                vector<string> f = splitText(line, '|');
                Doctor d(stoi(f[0]), f[1], f[2], f[3], stod(f[4]));
                doctors.push_back(d);
                if (d.getId() >= nextDoctorId) nextDoctorId = d.getId() + 1;
            } catch (const exception&) {
                cout << "Warning: doctors.txt line " << lineNo << " is damaged and was skipped.\n";
            }
        }

        ifstream f3("appointments.txt");
        lineNo = 0;
        while (getline(f3, line)) {
            lineNo++;
            if (line.empty()) continue;
            try {
                vector<string> f = splitText(line, '|');
                Date d;
                if (!Date::parse(f[3], d)) throw runtime_error("bad date");
                Appointment a(stoi(f[0]), stoi(f[1]), stoi(f[2]), d, stoi(f[4]), f[5], f[6]);
                appointments.push_back(a);
                if (a.getId() >= nextApptId) nextApptId = a.getId() + 1;
            } catch (const exception&) {
                cout << "Warning: appointments.txt line " << lineNo << " is damaged and was skipped.\n";
            }
        }

        ifstream f4("bills.txt");
        lineNo = 0;
        while (getline(f4, line)) {
            lineNo++;
            if (line.empty()) continue;
            try {
                vector<string> f = splitText(line, '|');
                Bill b(stoi(f[0]), stoi(f[1]), stoi(f[2]), stod(f[3]), stod(f[4]), stod(f[5]),
                       stod(f[6]));
                bills.push_back(b);
                if (b.getId() >= nextBillId) nextBillId = b.getId() + 1;
            } catch (const exception&) {
                cout << "Warning: bills.txt line " << lineNo << " is damaged and was skipped.\n";
            }
        }
    }
};

// ----------------------------------------------------------------------------
//  Input helpers (they keep asking until the user types a valid value)
// ----------------------------------------------------------------------------
string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) {                 // input finished (Ctrl+D / end of file)
        cout << "\nInput closed. Goodbye.\n";
        exit(0);
    }
    return cleanText(s);
}

int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t used = 0;
            int value = stoi(s, &used);
            if (used == s.size()) return value;
        } catch (const exception&) {}
        cout << "  Please enter a whole number.\n";
    }
}

double readMoney(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t used = 0;
            double value = stod(s, &used);
            if (used == s.size()) return value;
        } catch (const exception&) {}
        cout << "  Please enter an amount like 250 or 99.50.\n";
    }
}

Date readDate(const string& prompt) {
    string s = readLine(prompt);
    Date d;
    if (!Date::parse(s, d))
        throw invalid_argument("Date must be a real date in YYYY-MM-DD format.");
    return d;
}

int readSlot(const string& prompt) {
    string s = readLine(prompt);
    int slot = timeToSlot(s);
    if (slot < 0)
        throw invalid_argument("Time must be a slot from 09:00 to 16:30 in 30-minute steps.");
    return slot;
}

// ----------------------------------------------------------------------------
//  Menu
// ----------------------------------------------------------------------------
void showMenu() {
    cout << "\n=========== CLINIC APPOINTMENT SYSTEM ===========\n"
            " 1 Add patient            6 Record payment\n"
            " 2 Add doctor             7 Patient history\n"
            " 3 Book appointment       8 Doctor schedule\n"
            " 4 Cancel appointment     9 Reports\n"
            " 5 Complete visit + bill 10 Directory (all people)\n"
            " 0 Exit\n";
}

int main() {
    Clinic clinic;                          // loads the saved records
    int choice;

    do {
        showMenu();
        choice = readInt("Choice: ");

        try {
            if (choice == 1) {
                string name = readLine("Name: ");
                string phone = readLine("Phone (10 digits): ");
                int age = readInt("Age: ");
                string g = readLine("Gender (M/F/O): ");
                string blood = readLine("Blood group (e.g. B+): ");
                int id = clinic.addPatient(name, phone, age, g.empty() ? '?' : g[0], blood);
                cout << "  Patient registered with ID " << id << ".\n";
                clinic.save();
            }
            else if (choice == 2) {
                string name = readLine("Name: ");
                string phone = readLine("Phone (10 digits): ");
                string spec = readLine("Specialization: ");
                double fee = readMoney("Consultation fee: ");
                int id = clinic.addDoctor(name, phone, spec, fee);
                cout << "  Doctor registered with ID " << id << ".\n";
                clinic.save();
            }
            else if (choice == 3) {
                int pid = readInt("Patient ID: ");
                int did = readInt("Doctor ID: ");
                Date date = readDate("Date (YYYY-MM-DD): ");
                int slot = readSlot("Time (HH:MM, 09:00-16:30): ");
                string reason = readLine("Reason: ");
                int id = clinic.bookAppointment(pid, did, date, slot, reason);
                if (id != 0) {
                    cout << "  Appointment booked. ID " << id << ".\n";
                    clinic.save();
                }
            }
            else if (choice == 4) {
                clinic.cancelAppointment(readInt("Appointment ID: "));
                cout << "  Appointment cancelled.\n";
                clinic.save();
            }
            else if (choice == 5) {
                int aid = readInt("Appointment ID: ");
                string diagnosis = readLine("Diagnosis: ");
                string treatment = readLine("Treatment: ");
                double medicine = readMoney("Medicine charge: ");
                double lab = readMoney("Lab charge: ");
                int billId = clinic.completeVisit(aid, diagnosis, treatment, medicine, lab);
                cout << "  Visit completed.\n";
                clinic.printBill(billId);
                clinic.save();
            }
            else if (choice == 6) {
                int bid = readInt("Bill ID: ");
                double amount = readMoney("Amount paid: ");
                clinic.payBill(bid, amount);
                clinic.save();
            }
            else if (choice == 7) {
                clinic.showPatientHistory(readInt("Patient ID: "));
            }
            else if (choice == 8) {
                int did = readInt("Doctor ID: ");
                clinic.showSchedule(did, readDate("Date (YYYY-MM-DD): "));
            }
            else if (choice == 9) {
                clinic.showReports();
            }
            else if (choice == 10) {
                clinic.showDirectory();
            }
            else if (choice == 0) {
                cout << "Goodbye.\n";
            }
            else {
                cout << "  Invalid choice. Please select from the menu.\n";
            }
        } catch (const exception& e) {
            cout << "  Error: " << e.what() << "\n";
        }
    } while (choice != 0);

    return 0;
}
