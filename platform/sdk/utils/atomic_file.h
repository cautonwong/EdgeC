#ifndef CEDGE_ATOMIC_FILE_H
#define CEDGE_ATOMIC_FILE_H

/**
 * @brief Atomically saves content to a file using the write-sync-rename pattern.
 * 
 * This function first writes the content to a temporary file in the same
 * directory. It then calls fsync() to ensure the data is physically written
 * to disk. Finally, it uses rename() to atomically replace the original file
 * with the new one. This pattern guarantees that the original file will not be
 * corrupted or left in an inconsistent state, even in the event of a power
- * failure or system crash during the write process.
 * 
 * @param filename The final path of the file to save.
 * @param content The string content to write to the file.
 * @return 0 on success, -1 on failure.
 */
int atomic_file_save(const char *filename, const char *content);

#endif // CEDGE_ATOMIC_FILE_H
