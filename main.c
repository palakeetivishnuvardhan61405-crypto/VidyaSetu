
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define BOOK_FILE "books.dat"

/* Book record stored in memory */
typedef struct Book {
    int bookID;
    char title[100];
    char author[50];
    char subject[50];
    char branch[20];
    int semester;
    int status;
    struct Book *next;
} Book;

/* Request queue */
typedef struct Request {
    int requestID;
    int bookID;
    char studentName[50];
    struct Request *next;
} Request;

/* File record: deliberately contains no linked-list pointer */
typedef struct {
    int bookID;
    char title[100];
    char author[50];
    char subject[50];
    char branch[20];
    int semester;
    int status;
} BookRecord;

Book *head = NULL;
Request *front = NULL;
Request *rear = NULL;
int nextRequestID = 1;

/* Input an integer */
int readInteger(const char *prompt, int *value) {
    char line[100];
    char *end;
    long number;

    printf("%s", prompt);

    if (fgets(line, sizeof(line), stdin) == NULL)
        return 0;

    if (strchr(line, '\n') == NULL) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }

    number = strtol(line, &end, 10);

    if (end == line)
        return 0;

    while (isspace((unsigned char)*end))
        end++;

    if (*end != '\0' && *end != '\n')
        return 0;

    if (number < INT_MIN || number > INT_MAX)
        return 0;

    *value = (int)number;
    return 1;
}

/* Input a non-empty string */
int readString(const char *prompt, char *value, size_t size) {
    size_t length;
    int ch;

    printf("%s", prompt);

    if (fgets(value, (int)size, stdin) == NULL)
        return 0;

    length = strlen(value);

    if (length > 0 && value[length - 1] == '\n') {
        value[length - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }

    if (value[0] == '\0') {
        printf("Input cannot be empty.\n");
        return 0;
    }

    return 1;
}

/* Save book records to a file */
void saveBooks(void) {
    FILE *file = fopen(BOOK_FILE, "wb");
    Book *current = head;

    if (file == NULL) {
        printf("Warning: Could not save books to file.\n");
        return;
    }

    while (current != NULL) {
        BookRecord record;

        record.bookID = current->bookID;
        strcpy(record.title, current->title);
        strcpy(record.author, current->author);
        strcpy(record.subject, current->subject);
        strcpy(record.branch, current->branch);
        record.semester = current->semester;
        record.status = current->status;

        if (fwrite(&record, sizeof(record), 1, file) != 1) {
            printf("Warning: Error while saving book records.\n");
            fclose(file);
            return;
        }

        current = current->next;
    }

    fclose(file);
}

/* Load previously saved books */
void loadBooks(void) {
    FILE *file = fopen(BOOK_FILE, "rb");
    BookRecord record;

    if (file == NULL) {
        /* First run: the file may not exist yet */
        return;
    }

    while (fread(&record, sizeof(record), 1, file) == 1) {
        Book *newBook = malloc(sizeof(Book));

        if (newBook == NULL) {
            printf("Memory allocation failed while loading books.\n");
            break;
        }

        newBook->bookID = record.bookID;
        strcpy(newBook->title, record.title);
        strcpy(newBook->author, record.author);
        strcpy(newBook->subject, record.subject);
        strcpy(newBook->branch, record.branch);
        newBook->semester = record.semester;
        newBook->status = record.status;
        newBook->next = NULL;

        if (head == NULL) {
            head = newBook;
        } else {
            Book *current = head;

            while (current->next != NULL)
                current = current->next;

            current->next = newBook;
        }
    }

    fclose(file);
    printf("Saved book records loaded successfully.\n");
}

/* Find a book by ID */
Book *findBookByID(int id) {
    Book *current = head;

    while (current != NULL) {
        if (current->bookID == id)
            return current;

        current = current->next;
    }

    return NULL;
}

/* Check for duplicate IDs */
int bookIDExists(int id) {
    return findBookByID(id) != NULL;
}

/* Case-insensitive partial search */
int containsIgnoreCase(const char *text, const char *pattern) {
    size_t i, j;

    if (pattern[0] == '\0')
        return 0;

    for (i = 0; text[i] != '\0'; i++) {
        for (j = 0; pattern[j] != '\0'; j++) {
            if (text[i + j] == '\0' ||
                tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)pattern[j]))
                break;
        }

        if (pattern[j] == '\0')
            return 1;
    }

    return 0;
}

