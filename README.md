# Clinic Appointment System (C++)

OOPS (C++) PBL / Mini Project - Chandigarh University
**Author:** Mbafor Joel Afese (UID 25BCD10002, Section 25BCD-3B)
**Unique feature:** Smart Slot Suggestion

A console program to manage patients, doctors, appointments, visits and bills.
When a doctor's slot is already booked, the program suggests the next free slots of that doctor
(continuing into the next days) and other doctors of the same specialization who are free at that time.

## Compile and run
```
g++ main.cpp -o clinic
./clinic            (on Windows: clinic.exe)
```
Needs a C++11 (or later) compiler. Records are saved in `patients.txt`, `doctors.txt`,
`appointments.txt` and `bills.txt` in the program folder.

## Menu
1 Add patient, 2 Add doctor, 3 Book appointment, 4 Cancel appointment, 5 Complete visit + bill,
6 Record payment, 7 Patient history, 8 Doctor schedule, 9 Reports, 10 Directory, 0 Exit

## OOP concepts used
| Concept | Where |
|---|---|
| Classes and objects, constructors | Date, Person, Patient, Doctor, Appointment, Bill, Clinic |
| Abstraction | `Person` has pure virtual `describe()` and `toFileLine()` |
| Inheritance | `Patient` and `Doctor` inherit from `Person` |
| Polymorphism | `showDirectory()` keeps patients and doctors in one `vector<Person*>` |
| Encapsulation | private / protected data with getter functions |
| Operator overloading | `operator<<` for `Person`, `operator==` for `Date` |
| Exception handling | constructors throw errors, `main()` catches them |
| File handling | `Clinic::save()` and `Clinic::load()` |

## Rules checked by the program
- A doctor cannot have two active bookings at the same date and time (a cancelled slot can be booked again).
- A patient cannot have two active bookings at the same date and time.
- Slots are 09:00 to 16:30 in 30-minute steps; dates must be real (YYYY-MM-DD).
- Only Scheduled appointments can be cancelled or completed; one bill per completed visit.
- A payment must be positive and not more than the balance. Fees and charges cannot be negative.
