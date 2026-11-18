
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define main vidyasetu_console_main
#include "main.c"
#undef main

#define PORT 8080
#define REQUEST_SIZE 16384
#define RESPONSE_SIZE 32768
#define REQUESTS_FILE "requests.dat"

typedef struct {
    int requestID;
    int bookID;
    char studentName[50];
} RequestRecord;

/* Escape text before placing it inside JSON */
int jsonEscape(const char *source, char *destination, size_t size)
{
    size_t used = 0;

    for (size_t i = 0; source[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)source[i];

        if (ch == '"' || ch == '\\') {
            if (used + 2 >= size) return 0;
            destination[used++] = '\\';
            destination[used++] = (char)ch;
        }
        else if (ch < 32) {
            if (used + 6 >= size) return 0;
            int written = snprintf(destination + used, size - used,
                                   "\\u%04x", ch);
            if (written < 0 || (size_t)written >= size - used)
                return 0;
            used += (size_t)written;
        }
        else {
            if (used + 1 >= size) return 0;
            destination[used++] = (char)ch;
        }
    }

    destination[used] = '\0';
    return 1;
}

/* Extract a string from JSON */
int extractJsonString(const char *json, const char *key,
                      char *output, size_t outputSize)
{
    char searchKey[100];

    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);

    const char *position = strstr(json, searchKey);
    if (position == NULL) return 0;

    position = strchr(position, ':');
    if (position == NULL) return 0;

    position++;

    while (isspace((unsigned char)*position))
        position++;

    if (*position != '"') return 0;
    position++;

    size_t used = 0;

    while (*position != '\0' && *position != '"') {
        char ch = *position++;

        if (ch == '\\') {
            if (*position == '\0') return 0;

            ch = *position++;

            if (ch == 'n') ch = '\n';
            else if (ch == 'r') ch = '\r';
            else if (ch == 't') ch = '\t';
        }

        if (used + 1 >= outputSize) return 0;
        output[used++] = ch;
    }

    if (*position != '"') return 0;

    output[used] = '\0';
    return 1;
}

/* Extract a positive integer from JSON */
int extractJsonPositiveInt(const char *json, const char *key,
                           int *value)
{
    char searchKey[100];

    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);

    const char *position = strstr(json, searchKey);
    if (position == NULL) return 0;

    position = strchr(position, ':');
    if (position == NULL) return 0;

    position++;

    while (isspace((unsigned char)*position))
        position++;

    char *end;
    long number = strtol(position, &end, 10);

    if (end == position || number <= 0 || number > INT_MAX)
        return 0;

    *value = (int)number;
    return 1;
}

/* Extract semester (1 to 8) */
int extractJsonInt(const char *json, const char *key, int *value)
{
    if (!extractJsonPositiveInt(json, key, value))
        return 0;

    return *value >= 1 && *value <= 8;
}

/* Send HTTP response */
void sendResponse(SOCKET client, int status,
                  const char *statusText, const char *body)
{
    char header[512];

    int length = (int)strlen(body);

    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n\r\n",
        status, statusText, length);

    send(client, header, (int)strlen(header), 0);

    if (length > 0)
        send(client, body, length, 0);
}

/* Save the pending FIFO queue to requests.dat */
int savePendingRequests(void)
{
    FILE *file = fopen(REQUESTS_FILE, "wb");

    if (file == NULL) {
        perror("Could not open requests.dat for saving");
        return 0;
    }

    int savedNextID = nextRequestID;

    if (fwrite(&savedNextID, sizeof(savedNextID), 1, file) != 1) {
        fclose(file);
        return 0;
    }

    Request *current = front;

    while (current != NULL) {
        RequestRecord record;

        memset(&record, 0, sizeof(record));
        record.requestID = current->requestID;
        record.bookID = current->bookID;

        strncpy(record.studentName, current->studentName,
                sizeof(record.studentName) - 1);

        if (fwrite(&record, sizeof(record), 1, file) != 1) {
            fclose(file);
            return 0;
        }

        current = current->next;
    }

    if (fclose(file) != 0) {
        perror("Could not finish saving requests.dat");
        return 0;
    }

    return 1;
}