/* Display one book */
void printBook(const Book *book) {
    printf("\nBook ID : %d\n", book->bookID);
    printf("Title   : %s\n", book->title);
    printf("Author  : %s\n", book->author);
    printf("Subject : %s\n", book->subject);
    printf("Branch  : %s\n", book->branch);
    printf("Semester: %d\n", book->semester);
    printf("Status  : %s\n",
           book->status == 1 ? "Available" : "Unavailable");
    printf("------------------------------\n");
}

/* Donate a book */
void addBook(void) {
    Book *newBook;
    Book *current;
    int id;

    newBook = malloc(sizeof(Book));

    if (newBook == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    if (!readInteger("Enter Book ID: ", &id) || id <= 0) {
        printf("Enter a valid positive Book ID.\n");
        free(newBook);
        return;
    }

    if (bookIDExists(id)) {
        printf("This Book ID already exists.\n");
        free(newBook);
        return;
    }

    newBook->bookID = id;

    if (!readString("Enter book title: ",
                    newBook->title, sizeof(newBook->title)) ||
        !readString("Enter author name: ",
                    newBook->author, sizeof(newBook->author)) ||
        !readString("Enter subject: ",
                    newBook->subject, sizeof(newBook->subject)) ||
        !readString("Enter branch: ",
                    newBook->branch, sizeof(newBook->branch))) {
        printf("Book was not added. Please try again.\n");
        free(newBook);
        return;
    }

    if (!readInteger("Enter semester (1-8): ",
                     &newBook->semester) ||
        newBook->semester < 1 || newBook->semester > 8) {
        printf("Invalid semester. Enter a number from 1 to 8.\n");
        free(newBook);
        return;
    }

    newBook->status = 1;
    newBook->next = NULL;

    if (head == NULL) {
        head = newBook;
    } else {
        current = head;

        while (current->next != NULL)
            current = current->next;

        current->next = newBook;
    }

    saveBooks();
    printf("Book donated and saved successfully!\n");
}

/* Display all books */
void displayBooks(void) {
    Book *current = head;

    if (head == NULL) {
        printf("No books available.\n");
        return;
    }

    while (current != NULL) {
        printBook(current);
        current = current->next;
    }
}

/* Search by title or subject */
void searchBooks(void) {
    int choice, found = 0;
    char keyword[100];
    Book *current = head;

    if (head == NULL) {
        printf("No books available to search.\n");
        return;
    }

    printf("1. Search by title\n");
    printf("2. Search by subject\n");

    if (!readInteger("Enter choice: ", &choice) ||
        (choice != 1 && choice != 2)) {
        printf("Invalid choice.\n");
        return;
    }

    if (!readString("Enter search keyword: ",
                    keyword, sizeof(keyword))) {
        printf("Search cancelled.\n");
        return;
    }

    while (current != NULL) {
        int match;

        if (choice == 1)
            match = containsIgnoreCase(current->title, keyword);
        else
            match = containsIgnoreCase(current->subject, keyword);

        if (match) {
            printBook(current);
            found = 1;
        }

        current = current->next;
    }

    if (!found)
        printf("No matching books found.\n");
}

/* Delete a book */
void deleteBook(void) {
    int id;
    Book *current = head;
    Book *previous = NULL;
    Request *r = front;

    if (head == NULL) {
        printf("No books available to delete.\n");
        return;
    }

    if (!readInteger("Enter Book ID to delete: ", &id)) {
        printf("Invalid Book ID.\n");
        return;
    }

    while (r != NULL) {
        if (r->bookID == id) {
            printf("Cannot delete: this book has a pending request.\n");
            return;
        }
        r = r->next;
    }

    while (current != NULL && current->bookID != id) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Book ID not found.\n");
        return;
    }

    if (previous == NULL)
        head = current->next;
    else
        previous->next = current->next;

    free(current);
    saveBooks();

    printf("Book deleted and changes saved.\n");
}

