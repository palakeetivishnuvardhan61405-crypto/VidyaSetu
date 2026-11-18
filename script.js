
"use strict";

// ========================================
// 1. C BACKEND CONNECTION
// ========================================

const API_URL = "http://127.0.0.1:8080/api";

let books = [];
let requests = [];
let toastTimeout;


// ========================================
// 2. DOM ELEMENTS
// ========================================

const bookGrid = document.getElementById("bookGrid");
const searchInput = document.getElementById("searchInput");
const searchButton = document.getElementById("searchButton");
const semesterFilter = document.getElementById("semesterFilter");
const bookCount = document.getElementById("bookCount");
const emptyState = document.getElementById("emptyState");

const donationForm = document.getElementById("donationForm");

const requestForm = document.getElementById("requestForm");
const requestBook = document.getElementById("requestBook");

const returnForm = document.getElementById("returnForm");
const returnBook = document.getElementById("returnBook");

const totalBooksElement = document.getElementById("totalBooks");
const availableBooksElement = document.getElementById("availableBooks");
const totalRequestsElement = document.getElementById("totalRequests");

const toast = document.getElementById("toast");


// ========================================
// 3. TOAST MESSAGE
// ========================================

function showToast(message) {

    if (!toast) {
        alert(message);
        return;
    }

    toast.textContent = message;
    toast.classList.add("show");

    clearTimeout(toastTimeout);

    toastTimeout = setTimeout(() => {
        toast.classList.remove("show");
    }, 3000);
}


// ========================================
// 4. STATISTICS
// ========================================

function updateStatistics() {

    if (totalBooksElement) {
        totalBooksElement.textContent = books.length;
    }

    if (availableBooksElement) {
        availableBooksElement.textContent =
            books.filter(book => book.available).length;
    }

    if (totalRequestsElement) {
        totalRequestsElement.textContent = requests.length;
    }
}


// ========================================
// 5. LOAD BOOKS FROM C BACKEND
// ========================================

async function loadBooksFromBackend(showMessage = false) {

    try {

        const response = await fetch(`${API_URL}/books`);
        const data = await response.json();

        if (!response.ok || data.status !== "success") {
            throw new Error(data.message || "Could not load books.");
        }

        books = data.books.map(book => ({
            id: book.bookID,
            title: book.title,
            subject: book.subject,
            semester: book.semester,
            condition: "Not specified",
            donor: book.author,
            available: book.status === "Available"
        }));

        renderBooks();
        updateRequestOptions();
        updateReturnOptions();
        updateStatistics();

        if (showMessage) {
            showToast("Books loaded successfully!");
        }

    } catch (error) {

        console.error("Could not load books:", error);

        showToast(
            "Could not connect to C backend. Start the server first."
        );
    }
}


// ========================================
// 6. LOAD PENDING REQUESTS FROM C BACKEND
// ========================================

async function loadRequestsFromBackend(showMessage = false) {

    try {

        const response = await fetch(`${API_URL}/requests`);
        const data = await response.json();

        if (!response.ok || data.status !== "success") {
            throw new Error(data.message || "Could not load requests.");
        }

        requests = data.requests.map(request => ({
            requestId: request.requestID,
            bookId: request.bookID,
            student: request.studentName,
            bookTitle: request.bookTitle,
            status: "Pending"
        }));

        renderRequests();
        updateStatistics();

        if (showMessage) {
            showToast("Pending requests refreshed.");
        }

    } catch (error) {

        console.error("Could not load requests:", error);
        showToast("Could not load pending requests.");
    }
}


// ========================================
// 7. EXPLORE BOOKS PAGE
// ========================================

