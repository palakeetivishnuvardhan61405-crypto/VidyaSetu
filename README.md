
# VidyaSetu — Knowledge Without Barriers

VidyaSetu is a student-focused book donation and sharing platform designed to make educational resources accessible to students. It allows students to donate books, explore available books, and request books shared by others.

The project uses a C backend with Data Structures and Algorithms, connected to a web-based frontend.

## Problem Statement

Many students have unused textbooks and reference materials, while other students may struggle to access affordable learning resources. VidyaSetu provides a common platform where students can share and request educational books.

## Objectives

- Encourage book reuse and sharing among students.
- Reduce educational resource wastage.
- Help students access books at no additional cost.
- Demonstrate practical applications of Data Structures in C.

## Features

- Donate educational books.
- View all donated books.
- Search and filter books.
- Display only available books in the request dropdown.
- Submit book requests.
- Prevent duplicate pending requests for the same book.
- Process requests in FIFO order.
- Save books and pending requests using file handling.
- Maintain book availability after processing requests.

## Technologies Used

**Frontend**
- HTML
- CSS
- JavaScript

**Backend**
- C Programming
- Windows Sockets (Winsock)
- HTTP API

**Data Storage**
- Binary file handling (`books.dat`, `requests.dat`)

## Data Structures Used

### 1. Linked List
Used to maintain the collection of books dynamically.

### 2. Queue (FIFO)
Used to manage pending book requests. The first request received is processed first.

### 3. File Handling
Used to preserve book records and pending requests across server restarts.

## Project Structure

```text
VIDYASETU/
│
├── main.c
├── server.c
├── vidyasetu_server.exe
├── index.html
├── style.css
├── script.js
├── books.dat
├── requests.dat
└── README.md
```

## How to Run

### Requirements
- Windows operating system
- GCC compiler
- Web browser

### Step 1: Open the project folder

Open the VIDYASETU folder in PowerShell.

### Step 2: Compile the C server

```powershell
gcc server.c -o vidyasetu_server.exe -lws2_32
```

### Step 3: Start the server

```powershell
.\vidyasetu_server.exe
```

### Step 4: Open the website

Open `index.html` in your browser using VS Code Live Server or directly in your browser.

Keep the C server running while using the website.

## API Endpoints

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | Check server status |
| GET | `/api/books` | Retrieve books |
| POST | `/api/books` | Donate a book |
| GET | `/api/requests` | View pending requests |
| POST | `/api/requests` | Submit a book request |
| POST | `/api/requests/process` | Process oldest request |

Base URL: `http://127.0.0.1:8080/api`

## Social Impact

VidyaSetu promotes educational equality, resource sharing, sustainability, and collaboration among students.

## Future Enhancements

- Student authentication.
- QR-based book identification.
- Advanced book recommendation.
- Notifications for book requests.
- Administrative dashboard.

## Project Status

Working academic project prototype. Further enhancements are planned.

---

**VidyaSetu — Knowledge Without Barriers**
