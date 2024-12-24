#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DATABASE_CAPACITY 65536

struct database_entry {
    char *key;
    char *value;
};

struct database {
    size_t size;
    struct database_entry entry[DATABASE_CAPACITY];
};

void database__append(struct database *database, const char *key, const char *value) {
    if (database->size == DATABASE_CAPACITY) {
        fprintf(stderr, "database full!");
        exit(1);
    }
    database->size += 1;
    database->entry[database->size - 1].key = strdup(key);
    database->entry[database->size - 1].value = strdup(value);
}

int database__find_index(size_t *index_ref, const struct database *database, const char *key) {
    for (size_t i = 0; i < database->size; ++i) {
        if (strcmp(database->entry[i].key, key) != 0) {
            continue;
        }
        *index_ref = i;
        return 0;
    }
    return 1;
}

int database__get(char **value_ref, const struct database *database, const char *key) {
    size_t index;
    const int status = database__find_index(&index, database, key);
    if (status) {
        return status;
    }
    *value_ref = database->entry[index].value;
    return 0;
}

void database__put(struct database *database, const char *key, const char *value) {
    size_t index;
    const int status = database__find_index(&index, database, key);
    if (status) {
        database__append(database, key, value);
        return;
    }
    free(database->entry[index].value);
    database->entry[index].value = strdup(value);
    return;
}

int database__delete(struct database *database, const char *key) {
    size_t index;
    const int status = database__find_index(&index, database, key);
    if (status) {
        return status;
    }
    database->entry[index].key[0] = '\0';
    return 0;
}

int database__next(size_t *next_ref, const char **key_ref, const char **value_ref, const struct database *database, size_t start) {
    for (size_t i = start; i < database->size; ++i) {
        if (database->entry[i].key[0] == '\0') {
            continue;
        }
        *next_ref = i + 1;
        *key_ref = database->entry[i].key;
        *value_ref = database->entry[i].value;
        return 0;
    }
    return 1;
}

void database__clear(struct database *database) {
    for (size_t i = 0; i < database->size; ++i) {
        free(database->entry[i].key);
        free(database->entry[i].value);
    }
    database->size = 0;
}

#define USING_DATABASE(DB_VAR) \
    for(; (DB_VAR)->size; database__clear(DB_VAR))

FILE *open_database(const char *database_path, const char *mode) {
    FILE *database = fopen(database_path, mode);
    int fopen_errno = errno;
    if (fopen_errno) {
        perror("error: database open failure");
        exit(1);
    }
    return database;
}

void close_database(FILE **database_fp_ref) {
    int fclose_errno = fclose(*database_fp_ref);
    if (fclose_errno) {
        perror("error: database close failure");
        exit(1);
    }
    *database_fp_ref = NULL;
}

#define USING_DATABASE_FILE(DBFP_REF) \
    for (; *(DBFP_REF); close_database(DBFP_REF))

int main(int argc, char **argv) {
    struct database database;

    FILE *database_fp = open_database("database.txt", "r+");
    USING_DATABASE_FILE(&database_fp) {
        char *line = NULL;
        size_t capacity;
        ssize_t length; 
        char *key = NULL;
        char *value = NULL;
        while ((length = getline(&line, &capacity, database_fp)) > 0) {
            line[length - 1] = '\0';
            char *pos = line;
            key = strsep(&pos, ",");
            if (!pos) {
                continue;
            }
            value = pos;
            database__append(&database, key, value);
        }
    }

    USING_DATABASE(&database) {
        for (int i = 0; i < argc; ++i) {
            switch (argv[i][0]) {
            case 'p':
                {
                    char *pos = &argv[i][2];
                    const char *key = strsep(&pos, ",");
                    if (!pos) {
                        puts("bad command\n");
                        fprintf(stderr, "arg %d: expected (p,x,y), got (p,%s)", i, key);
                        continue;
                    }
                    const char *value = pos;
                    database__put(&database, key, value);
                }
                break;
            case 'g':
                {
                    char *key = &argv[i][2];
                    char *value = NULL;
                    int status = database__get(&value, &database, key);
                    if (status) {
                        printf("%s not found\n", key);
                        continue;
                    }
                    printf("%s,%s\n", key, value);
                }
                break;
            case 'd':
                {
                    char *key = &argv[i][2];
                    int status = database__delete(&database, key);
                    if (status) {
                        printf("%s not found\n", key);
                        continue;
                    }
                }
                break;
            case 'c':
                database__clear(&database);
                break;
            case 'a':
                {
                    size_t next_index = 0;
                    const char *key = NULL;
                    const char *value = NULL;
                    while (database__next(&next_index, &key, &value, &database, next_index) == 0) {
                        printf("%s,%s\n", key, value);
                    }
                }
                break;
            default:
                break;
            }
        }

        database_fp = open_database("database.txt", "w");
        USING_DATABASE_FILE(&database_fp) {
            size_t next_index = 0;
            const char *key = NULL;
            const char *value = NULL;
            while (database__next(&next_index, &key, &value, &database, next_index) == 0) {
                fprintf(database_fp, "%s,%s\n", key, value);
            }
        }
    }

    return 0;
}