function renderBooks() {

    // Run only on Explore Books page.
    if (!bookGrid) return;

    const searchTerm = searchInput.value.trim().toLowerCase();
    const selectedSemester = semesterFilter.value;

    const filteredBooks = books.filter(book => {

        const searchableText = [
            book.title,
            book.subject,
            book.donor
        ].join(" ").toLowerCase();

        const matchesSearch =
            searchableText.includes(searchTerm);

        const matchesSemester =
            selectedSemester === "all" ||
            String(book.semester) === selectedSemester;

        return matchesSearch && matchesSemester;
    });

    bookGrid.replaceChildren();

    filteredBooks.forEach(book => {

        const card = document.createElement("article");
        card.className = "book-card";

        const top = document.createElement("div");
        top.className = "book-card-top";

        const icon = document.createElement("span");
        icon.className = "book-card-icon";
        icon.textContent = "▤";

        const status = document.createElement("span");

        status.className = book.available
            ? "book-status"
            : "book-status unavailable";

        status.textContent = book.available
            ? "AVAILABLE"
            : "UNAVAILABLE";

        top.append(icon, status);

        const title = document.createElement("h3");
        title.textContent = book.title;

        const subject = document.createElement("p");
        subject.className = "book-subject";
        subject.textContent = book.subject;

        const meta = document.createElement("div");
        meta.className = "book-meta";

        const semester = document.createElement("span");
        semester.textContent = `Semester ${book.semester}`;

        const condition = document.createElement("span");
        condition.textContent = book.condition;

        meta.append(semester, condition);

        const donor = document.createElement("p");
        donor.className = "book-donor";
        donor.textContent = `Shared by ${book.donor}`;

        const requestButton = document.createElement("button");

        requestButton.type = "button";
        requestButton.className = "primary-button";

        requestButton.textContent = book.available
            ? "Request This Book →"
            : "Currently Unavailable";

        requestButton.disabled = !book.available;

        requestButton.addEventListener("click", () => {

            window.location.href =
                `requests.html?bookID=${encodeURIComponent(book.id)}`;

        });

        card.append(
            top,
            title,
            subject,
            meta,
            donor,
            requestButton
        );

        bookGrid.appendChild(card);
    });

    bookCount.textContent =
        `${filteredBooks.length} ${
            filteredBooks.length === 1 ? "book" : "books"
        }`;

    emptyState.hidden = filteredBooks.length !== 0;

}


// ========================================
// 8. SEARCH AND FILTER
// ========================================

if (searchInput) {
    searchInput.addEventListener("input", renderBooks);
}

if (searchButton) {
    searchButton.addEventListener("click", renderBooks);
}

if (semesterFilter) {
    semesterFilter.addEventListener("change", renderBooks);
}


// ========================================
// 9. DONATE A BOOK
// ========================================

if (donationForm) {

    donationForm.addEventListener("submit", async function (event) {

        event.preventDefault();

        const donor =
            document.getElementById("donorName").value.trim();

        const title =
            document.getElementById("bookTitle").value.trim();

        const subject =
            document.getElementById("bookSubject").value.trim();

        const semester =
            Number(document.getElementById("bookSemester").value);

        const condition =
            document.getElementById("bookCondition").value;

        const contact =
            document.getElementById("donorContact").value.trim();

        if (
            !donor ||
            !title ||
            !subject ||
            !contact ||
            !condition ||
            !Number.isInteger(semester) ||
            semester < 1 ||
            semester > 8
        ) {

            showToast("Please fill in all book details correctly.");
            return;
        }

        const submitButton = donationForm.querySelector(
            'button[type="submit"]'
        );

        if (submitButton) {
            submitButton.disabled = true;
        }

        try {

            const response = await fetch(`${API_URL}/books`, {

                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    title,
                    donorName: donor,
                    subject,
                    semester
                })
            });

            const data = await response.json();

            if (!response.ok || data.status !== "success") {
                throw new Error(data.message || "Donation failed.");
            }

            donationForm.reset();

            showToast(
                `Book donated successfully! Book ID: ${data.bookID}`
            );

            await loadBooksFromBackend(false);

        } catch (error) {

            console.error("Donation failed:", error);

            showToast(
                error.message || "Could not connect to C backend."
            );

        } finally {

            if (submitButton) {
                submitButton.disabled = false;
            }
        }

    });

}


