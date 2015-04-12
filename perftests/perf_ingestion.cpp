#include <iostream>
#include <cassert>
#include <random>
#include <tuple>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <apr_mmap.h>
#include <apr_general.h>
#include <sys/time.h>

#include "akumuli.h"

using namespace std;

const int DB_SIZE = 2;
const int NUM_ITERATIONS = 10;

const char* DB_NAME = "test";
const char* DB_PATH = "./test";
const char* DB_META_FILE = "./test/test.akumuli";

void delete_storage() {
    aku_remove_database(DB_META_FILE, &aku_console_logger);
}

bool query_database_forward(aku_Database* db) {
    const unsigned int NUM_ELEMENTS = 100;
    aku_ParamId params[] = {1};
    aku_Timestamp begin = 100;
    aku_Timestamp end = 0;
    aku_SelectQuery* query = aku_make_select_query( begin
                                                  , end
                                                  , 2
                                                  , params);
    aku_Cursor* cursor = aku_select(db, query);
    while(!aku_cursor_is_done(cursor)) {
        int err = AKU_SUCCESS;
        if (aku_cursor_is_error(cursor, &err)) {
            std::cout << aku_error_message(err) << std::endl;
            return false;
        }
        aku_Timestamp timestamps[NUM_ELEMENTS];
        aku_ParamId paramids[NUM_ELEMENTS];
        aku_PData pointers[NUM_ELEMENTS];
        uint32_t lengths[NUM_ELEMENTS];
        int n_entries = aku_cursor_read_columns(cursor, timestamps, paramids, pointers, lengths, NUM_ELEMENTS);
        for (int i = 0; i < n_entries; i++) {
            std::cout << "Read #" << i << " : ts = (" << timestamps[i] << "), id = (" << paramids[i]  << "), value = (" << pointers[i].float64 << "), length = (" << lengths[i] << ")." << std::endl;
        }
    }
    aku_close_cursor(cursor);

    return true;
}

enum Mode {
    NONE,
    CREATE,
    DELETE,
    READ
};

Mode read_cmd(int cnt, const char** args) {
    if (cnt < 2) {
        return NONE;
    }
    if (std::string(args[1]) == "create") {
        return CREATE;
    }
    if (std::string(args[1]) == "read") {
        return READ;
    }
    if (std::string(args[1]) == "delete") {
        return DELETE;
    }
    std::cout << "Invalid command line" << std::endl;
    throw std::runtime_error("Bad command line parameters");
}

int main(int cnt, const char** args)
{
    Mode mode = read_cmd(cnt, args);

    aku_initialize(nullptr);

    if (mode == DELETE) {
        delete_storage();
        std::cout << "storage deleted" << std::endl;
        return 0;
    }

    if (mode != READ) {
        // Cleanup
        delete_storage();

        // Create database : using default threshold, windowsize, max_cache_size and no logger function
        apr_status_t result = aku_create_database(DB_NAME, DB_PATH, DB_PATH, DB_SIZE, 0, 0, 0, nullptr);
        if (result != APR_SUCCESS) {
            std::cout << "Error in new_storage" << std::endl;
            return (int)result;
        }
    }

    // Open database : using default aku_FineTuneParams's values
    aku_FineTuneParams params = {};
    auto db = aku_open_database(DB_META_FILE, params);

    if (mode != READ) {
        uint64_t busy_count = 0;
        // Fill in data
        for(uint64_t i = 0; i < NUM_ITERATIONS; i++) {
            double value = i + 0.5;
            aku_ParamId id = i % 2;
            std::cout << "Write #" << i << " : ts = (" << i << "), id = (" << id  << "), value = (" << value << ")." << std::endl;
            aku_Status status = aku_write_double_raw(db, id, i, value);
            if (status == AKU_EBUSY) {
                status = aku_write_double_raw(db, id, i, value);
                busy_count++;
                if (status != AKU_SUCCESS) {
                    std::cout << "add error at " << i << std::endl;
                    return 1;
                }
            }
        }
        std::cout << "busy count = " << busy_count << std::endl;
    }

    if (mode != CREATE) {
        // Search
        std::cout << "Sequential access" << std::endl;
        aku_SearchStats search_stats;
        if (!query_database_forward(db)) {
            return 2;
        }
    }

    aku_close_database(db);

    if (mode == NONE) {
        delete_storage();
    }
    return 0;
}

