# Mini SQL-like Customer Database System in C

## Description
A C-based mini database system that simulates SQL-like operations on a customer table.
The program loads initial data from a file and processes command files containing
INSERT, DELETE, UPDATE, and TRUNCATE operations.

It dynamically manages memory, parses complex command strings, and outputs the
database state after each operation.

## Features
- Dynamic customer table with automatic resizing (realloc)
- SQL-like command support:
  - INSERT INTO CUSTOMER
  - DELETE FROM CUSTOMER
  - UPDATE CUSTOMER SET ... WHERE id=...
  - TRUNCATE TABLE CUSTOMER
- Robust string parsing with quote handling
- Enum-based job type modeling
- File-based input and output
- Error handling for malformed update commands

## Concepts & Techniques
- Dynamic arrays and memory management
- Structs and enums
- SQL-like command parsing
- String manipulation and tokenization
- File I/O operations
- Modular program design

## Technologies
- C
- GCC
- Standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`)

## Input Files
- `input.txt` : Initial customer data
- `commands.txt` : SQL-like commands to manipulate the database

### Example Commands
```sql
INSERT INTO CUSTOMER ("John Doe","john@mail.com",0,1,"12.05.1999");
DELETE FROM CUSTOMER WHERE id=3;
UPDATE CUSTOMER SET name="Alice", email_verified=true WHERE id=1;
TRUNCATE TABLE CUSTOMER;