// ========================================
// 10. AVAILABLE BOOK DROPDOWN
// ========================================

function updateRequestOptions() {

    if (!requestBook) return;

    const previousValue = requestBook.value;

    requestBook.replaceChildren();

    const defaultOption = document.createElement("option");

    defaultOption.value = "";
    defaultOption.textContent = "Select an available book";

    requestBook.appendChild(defaultOption);

    books.filter(book => book.available).forEach(book => {

        const option = document.createElement("option");

        option.value = String(book.id);

        option.textContent =
            `${book.title} — Semester ${book.semester}`;

        requestBook.appendChild(option);

    });

    if ([...requestBook.options].some(
        option => option.value === previousValue
    )) {
        requestBook.value = previousValue;
    }

}


// ========================================
// 11. REQUEST A BOOK — FIFO QUEUE
// ========================================

if (requestForm) {

    requestForm.addEventListener("submit", async function (event) {

        event.preventDefault();

        const student =
            document.getElementById("requestStudent").value.trim();

        const bookId = Number(requestBook.value);

        const semester = Number(
            document.getElementById("requestSemester").value
        );

        const book = books.find(item => item.id === bookId);

        if (
            !student ||
            !book ||
            !book.available ||
            !Number.isInteger(semester) ||
            semester < 1 ||
            semester > 8
        ) {

            showToast(
                "Please select a valid available book and semester."
            );

            return;
        }

        const submitButton = requestForm.querySelector(
            'button[type="submit"]'
        );

        if (submitButton) {
            submitButton.disabled = true;
        }

        try {

            const response = await fetch(`${API_URL}/requests`, {

                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    bookID: bookId,
                    studentName: student
                })
            });

            const data = await response.json();

            if (!response.ok || data.status !== "success") {
                throw new Error(data.message || "Request failed.");
            }

            requestForm.reset();

            showToast(
                `Request submitted! Request ID: ${data.requestID}`
            );

            await loadRequestsFromBackend(false);
            await loadBooksFromBackend(false);

        } catch (error) {

            console.error("Book request failed:", error);

            showToast(
                error.message || "Could not submit book request."
            );

        } finally {

            if (submitButton) {
                submitButton.disabled = false;
            }
        }

    });

}


// ========================================
// 12. PRESELECT BOOK FROM EXPLORE PAGE
// ========================================

function selectRequestedBookFromURL() {

    if (!requestBook) return;

    const params = new URLSearchParams(window.location.search);

    const bookID = params.get("bookID");

    if (bookID) {
        requestBook.value = bookID;
    }

}


// ========================================
// 13. UNAVAILABLE BOOK DROPDOWN
// ========================================

function updateReturnOptions() {

    if (!returnBook) return;

    const previousValue = returnBook.value;

    returnBook.replaceChildren();

    const defaultOption = document.createElement("option");

    defaultOption.value = "";
    defaultOption.textContent = "Select an unavailable book";

    returnBook.appendChild(defaultOption);

    books.filter(book => !book.available).forEach(book => {

        const option = document.createElement("option");

        option.value = String(book.id);

        option.textContent =
            `${book.title} — Book ID: ${book.id}`;

        returnBook.appendChild(option);

    });

    if ([...returnBook.options].some(
        option => option.value === previousValue
    )) {
        returnBook.value = previousValue;
    }

}


// ========================================
// 14. RETURN A BOOK
// ========================================

if (returnForm) {

    returnForm.addEventListener("submit", async function (event) {

        event.preventDefault();

        const bookId = Number(returnBook.value);

        const book = books.find(item => item.id === bookId);

        if (!book || book.available) {

            showToast("Please select a valid unavailable book.");
            return;
        }

        const submitButton = returnForm.querySelector(
            'button[type="submit"]'
        );

        if (submitButton) {
            submitButton.disabled = true;
        }

        try {

            const response = await fetch(
                `${API_URL}/books/return`,
                {
                    method: "POST",

                    headers: {
                        "Content-Type": "application/json"
                    },

                    body: JSON.stringify({
                        bookID: bookId
                    })
                }
            );

            const data = await response.json();

            if (!response.ok || data.status !== "success") {
                throw new Error(data.message || "Book return failed.");
            }

            returnForm.reset();

            showToast(
                `${book.title} returned successfully! It is now available.`
            );

            await loadBooksFromBackend(false);

        } catch (error) {

            console.error("Book return failed:", error);

            showToast(
                error.message || "Could not return the book."
            );

        } finally {

            if (submitButton) {
                submitButton.disabled = false;
            }
        }

    });

}


