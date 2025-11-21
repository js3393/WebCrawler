# Web Crawler README

## Group Members
- Jose Santos (RUID: 225009670)

- Dhruvil Patel (RUID: 200006142)

- Almuatasam Asseadi (RUID: 211005266)

## Roles: 
- Jose Santos – HTML storage, depth control, word searching, thread management

- Dhruvil Patel – Logging, URL queue, synchronization

- Almuatasam Asseadi – Error handling

Note:
Although responsibilities were divided, the entire codebase was written collaboratively. Each team member contributed to multiple parts of the project regardless of their primary assigned role.

## Description
This project is a multithreaded web crawler implemented in C. It fetches HTML content from a starting web page, extracts links, stores the content, and logs the crawling process. Additionally, it analyzes each downloaded page by converting the text to lowercase and counting occurrences of a predefined set of “important words.”

The program uses:

    - pthreads for multithreading

    - libcurl for HTTP requests

At this stage, the crawler uses a fixed starting URL to ensure reliable and reproducible behavior.
Future versions will expand support for general URLs provided by the user.

### Architecture
The web crawler consists of several components:
- **Main Program**: The main program initializes the URL queue, spawns multiple threads to fetch URLs concurrently, and manages thread synchronization.
- **URL Queue**: A thread-safe FIFO queue implemented using a circular buffer to store URLs waiting to be fetched.
- **URL Fetching Threads**: Multiple threads are created to fetch URLs from the queue, download HTML content using libcurl, parse the content to extract links, and log the process.
- **Logging**: The crawler logs the fetching process, HTML content, and extracted links to a specified log file.
- **Word Counting**: Take all content in the html file, make all words lowercase, match each word to the set of important words, and increment count per word found

### Multithreading Approach
The program uses POSIX threads (pthreads) to fetch multiple URLs concurrently. Synchronization mechanisms include:

    - Mutexes for shared data structures (URL queue, counters, visited list)

    - A condition variable to block threads when the queue is empty

    - Controlled shutdown using a global done flag

Threads repeatedly:

    1. Dequeue a URL

    2. Fetch HTML content

    3. Extract new links

    4. Save data

    5. Log activity

    6. Enqueue new URLs (within depth limits)

The main thread waits for all worker threads to finish before cleanup.

### Specific Roles and Contributions
**Jose Santos**: 
    - Implemented the main control loop, URL queue logic, thread creation/management, HTML saving, wordfinding system, and major portions of link extraction.
    - Coordinated integration of components and performed extensive testing/debugging.
**Dhruvil Patel**: 
    - Developed the logging system, designed and managed the URL queue structure, and implemented synchronization between threads to ensure safe access to shared resources. 
    - Supported the stability of the program by handling concurrency issues and helped with debugging related to multi-threading and logging events.
**Almuatasam Asseadi**: 
    - Implemented comprehensive error handling across the program, including invalid URLs, network failures, and file access errors. 
    - Ensured the crawler could handle unexpected failures gracefully without crashing and contributed to improving the overall reliability and robustness of the project.

### Libraries Used
- **pthread**: Used for multithreading and thread synchronization.
- **libcurl**: Used for making HTTP requests and fetching HTML content from web pages.

## How to Use
1. Compile the program using `make` in the terminal.

2. Run the executable `crawler` with `./crawler` in the terminal.

3. View the log file `crawler_log.txt` for the crawl progress, extracted links, and diagnostic messages.

4. (optional) You can also run the code using `make run` by default it will have the starting URL : https://books.toscrape.com/catalogue/category/books/travel_2/index.html however this could be modified at the bottom of the makefile to any URL of the user's choosing (implementation currently under construction). 

5.`make clean` command removes the executable (crawler), object files, and the log file (crawler_log.txt).

## Makefile
The provided Makefile compiles the source code into an executable named `crawler`. It also includes a `clean` target to remove object files and the executable.

## Output Files

After running the crawler, you may see:

    - page_X.html — Saved HTML content of fetched pages

    - urls.txt — List of all successfully crawled URLs

    - crawler_log.txt — Detailed log of crawl events

## Current Limitations

To ensure correctness during development, the crawler currently:

    - Uses one fixed starting URL

    - Only crawls within the same domain

    - Does not fully normalize complex URLs (query parameters, fragments, JS links, etc.)

    - May skip pages with irregular or malformed link structures

    - Does not yet support command-line URLs (./crawler <URL>)

These limitations are intentional for stability and testing.

## Roadmap / Future Improvements

Planned enhancements include:

    - Accept starting URL as a command-line argument

    - Improved URL normalization (resolve fragments, queries, absolute/relative complexity)

    - Robots.txt support

    - Timeout/backoff handling

    - SQLite storage for URL graphs

    - More advanced HTML parsing

    - Safer memory handling + sanitizers

    - Optional single-thread or N-thread mode

## Contributing

Pull requests, suggestions, and optimizations are welcome!