/* Restore pending requests in FIFO order */
void loadPendingRequests(void)
{
    FILE *file = fopen(REQUESTS_FILE, "rb");

    if (file == NULL) {
        printf("No previous pending-request file found.\n");
        return;
    }

    int savedNextID = 1;

    if (fread(&savedNextID, sizeof(savedNextID), 1, file) != 1 ||
        savedNextID < 1) {
        printf("Invalid requests.dat header; starting with an empty queue.\n");
        fclose(file);
        return;
    }

    int maximumID = 0;
    int validFile = 1;
    RequestRecord record;

    while (fread(&record, sizeof(record), 1, file) == 1) {
        record.studentName[sizeof(record.studentName) - 1] = '\0';

        if (record.requestID <= 0 ||
            record.bookID <= 0 ||
            record.studentName[0] == '\0') {
            validFile = 0;
            break;
        }

        Book *book = findBookByID(record.bookID);

        if (book == NULL || book->status == 0) {
            printf("Skipping stale request %d for book %d.\n",
                   record.requestID, record.bookID);
            continue;
        }

        int duplicate = 0;
        Request *check = front;

        while (check != NULL) {
            if (check->bookID == record.bookID) {
                duplicate = 1;
                break;
            }
            check = check->next;
        }

        if (duplicate) {
            printf("Skipping duplicate request for book %d.\n",
                   record.bookID);
            continue;
        }

        Request *newRequest = (Request *)malloc(sizeof(Request));

        if (newRequest == NULL) {
            printf("Memory allocation failed while restoring requests.\n");
            validFile = 0;
            break;
        }

        newRequest->requestID = record.requestID;
        newRequest->bookID = record.bookID;

        strcpy(newRequest->studentName, record.studentName);
        newRequest->next = NULL;

        if (rear == NULL) {
            front = rear = newRequest;
        }
        else {
            rear->next = newRequest;
            rear = newRequest;
        }

        if (record.requestID > maximumID)
            maximumID = record.requestID;
    }

    if (ferror(file))
        validFile = 0;

    fclose(file);

    if (maximumID >= savedNextID) {
        if (maximumID == INT_MAX) {
            printf("Request ID limit reached.\n");
            nextRequestID = INT_MAX;
        }
        else {
            nextRequestID = maximumID + 1;
        }
    }
    else {
        nextRequestID = savedNextID;
    }

    int count = 0;
    Request *current = front;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    printf("Pending requests restored: %d\n", count);

    if (!validFile)
        printf("Warning: requests.dat may be incomplete or damaged.\n");
}

/* Build JSON containing all books */
int getBooksJSON(char *body, size_t size)
{
    Book *current = head;
    size_t used = 0;
    int count = 0;

    int written = snprintf(body, size,
        "{\"status\":\"success\",\"books\":[");

    if (written < 0 || (size_t)written >= size)
        return 0;

    used = (size_t)written;

    while (current != NULL) {
        char title[300], author[200];
        char subject[200], branch[100];

        if (!jsonEscape(current->title, title, sizeof(title)) ||
            !jsonEscape(current->author, author, sizeof(author)) ||
            !jsonEscape(current->subject, subject, sizeof(subject)) ||
            !jsonEscape(current->branch, branch, sizeof(branch)))
            return 0;

        written = snprintf(body + used, size - used,
            "%s{\"bookID\":%d,\"title\":\"%s\","
            "\"author\":\"%s\",\"subject\":\"%s\","
            "\"branch\":\"%s\",\"semester\":%d,"
            "\"status\":\"%s\"}",
            count > 0 ? "," : "",
            current->bookID, title, author, subject, branch,
            current->semester,
            current->status == 1 ? "Available" : "Unavailable");

        if (written < 0 || (size_t)written >= size - used)
            return 0;

        used += (size_t)written;
        count++;
        current = current->next;
    }

    written = snprintf(body + used, size - used, "]}");

    return written >= 0 && (size_t)written < size - used;
}

