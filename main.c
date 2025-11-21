// Tell compiler to use POSIX.1-2008 and later for APIs like pthreads
#define _POSIX_C_SOURCE 200809L
// Standard libraries needed for I/O, memory management, string handling, multithreading, etc.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <curl/curl.h> // for downloading web pages
#include <ctype.h>  // for character handling functions like tolower
#include <time.h> // for timestamping or time functions (if used)

// Constants for basic settings
#define BASE_URL "https://books.toscrape.com/catalogue/category/books/travel_2/index.html" // Website to start crawling
#define MAX_URL_LENGTH 1000 // Maximum length of a URL string
#define MAX_DEPTH 200 // Maximum depth for recursive crawling
#define MAX_THREADS 10 // Number of threads for parallel crawling
#define LOG_FILE "crawler_log.txt" // Log file name
#define URLS_FILE "urls.txt" // File to save visited URLs
// Limit for number of URLs per depth
#define MAX_URLS_PER_DEPTH 200

// Important words to search for inside the HTML pages
const char *important_words[] = {"data", "algorithm", "math", "generate", "link", "information"};
const int word_count = sizeof(important_words) / sizeof(important_words[0]);

// Structure to store a URL along with its crawl depth 
typedef struct {
    char url[MAX_URL_LENGTH];
    int depth;
} URL;

// Structure to represent a thread-safe queue for URLs
typedef struct {
    URL data[MAX_URL_LENGTH]; // Array of URLs
    int front, rear; // Tracks indices for front and rear of the queue
    pthread_mutex_t lock; // Mutex for thread-safe access ensures one thread mutates at a time
    pthread_cond_t cond; // Condition variable for thread waiting when queue is empty until new URL arrives
} URLQueue;

// Global variables for the crawler
URLQueue urlQueue;
pthread_t threads[MAX_THREADS]; //pThread IDs
FILE *logFile;
FILE *urlsFile;
int done = 0;    // Flag to indicate if crawling is done
pthread_mutex_t done_lock = PTHREAD_MUTEX_INITIALIZER;
int urls_per_depth[MAX_DEPTH];
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t urls_per_depth_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t urls_file_lock = PTHREAD_MUTEX_INITIALIZER;
char *visited_urls[MAX_URL_LENGTH];
int visited_count = 0;
pthread_mutex_t visited_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Initializes a URL queue by setting the front and rear to -1 and
 * initializing its mutex and condition variable.
 */
