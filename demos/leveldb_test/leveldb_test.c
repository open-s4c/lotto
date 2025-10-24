
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "leveldb/c.h"

char *error      = NULL;
const char *path = "./leveldb";

void
check_error(char *msg, char *error)
{
    if (error != NULL) {
        fprintf(stderr, "Error detected %s | %s\n", msg, error);
        exit(0);
    }
    leveldb_free(error);
    error = NULL;
}

void
write_leveldb(const char *key, const char *value)
{
    leveldb_t *db;
    leveldb_options_t *options = leveldb_options_create();

    leveldb_options_set_create_if_missing(options, 1);

    db = leveldb_open(options, path, &error);

    check_error("OPEN FAILED", error);

    leveldb_writeoptions_t *woptions = leveldb_writeoptions_create();
    leveldb_put(db, woptions, key, strlen(key), value, strlen(value), &error);

    check_error("WRITE FAILED", error);

    leveldb_close(db);
}

char *
read_leveldb(const char *key)
{
    leveldb_t *db;
    leveldb_options_t *options = leveldb_options_create();

    leveldb_options_set_create_if_missing(options, 1);
    db = leveldb_open(options, path, &error);

    check_error("OPEN FAILED", error);

    size_t value_size;

    leveldb_readoptions_t *roptions = leveldb_readoptions_create();
    char *value =
        leveldb_get(db, roptions, key, strlen(key), &value_size, &error);

    check_error("READ FAILED", error);

    printf("get key = %s\n", value);
    leveldb_close(db);

    return value;
}

void
iterate_db()
{
    leveldb_t *db;
    leveldb_options_t *options = leveldb_options_create();

    leveldb_options_set_create_if_missing(options, 1);

    db = leveldb_open(options, path, &error);

    check_error("OPEN FAILED", error);

    leveldb_writeoptions_t *woptions = leveldb_writeoptions_create();
    {
        const char *key = "alice", *value = "val_alice";
        leveldb_put(db, woptions, key, strlen(key), value, strlen(value),
                    &error);
        check_error("WRITE FAILED", error);
    }
    {
        const char *key = "bob", *value = "val_bob";
        leveldb_put(db, woptions, key, strlen(key), value, strlen(value),
                    &error);
        check_error("WRITE FAILED", error);
    }
    {
        const char *key = "test", *value = "val_test";
        leveldb_put(db, woptions, key, strlen(key), value, strlen(value),
                    &error);
        check_error("WRITE FAILED", error);
    }

    {
        // ITERATE
        leveldb_readoptions_t *roptions = leveldb_readoptions_create();
        leveldb_iterator_t *iter        = leveldb_create_iterator(db, roptions);

        for (leveldb_iter_seek_to_first(iter); leveldb_iter_valid(iter);
             leveldb_iter_next(iter)) {
            size_t klen, vlen;
            const char *key   = leveldb_iter_key(iter, &klen);
            const char *value = leveldb_iter_value(iter, &vlen);

            printf("key = %.*s | value = %.*s\n", (int)klen, key, (int)vlen,
                   value);
        }
        leveldb_iter_destroy(iter);
        check_error("ITERATOR DESTROY FAILED", error);
    }
    leveldb_close(db);
}
void *
run()
{
    const char *value = "alice";

    /* If libcrep is not being used, this should print nothing in record and 4
       key-values in replay key = alice | value = val_alice key = bob | value =
       val_bob key = oi | value = alice key = test | value = val_test
    */
    iterate_db();

    // If libcrep is not being used, this should return NULL in record and
    // non-NULL in replay
    read_leveldb("oi");
    write_leveldb("oi", value);
    read_leveldb("oi");

    iterate_db();

    return NULL;
}


int
main()
{
    pthread_t t1;
    pthread_create(&t1, NULL, run, NULL);
    pthread_join(t1, NULL);

    return 0;
}