/* Add a donated book to the linked list */
void handleBookDonation(SOCKET client, const char *json)
{
    char title[100], donor[50], subject[50];
    char branch[20] = "General";
    int semester;

    if (!extractJsonString(json, "title", title, sizeof(title)) ||
        !extractJsonString(json, "donorName", donor, sizeof(donor)) ||
        !extractJsonString(json, "subject", subject, sizeof(subject)) ||
        !extractJsonInt(json, "semester", &semester)) {

        sendResponse(client, 400, "Bad Request",
            "{\"status\":\"error\",\"message\":\"Invalid donation details\"}");
        return;
    }

    if (title[0] == '\0' || donor[0] == '\0' || subject[0] == '\0') {
        sendResponse(client, 400, "Bad Request",
            "{\"status\":\"error\",\"message\":\"Please fill all required fields\"}");
        return;
    }

    int newID = 1;
    Book *current = head;

    while (current != NULL) {
        if (current->bookID >= newID) {
            if (current->bookID == INT_MAX) {
                sendResponse(client, 500, "Internal Server Error",
                    "{\"status\":\"error\",\"message\":\"Book ID limit reached\"}");
                return;
            }
            newID = current->bookID + 1;
        }
        current = current->next;
    }

    Book *newBook = (Book *)malloc(sizeof(Book));

    if (newBook == NULL) {
        sendResponse(client, 500, "Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Memory allocation failed\"}");
        return;
    }

    newBook->bookID = newID;
    strcpy(newBook->title, title);
    strcpy(newBook->author, donor);
    strcpy(newBook->subject, subject);
    strcpy(newBook->branch, branch);
    newBook->semester = semester;
    newBook->status = 1;
    newBook->next = NULL;

    if (head == NULL) {
        head = newBook;
    }
    else {
        current = head;

        while (current->next != NULL)
            current = current->next;

        current->next = newBook;
    }

    saveBooks();

    char response[512];

    snprintf(response, sizeof(response),
        "{\"status\":\"success\","
        "\"message\":\"Book donated successfully\","
        "\"bookID\":%d}", newID);

    sendResponse(client, 201, "Created", response);

    printf("New book donated: ID %d - %s\n", newID, title);
}

/* Add a student request to the FIFO queue */
void handleBookRequest(SOCKET client, const char *json)
{
    int bookID;
    char studentName[50];

    if (!extractJsonPositiveInt(json, "bookID", &bookID) ||
        !extractJsonString(json, "studentName",
                           studentName, sizeof(studentName)) ||
        studentName[0] == '\0') {

        sendResponse(client, 400, "Bad Request",
            "{\"status\":\"error\",\"message\":\"Invalid request details\"}");
        return;
    }

    Book *book = findBookByID(bookID);

    if (book == NULL) {
        sendResponse(client, 404, "Not Found",
            "{\"status\":\"error\",\"message\":\"Book ID not found\"}");
        return;
    }

    if (book->status == 0) {
        sendResponse(client, 409, "Conflict",
            "{\"status\":\"error\",\"message\":\"This book is unavailable\"}");
        return;
    }

    Request *current = front;

    while (current != NULL) {
        if (current->bookID == bookID) {
            sendResponse(client, 409, "Conflict",
                "{\"status\":\"error\",\"message\":\"A request for this book is already pending\"}");
            return;
        }
        current = current->next;
    }

    if (nextRequestID <= 0 || nextRequestID == INT_MAX) {
        sendResponse(client, 500, "Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Request ID limit reached\"}");
        return;
    }

    Request *newRequest = (Request *)malloc(sizeof(Request));

    if (newRequest == NULL) {
        sendResponse(client, 500, "Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Memory allocation failed\"}");
        return;
    }

    newRequest->requestID = nextRequestID++;
    newRequest->bookID = bookID;
    strcpy(newRequest->studentName, studentName);
    newRequest->next = NULL;

    if (rear == NULL) {
        front = rear = newRequest;
    }
    else {
        rear->next = newRequest;
        rear = newRequest;
    }

    if (!savePendingRequests()) {
        if (front == newRequest) {
            front = rear = NULL;
        }
        else {
            Request *previous = front;

            while (previous != NULL && previous->next != newRequest)
                previous = previous->next;

            if (previous != NULL) {
                previous->next = NULL;
                rear = previous;
            }
        }

        nextRequestID--;
        free(newRequest);

        sendResponse(client, 500, "Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Could not save the request. Please try again.\"}");
        return;
    }

    char response[512];

    snprintf(response, sizeof(response),
        "{\"status\":\"success\","
        "\"message\":\"Book request added successfully\","
        "\"requestID\":%d,\"bookID\":%d}",
        newRequest->requestID, bookID);

    sendResponse(client, 201, "Created", response);

    printf("Request added and saved: ID %d, Student: %s, Book: %d\n",
        newRequest->requestID, studentName, bookID);
}

