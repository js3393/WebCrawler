# Web Crawler README

## Group Members
- Jose Santos(RUID: (225009670), Dhruvi)l Patel (RUID: 200006142), Almuatasam Asseadi (RUID: 211005266)

##Roles of group members : 
- Jose (HTML Storage, Depth control, Word Searching, Thread management).
- Almuatasam Asseadi (Error handling). 
- Dhruvil Patel (Logging, URL Queue, Synchronization)
- it is worth noting that even though we did specific parts of the code apart, the whole code was written in cohesion, Everyone contributed in some shape to all of the written code, even if they are not directly responsible for it.

## Description
This web crawler is a multithreaded application designed to fetch HTML content from web pages, extract links, and log the process to a file. Then take a data set of important words and count how many occurences. It is implemented in C programming language and utilizes pthreads for multithreading and libcurl for handling HTTP requests.

### Architecture
The web crawler consists of several components:
- **Main Program**: The main program initializes the URL queue, spawns multiple threads to fetch URLs concurrently, and manages thread synchronization.
- **URL Queue**: A thread-safe FIFO queue implemented using a circular buffer to store URLs waiting to be fetched.
- **URL Fetching Threads**: Multiple threads are created to fetch URLs from the queue, download HTML content using libcurl, parse the content to extract links, and log the process.
- **Logging**: The crawler logs the fetching process, HTML content, and extracted links to a specified log file.
- **Word Counting**: Take all content in the html file, make all words lowercase, match each word to the set of important words, and increment count per word found

### Multithreading Approach
The crawler adopts a multithreading approach to improve efficiency by fetching multiple URLs concurrently. It uses pthreads for thread management and synchronization. Each fetching thread dequeues a URL from the URL queue, fetches its content, extracts links, and logs the process. The main program creates and joins threads, ensuring proper synchronization and resource management.

### Specific Roles and Contributions
- **Jose Santos**: Implemented the main program, URL queue, URL fetching threads, word finding, logging mechanism, and overall project organization. Conducted testing and debugging to ensure functionality and correctness.
- **Dhruvil Patel**: Developed the logging system, designed and managed the URL queue structure, and implemented synchronization between threads to ensure safe access to shared resources. Supported the stability of the program by handling concurrency issues and helped with debugging related to multi-threading and logging events.
- **Almuatasam Asseadi**: Implemented comprehensive error handling across the program, including invalid URLs, network failures, and file access errors. Ensured the crawler could handle unexpected failures gracefully without crashing and contributed to improving the overall reliability and robustness of the project.

### Libraries Used
- **pthread**: Used for multithreading and thread synchronization.
- **libcurl**: Used for making HTTP requests and fetching HTML content from web pages.

## How to Use
1. Compile the code using `make`.
2. Run the executable `crawler` with the starting URL as a command-line argument.
3. View the log file `crawler_log.txt` for the fetching process, HTML content, and extracted links.
4. (optional) You can also run the code using `make run` by default it will have the starting URL : https://en.wikipedia.org/wiki/Main_Page however this could be modified at the bottom of the makefile to any URL of the user's choosing. 
5.`make clean` command removes the executable (crawler), object files, and the log file (crawler_log.txt).

## Makefile
The provided Makefile compiles the source code into an executable named `crawler`. It also includes a `clean` target to remove object files and the executable.