void initQueue(URLQueue *queue) {
    queue->front = queue->rear = -1;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

/**
 * Saves the HTML content of a page to a local file (page_X.html) where X is the index.
 * Logs success or error information into the log file.
 */
void save_html(const char *html_content, int index, const char *url) {
    if (!html_content) {
        pthread_mutex_lock(&print_lock);
        fprintf(stderr, "Error: html_content is NULL in save_html for URL: %s\n", url);
        fprintf(logFile, "Error: html_content is NULL in save_html for URL: %s\n", url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
        return;
    }
    char filename[100];
    snprintf(filename, sizeof(filename), "page_%d.html", index); // Generate filename
    FILE *file = fopen(filename, "w");

    if (file) {
        // Save the content to file
        fprintf(file, "%s", html_content);
        fclose(file);
         // Log the successful save
        pthread_mutex_lock(&print_lock);
        printf("HTML content saved to %s for URL: %s\n", filename, url);
        fprintf(logFile, "HTML content saved to %s for URL: %s\n", filename, url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
    } else {
        // Log any file opening error
        pthread_mutex_lock(&print_lock);
        perror("Error opening output file");
        fprintf(logFile, "Error opening output file: %s for URL: %s\n", filename, url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
    }
}

/**
 * Finds and counts occurrences of important words in the HTML content.
 * It prints and logs how many times each important word appears on a page.
 */
void word_finder(const char *html_content, int page_index, const char *url) {
    if (!html_content) {
        pthread_mutex_lock(&print_lock);
        fprintf(stderr, "Error: html_content is NULL in word_finder for URL: %s\n", url);
        fprintf(logFile, "Error: html_content is NULL in word_finder for URL: %s\n", url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
        return;
    }
    int count[word_count];
    memset(count, 0, sizeof(count)); // Initialize count array to 0

    // Make a lowercase copy of the HTML content to make search case-insensitive
    char *lowercase_content = strdup(html_content);
    if (!lowercase_content) {
        pthread_mutex_lock(&print_lock);
        perror("Failed to allocate memory for lowercase_content");
        fprintf(logFile, "Failed to allocate memory for lowercase_content for URL: %s\n", url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
        return;
    }
     // Convert all characters to lowercase
    for (char *p = lowercase_content; *p; ++p) {
        *p = tolower(*p);
    }
     // Count occurrences of each important word
    for (int i = 0; i < word_count; i++) {
        char *ptr = lowercase_content;
        while ((ptr = strstr(ptr, important_words[i])) != NULL) {
            int word_len = strlen(important_words[i]);
            char before = (ptr == lowercase_content) ? ' ' : *(ptr - 1);
            char after = ptr[word_len];
            // Check if the word is not part of another word (by checking surrounding characters)
            if ((before == ' ' || ispunct(before) || before == '\n' || before == '\t') &&
                (after == ' ' || ispunct(after) || after == '\n' || after == '\t' || after == '\0')) {
                count[i]++;
            }
            ptr += word_len;
        }
    }
    // Print and log the word counts
    pthread_mutex_lock(&print_lock);
    printf("Word counts for page_%d (URL: %s):\n", page_index, url);
    fprintf(logFile, "Word counts for page_%d (URL: %s):\n", page_index, url);
    for (int i = 0; i < word_count; i++) {
        printf("The word '%s' appears %d times on page_%d.\n", important_words[i], count[i], page_index);
        fprintf(logFile, "The word '%s' appears %d times on page_%d.\n", important_words[i], count[i], page_index);
    }
    printf("--- End of word counts for page_%d ---\n", page_index);
    fprintf(logFile, "--- End of word counts for page_%d ---\n", page_index);
    fflush(logFile);
    pthread_mutex_unlock(&print_lock);
    free(lowercase_content); // Free allocated memory
}
/**
 * Saves a URL into the "urls.txt" file in a thread-safe way.
 */
void save_url_to_file(const char *url) {
    pthread_mutex_lock(&urls_file_lock);
    fprintf(urlsFile, "%s\n", url);
    fflush(urlsFile);
    pthread_mutex_unlock(&urls_file_lock);
}
/**
 * Adds a URL to the URL queue in a thread-safe manner.
 * If the queue is full, logs an error and discards the URL.
 */
void enqueue(URLQueue *queue, const URL *url) {
    pthread_mutex_lock(&queue->lock);
    if (queue->rear == MAX_URL_LENGTH - 1) {
        // Queue is full; cannot enqueue
        pthread_mutex_unlock(&queue->lock);
        pthread_mutex_lock(&print_lock);
        fprintf(logFile, "Queue full, cannot enqueue URL: %s\n", url->url);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);
        return;
    }
    if (queue->front == -1)
        queue->front = 0;
    queue->rear++;
    queue->data[queue->rear] = *url; // Copy the URL into the queue
    pthread_cond_signal(&queue->cond);  // Wake up any thread waiting for URLs
    pthread_mutex_unlock(&queue->lock);
}

/**
 * Checks if the URL queue is empty.
 * Returns 1 (true) if empty, otherwise 0 (false).
 */
int isEmpty(URLQueue *queue) {
    return queue->front == -1 || queue->front > queue->rear;
}

/**
 * Dequeues a URL from the front of the URL queue in a thread-safe way.
 * If queue is empty and crawling is done, returns an empty URL struct.
 * Otherwise, waits until a URL is available.
 */
URL dequeue(URLQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    while (isEmpty(queue)) {
        pthread_mutex_lock(&done_lock);
        if (done) {
            pthread_mutex_unlock(&done_lock);
            pthread_mutex_unlock(&queue->lock);
            URL empty_url = {{0}, 0}; // Return empty URL
            return empty_url;
        }
        pthread_mutex_unlock(&done_lock);
        pthread_cond_wait(&queue->cond, &queue->lock); // Wait until URL is available
    }
    URL url = queue->data[queue->front++];
    pthread_mutex_unlock(&queue->lock);
    return url;
}

/**
 * Callback function used by libcurl to write the downloaded HTML data into memory.
 * Handles initial memory allocation and reallocation when more data arrives.
 */
size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t totalSize = size * nmemb;
    char **html = (char **)userp;

    if (*html == NULL) {
        *html = malloc(totalSize + 1);
        if (*html == NULL) {
            pthread_mutex_lock(&print_lock);
            fprintf(stderr, "Error: malloc failed in writeCallback\n");
            fprintf(logFile, "Error: malloc failed in writeCallback\n");
            fflush(logFile);
            pthread_mutex_unlock(&print_lock);
            return 0;
        }
        memcpy(*html, ptr, totalSize);
        (*html)[totalSize] = '\0';
    } else {
        size_t oldLen = strlen(*html);
        char *newHtml = realloc(*html, oldLen + totalSize + 1);
        if (newHtml == NULL) {
            pthread_mutex_lock(&print_lock);
            fprintf(stderr, "Error: realloc failed in writeCallback\n");
            fprintf(logFile, "Error: realloc failed in writeCallback\n");
            fflush(logFile);
            pthread_mutex_unlock(&print_lock);
            free(*html);
            *html = NULL;
            return 0;
        }
        *html = newHtml;
        memcpy(*html, ptr, totalSize);
        (*html)[oldLen + totalSize] = '\0';
    }

    return totalSize;
}

/**
 * Thread function for fetching and processing URLs.
 * Each thread continuously dequeues URLs, fetches HTML content, processes the page,
 * extracts new links, saves results, and enqueues new URLs to be crawled.
 */
void *fetchURL(void *arg) {
    static int page_counter = 1;
    static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

    while (1) {
        URL url = dequeue(&urlQueue);
        if (url.url[0] == '\0') {
            break; // Exit if no more URLs and done flag set
        }
        pthread_mutex_lock(&print_lock);
        printf("Fetching URL: %s (Depth: %d)\n", url.url, url.depth);
        fprintf(logFile, "Fetching URL: %s (Depth: %d)\n", url.url, url.depth);
        fflush(logFile);
        pthread_mutex_unlock(&print_lock);

        if (url.depth < MAX_DEPTH) {
            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {
                pthread_mutex_lock(&print_lock);
                printf("Attempting to fetch URL: %s\n", url.url);
                fprintf(logFile, "Attempting to fetch URL: %s\n", url.url);
                fflush(logFile);
                pthread_mutex_unlock(&print_lock);
                curl_easy_setopt(curl, CURLOPT_URL, url.url);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
                char *html_content = NULL;
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_content);
                res = curl_easy_perform(curl);
                if (res == CURLE_OK && html_content) {
                    int current_page;
                    pthread_mutex_lock(&counter_lock);
                    current_page = page_counter++;
                    pthread_mutex_unlock(&counter_lock);
                    pthread_mutex_lock(&print_lock);
                    printf("Processing page_%d for URL: %s\n", current_page, url.url);
                    fprintf(logFile, "Processing page_%d for URL: %s\n", current_page, url.url);
                    fflush(logFile);
                    pthread_mutex_unlock(&print_lock);

                    // Save the URL to urls.txt
                    // Save URL and page contents
                    save_url_to_file(url.url);

                    save_html(html_content, current_page, url.url);
                    word_finder(html_content, current_page, url.url);

                    // Extract and handle links
                    char *html_lower = strdup(html_content);
                    if (!html_lower) {
                        pthread_mutex_lock(&print_lock);
                        fprintf(stderr, "Error: strdup failed for html_lower\n");
                        fprintf(logFile, "Error: strdup failed for html_lower\n");
                        fflush(logFile);
                        pthread_mutex_unlock(&print_lock);
                    } else {
                        for (char *p = html_lower; *p; ++p) {
                            *p = tolower(*p);
                        }
                        char *start = html_lower;
                        while (start && (start = strstr(start, "<a href=\"")) != NULL) {
                            start += strlen("<a href=\"");
                            char *end = strstr(start, "\"");
                            if (end) {
                                int length = end - start;
                                char link[MAX_URL_LENGTH];
                                if (length >= MAX_URL_LENGTH) {
                                    start = end + 1;
                                    continue;
                                }
                                strncpy(link, start, length);
                                link[length] = '\0';

                                pthread_mutex_lock(&print_lock);
                                printf("Extracted Link: %s\n", link);
                                fprintf(logFile, "Extracted Link: %s\n", link);
                                fflush(logFile);
                                pthread_mutex_unlock(&print_lock);

                                // Build new full URL
                                URL new_url;
                                char base_domain[MAX_URL_LENGTH];
                                char relative_base[MAX_URL_LENGTH];
                                strncpy(base_domain, BASE_URL, MAX_URL_LENGTH);

                                // Extract base domain by truncating at the third slash
                                char *third_slash = base_domain;
                                int slash_count = 0;
                                for (char *p = base_domain; *p; ++p) {
                                    if (*p == '/') {
                                        slash_count++;
                                        if (slash_count == 3) {
                                            third_slash = p;
                                            break;
                                        }
                                    }
                                }
                                if (slash_count >= 3) {
                                    *third_slash = '\0';
                                }

                                // Ensure base_domain ends with a '/'
                                size_t base_len = strlen(base_domain);
                                if (base_domain[base_len - 1] != '/') {
                                    if (base_len + 1 < MAX_URL_LENGTH) {
                                        base_domain[base_len] = '/';
                                        base_domain[base_len + 1] = '\0';
                                    }
                                }

                                // Compute the relative base from the current URL
                                // Calculate relative base
                                strncpy(relative_base, url.url, MAX_URL_LENGTH);
                                char *last_slash = strrchr(relative_base, '/');
                                if (last_slash) {
                                    *last_slash = '\0'; // Remove the last component (e.g., index.html)
                                }

                                // Handle absolute vs relative URLs
                                if (strncmp(link, "http", 4) == 0) {
                                    if (strlen(link) < MAX_URL_LENGTH) {
                                        if (strncmp(link, base_domain, strlen(base_domain)) != 0) {
                                            start = end + 1;
                                            continue;
                                        }
                                        strncpy(new_url.url, link, MAX_URL_LENGTH);
                                    } else {
                                        pthread_mutex_lock(&print_lock);
                                        fprintf(stderr, "Skipping long URL: %s\n", link);
                                        fprintf(logFile, "Skipping long URL: %s\n", link);
                                        fflush(logFile);
                                        pthread_mutex_unlock(&print_lock);
                                        start = end + 1;
                                        continue;
                                    }
                                } else {
                                    // Handle relative URLs
                                    if (strncmp(link, "../", 3) == 0) {
                                        // Resolve ../ by removing the last directory from relative_base
                                        char temp_base[MAX_URL_LENGTH];
                                        strncpy(temp_base, relative_base, MAX_URL_LENGTH);
                                        char *second_last_slash = strrchr(temp_base, '/');
                                        if (second_last_slash) {
                                            *second_last_slash = '\0';
                                        }
                                        snprintf(new_url.url, MAX_URL_LENGTH, "%s/%s", temp_base, link + 3);
                                    } else {
                                        snprintf(new_url.url, MAX_URL_LENGTH, "%s/%s", base_domain, link);
                                    }
                                }
                                new_url.depth = url.depth + 1;

                                if (new_url.depth >= MAX_DEPTH) {
                                    start = end + 1;
                                    continue;
                                }

                                // Check if URL already visited
                                pthread_mutex_lock(&visited_lock);
                                int is_visited = 0;
                                for (int i = 0; i < visited_count; i++) {
                                    if (visited_urls[i] && strcmp(visited_urls[i], new_url.url) == 0) {
                                        is_visited = 1;
                                        break;
                                    }
                                }
                                if (!is_visited && visited_count < MAX_URL_LENGTH) {
                                    visited_urls[visited_count] = strdup(new_url.url);
                                    visited_count++;
                                }
                                pthread_mutex_unlock(&visited_lock);
                                if (is_visited) {
                                    start = end + 1;
                                    continue;
                                }

                                // Enqueue new URL
                                pthread_mutex_lock(&urls_per_depth_lock);
                                if (urls_per_depth[new_url.depth] >= MAX_URLS_PER_DEPTH) {
                                    pthread_mutex_unlock(&urls_per_depth_lock);
                                    start = end + 1;
                                    continue;
                                }
                                urls_per_depth[new_url.depth]++;
                                pthread_mutex_unlock(&urls_per_depth_lock);

                                enqueue(&urlQueue, &new_url);
                                start = end + 1;
                            }
                        }
                        free(html_lower);
                    }
                    pthread_mutex_lock(&print_lock);
                    printf("Successfully processed URL: %s\n", url.url);
                    fprintf(logFile, "Successfully processed URL: %s\n", url.url);
                    fflush(logFile);
                    pthread_mutex_unlock(&print_lock);
                } else {
                    pthread_mutex_lock(&print_lock);
                    printf("Failed to fetch URL: %s (%s)\n", url.url, curl_easy_strerror(res));
                    fprintf(logFile, "Failed to fetch URL: %s (%s)\n", url.url, curl_easy_strerror(res));
                    fflush(logFile);
                    pthread_mutex_unlock(&print_lock);
                }
                curl_easy_cleanup(curl);
                free(html_content);
            }
        }

        // Small sleep to prevent aggressive resource usage
        struct timespec ts = {0, 100000000};
        nanosleep(&ts, NULL);

        if (isEmpty(&urlQueue)) {
            pthread_mutex_lock(&done_lock);
            done = 1;
            pthread_cond_broadcast(&urlQueue.cond);
            pthread_mutex_unlock(&done_lock);
            break;
        }
    }
    return NULL;
}

/**
 * Main crawling function: starts threads, initializes the queue,
 * enqueues the starting URL, and waits for all threads to complete.
 */
void crawl() {
    URL start;
    strncpy(start.url, BASE_URL, MAX_URL_LENGTH);
    start.depth = 0;
    initQueue(&urlQueue);
    enqueue(&urlQueue, &start);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, fetchURL, NULL);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

/**
 * Main function.
 * Opens log and URLs files, initializes CURL, starts crawling,
 * and cleans up all allocated resources afterward.
 */
int main(int argc, char *argv[]) {
    logFile = fopen(LOG_FILE, "a");
    if (!logFile) {
        perror("Error opening log file");
        return 1;
    }

    urlsFile = fopen(URLS_FILE, "a");
    if (!urlsFile) {
        perror("Error opening urls file");
        fclose(logFile);
        return 1;
    }

    printf("Starting crawl with base URL: %s\n", BASE_URL);
    fprintf(logFile, "Starting crawl with base URL: %s\n", BASE_URL);
    fflush(logFile);

    memset(urls_per_depth, 0, sizeof(urls_per_depth));
    memset(visited_urls, 0, sizeof(visited_urls));
    visited_count = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    crawl();
    for (int i = 0; i < visited_count; i++) {
        free(visited_urls[i]);
    }
    curl_global_cleanup();
    fclose(logFile);
    fclose(urlsFile);
    return 0;
}