/* Return a borrowed book and make it available again */
void handleBookReturn(SOCKET client, const char *json)
{
    int bookID;

    if (!extractJsonPositiveInt(json, "bookID", &bookID)) {
        sendResponse(client, 400, "Bad Request",
            "{\"status\":\"error\",\"message\":\"Please provide a valid Book ID\"}");
        return;
    }

    Book *book = findBookByID(bookID);

    if (book == NULL) {
        sendResponse(client, 404, "Not Found",
            "{\"status\":\"error\",\"message\":\"Book ID not found\"}");
        return;
    }

    if (book->status == 1) {
        sendResponse(client, 409, "Conflict",
            "{\"status\":\"error\",\"message\":\"This book is already available\"}");
        return;
    }

    /* Change the book status back to Available. */
    book->status = 1;

    /* Save the updated status in books.dat. */
    saveBooks();

    char response[512];

    snprintf(response, sizeof(response),
        "{\"status\":\"success\","
        "\"message\":\"Book returned successfully. It is now available.\","
        "\"bookID\":%d,\"bookAvailable\":true}",
        bookID);

    sendResponse(client, 200, "OK", response);

    printf("Book returned: ID %d - %s\n", bookID, book->title);
}

/* Display all pending requests as JSON */
int getRequestsJSON(char *body, size_t size)
{
    Request *current = front;
    size_t used = 0;
    int count = 0;

    int written = snprintf(body, size,
        "{\"status\":\"success\",\"requests\":[");

    if (written < 0 || (size_t)written >= size)
        return 0;

    used = (size_t)written;

    while (current != NULL) {
        char student[200];
        char title[300] = "";

        Book *book = findBookByID(current->bookID);

        if (!jsonEscape(current->studentName,
                        student, sizeof(student)))
            return 0;

        if (book != NULL &&
            !jsonEscape(book->title, title, sizeof(title)))
            return 0;

        written = snprintf(body + used, size - used,
            "%s{\"requestID\":%d,\"bookID\":%d,"
            "\"studentName\":\"%s\",\"bookTitle\":\"%s\"}",
            count > 0 ? "," : "",
            current->requestID, current->bookID,
            student, title);

        if (written < 0 || (size_t)written >= size - used)
            return 0;

        used += (size_t)written;
        count++;
        current = current->next;
    }

    written = snprintf(body + used, size - used, "]}");

    return written >= 0 && (size_t)written < size - used;
}

/* Process the oldest request from the front */
void handleProcessRequest(SOCKET client)
{
    if (front == NULL) {
        sendResponse(client, 404, "Not Found",
            "{\"status\":\"error\",\"message\":\"No pending requests to process\"}");
        return;
    }

    Request *oldest = front;
    Book *book = findBookByID(oldest->bookID);

    if (book == NULL || book->status == 0) {
        sendResponse(client, 409, "Conflict",
            "{\"status\":\"error\",\"message\":\"The requested book is missing or unavailable\"}");
        return;
    }

    int requestID = oldest->requestID;
    int bookID = oldest->bookID;

    char student[200] = "";
    char title[300] = "";

    if (!jsonEscape(oldest->studentName, student, sizeof(student)) ||
        !jsonEscape(book->title, title, sizeof(title))) {
        sendResponse(client, 500, "Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Could not prepare request response\"}");
        return;
    }

    book->status = 0;

    front = oldest->next;
    if (front == NULL)
        rear = NULL;

    free(oldest);

    saveBooks();

    if (!savePendingRequests()) {
        printf("WARNING: Could not save the updated pending queue.\n");
    }

    char response[1024];

    snprintf(response, sizeof(response),
        "{\"status\":\"success\","
        "\"message\":\"Oldest request processed successfully\","
        "\"requestID\":%d,\"bookID\":%d,"
        "\"studentName\":\"%s\",\"bookTitle\":\"%s\","
        "\"bookAvailable\":false}",
        requestID, bookID, student, title);

    sendResponse(client, 200, "OK", response);

    printf("Processed request %d for student %s\n",
        requestID, student);
}