/* Add a book request to the FIFO queue */
void requestBook(void) {
    int id;
    Book *book;
    Request *newRequest;
    Request *current;

    if (!readInteger("Enter the Book ID to request: ", &id)) {
        printf("Invalid Book ID.\n");
        return;
    }

    book = findBookByID(id);

    if (book == NULL) {
        printf("Book ID not found.\n");
        return;
    }

    if (book->status == 0) {
        printf("This book is currently unavailable.\n");
        return;
    }

    current = front;

    while (current != NULL) {
        if (current->bookID == id) {
            printf("A request for this book is already pending.\n");
            return;
        }

        current = current->next;
    }

    newRequest = malloc(sizeof(Request));

    if (newRequest == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    if (!readString("Enter student name: ",
                    newRequest->studentName,
                    sizeof(newRequest->studentName))) {
        printf("Request was not added.\n");
        free(newRequest);
        return;
    }

    newRequest->requestID = nextRequestID++;
    newRequest->bookID = id;
    newRequest->next = NULL;

    if (rear == NULL) {
        front = rear = newRequest;
    } else {
        rear->next = newRequest;
        rear = newRequest;
    }

    printf("Book request added successfully!\n");
    printf("Request ID: %d\n", newRequest->requestID);
}

/* Display pending requests */
void displayRequests(void) {
    Request *current = front;

    if (front == NULL) {
        printf("No pending requests.\n");
        return;
    }

    while (current != NULL) {
        Book *book = findBookByID(current->bookID);

        printf("\nRequest ID: %d\n", current->requestID);
        printf("Student: %s\n", current->studentName);
        printf("Book ID: %d\n", current->bookID);

        if (book != NULL)
            printf("Book title: %s\n", book->title);

        current = current->next;
    }
}

/* Process the oldest request */
void processRequest(void) {
    Request *oldest;
    Book *book;

    if (front == NULL) {
        printf("No pending requests to process.\n");
        return;
    }

    oldest = front;
    book = findBookByID(oldest->bookID);

    printf("\nProcessing oldest request...\n");
    printf("Request ID: %d\n", oldest->requestID);
    printf("Student: %s\n", oldest->studentName);

    if (book != NULL) {
        printf("Book: %s\n", book->title);
        book->status = 0;
        saveBooks();
        printf("Book marked unavailable and saved.\n");
    } else {
        printf("Requested book no longer exists.\n");
    }

    front = front->next;

    if (front == NULL)
        rear = NULL;

    free(oldest);
}

/* Free memory used by books */
void freeBooks(void) {
    Book *current = head;

    while (current != NULL) {
        Book *temp = current;
        current = current->next;
        free(temp);
    }

    head = NULL;
}

/* Free memory used by requests */
void freeRequests(void) {
    Request *current = front;

    while (current != NULL) {
        Request *temp = current;
        current = current->next;
        free(temp);
    }

    front = rear = NULL;
}

int main(void) {
    int choice = 0;

    printf("========================================\n");
    printf(" VIDYASETU - KNOWLEDGE WITHOUT BARRIERS\n");
    printf("========================================\n");

    loadBooks();

    do {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. Donate a book\n");
        printf("2. Display all books\n");
        printf("3. Search books\n");
        printf("4. Delete a book\n");
        printf("5. Request a book\n");
        printf("6. View pending requests\n");
        printf("7. Process oldest request\n");
        printf("8. Exit\n");

        if (!readInteger("Enter your choice: ", &choice)) {
            printf("Invalid input. Enter a number from 1 to 8.\n");
            choice = 0;
            continue;
        }

        switch (choice) {
            case 1: addBook(); break;
            case 2: displayBooks(); break;
            case 3: searchBooks(); break;
            case 4: deleteBook(); break;
            case 5: requestBook(); break;
            case 6: displayRequests(); break;
            case 7: processRequest(); break;

            case 8:
                saveBooks();
                freeBooks();
                freeRequests();
                printf("Thank you for using VIDYASETU!\n");
                break;

            default:
                printf("Invalid choice. Select 1 to 8.\n");
        }

    } while (choice != 8);

    return 0;
}