// ========================================
// 15. PENDING REQUESTS DISPLAY
// ========================================

function renderRequests() {

    const requestSection = document.getElementById("requests");

    // Run only on Book Requests page.
    if (!requestSection) return;

    let panel = document.getElementById("pendingRequestsPanel");

    if (!panel) {

        panel = document.createElement("section");

        panel.id = "pendingRequestsPanel";
        panel.className = "pending-requests-panel";

        const heading = document.createElement("h2");
        heading.textContent = "Pending Book Requests";

        const description = document.createElement("p");

        description.textContent =
            "Requests are processed in the order they were received (FIFO).";

        const refreshButton = document.createElement("button");

        refreshButton.type = "button";
        refreshButton.className = "primary-button";
        refreshButton.textContent = "Refresh Requests";

        refreshButton.addEventListener("click", async () => {
            await loadRequestsFromBackend(true);
        });

        const list = document.createElement("div");
        list.id = "pendingRequestsList";

        const processButton = document.createElement("button");

        processButton.type = "button";
        processButton.id = "processOldestRequest";
        processButton.className = "primary-button";
        processButton.textContent = "Process Oldest Request";

        processButton.addEventListener(
            "click",
            processOldestRequest
        );

        panel.append(
            heading,
            description,
            refreshButton,
            list,
            processButton
        );

        requestSection.appendChild(panel);

    }

    const list = document.getElementById("pendingRequestsList");

    const processButton =
        document.getElementById("processOldestRequest");

    list.replaceChildren();

    if (requests.length === 0) {

        const empty = document.createElement("p");

        empty.textContent = "No pending requests.";

        list.appendChild(empty);

    } else {

        requests.forEach(request => {

            const item = document.createElement("article");

            item.className = "pending-request-item";

            const student = document.createElement("h3");

            student.textContent = request.student;

            const book = document.createElement("p");

            book.textContent =
                `Book: ${request.bookTitle} (ID: ${request.bookId})`;

            const id = document.createElement("p");

            id.textContent =
                `Request ID: ${request.requestId}`;

            item.append(student, book, id);

            list.appendChild(item);

        });

    }

    processButton.disabled = requests.length === 0;

}


// ========================================
// 16. PROCESS OLDEST REQUEST — FIFO
// ========================================

async function processOldestRequest() {

    if (requests.length === 0) {

        showToast("There are no pending requests to process.");
        return;
    }

    const processButton =
        document.getElementById("processOldestRequest");

    if (processButton) {
        processButton.disabled = true;
    }

    try {

        const response = await fetch(
            `${API_URL}/requests/process`,
            {
                method: "POST"
            }
        );

        const data = await response.json();

        if (!response.ok || data.status !== "success") {
            throw new Error(
                data.message || "Could not process request."
            );
        }

        showToast(
            `Processed request ${data.requestID} for ${data.studentName}.`
        );

        await loadBooksFromBackend(false);
        await loadRequestsFromBackend(false);

    } catch (error) {

        console.error("Processing request failed:", error);

        showToast(
            error.message || "Could not process the request."
        );

    } finally {

        if (processButton) {
            processButton.disabled = requests.length === 0;
        }
    }

}


// ========================================
// 17. START WEBSITE
// ========================================

async function startWebsite() {

    await loadBooksFromBackend(false);

    await loadRequestsFromBackend(false);

    // If a book was selected on Explore Books,
    // automatically select it on the Request page.
    selectRequestedBookFromURL();

}

startWebsite();