/* Start the HTTP server */
int main(void)
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddress;

    char request[REQUEST_SIZE];
    char body[RESPONSE_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock initialization failed.\n");
        return 1;
    }

    loadBooks();
    loadPendingRequests();

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Port binding failed. Is port 8080 already in use?\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        printf("Server listen failed.\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("\n====================================\n");
    printf(" VIDYASETU C BACKEND SERVER\n");
    printf("====================================\n");
    printf("Server: http://127.0.0.1:8080\n");
    printf("Books API: GET /api/books\n");
    printf("Donation API: POST /api/books\n");
    printf("Return Book: POST /api/books/return\n");
    printf("Request API: POST /api/requests\n");
    printf("View Requests: GET /api/requests\n");
    printf("Process Queue: POST /api/requests/process\n");
    printf("Persistent queue file: %s\n\n", REQUESTS_FILE);

    while (1) {
        clientSocket = accept(serverSocket, NULL, NULL);

        if (clientSocket == INVALID_SOCKET)
            continue;

        memset(request, 0, sizeof(request));

        int received = 0;
        int expectedLength = -1;
        char *headerEnd = NULL;

        while (received < REQUEST_SIZE - 1) {
            int bytes = recv(clientSocket,
                             request + received,
                             REQUEST_SIZE - 1 - received, 0);

            if (bytes <= 0)
                break;

            received += bytes;
            request[received] = '\0';

            if (headerEnd == NULL) {
                headerEnd = strstr(request, "\r\n\r\n");

                if (headerEnd != NULL) {
                    int contentLength = 0;
                    char *lengthHeader =
                        strstr(request, "Content-Length:");

                    if (lengthHeader != NULL) {
                        sscanf(lengthHeader,
                               "Content-Length: %d",
                               &contentLength);
                    }

                    if (contentLength < 0) {
                        expectedLength = -1;
                        break;
                    }

                    expectedLength =
                        (int)(headerEnd - request) + 4 +
                        contentLength;

                    if (expectedLength >= REQUEST_SIZE) {
                        expectedLength = -1;
                        break;
                    }
                }
            }

            if (headerEnd != NULL &&
                expectedLength >= 0 &&
                received >= expectedLength)
                break;
        }

        if (received > 0 && headerEnd != NULL &&
            expectedLength >= 0 && received >= expectedLength) {

            char *jsonBody = headerEnd + 4;

            if (strncmp(request, "OPTIONS ", 8) == 0) {
                sendResponse(clientSocket, 204, "No Content", "");
            }
            else if (strncmp(request, "GET /api/health ", 16) == 0) {
                sendResponse(clientSocket, 200, "OK",
                    "{\"status\":\"success\",\"message\":\"VidyaSetu C backend is running\"}");
            }
            else if (strncmp(request, "GET /api/books ", 15) == 0) {
                if (getBooksJSON(body, sizeof(body)))
                    sendResponse(clientSocket, 200, "OK", body);
                else
                    sendResponse(clientSocket, 500,
                        "Internal Server Error",
                        "{\"status\":\"error\",\"message\":\"Could not create book response\"}");
            }
            else if (strncmp(request,
                             "POST /api/books/return ", 23) == 0) {
                handleBookReturn(clientSocket, jsonBody);
            }
            else if (strncmp(request, "POST /api/books ", 16) == 0) {
                handleBookDonation(clientSocket, jsonBody);
            }
            else if (strncmp(request,
                             "POST /api/requests/process ", 27) == 0) {
                handleProcessRequest(clientSocket);
            }
            else if (strncmp(request, "POST /api/requests ", 19) == 0) {
                handleBookRequest(clientSocket, jsonBody);
            }
            else if (strncmp(request, "GET /api/requests ", 18) == 0) {
                if (getRequestsJSON(body, sizeof(body)))
                    sendResponse(clientSocket, 200, "OK", body);
                else
                    sendResponse(clientSocket, 500,
                        "Internal Server Error",
                        "{\"status\":\"error\",\"message\":\"Could not create request response\"}");
            }
            else {
                sendResponse(clientSocket, 404, "Not Found",
                    "{\"status\":\"error\",\"message\":\"Route not found\"}");
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
