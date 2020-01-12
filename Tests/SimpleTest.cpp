#include "../ArgParser/cxxopts.hpp"
#include "../CF/CuckooFilter.h"
#include <chrono>


template<typename fp_type>
int insertIntsInRange(CuckooFilter<size_t, fp_type> *filter, size_t from, size_t to) {
    assert(from < to);

    size_t numInserted = 0;
    for (size_t i = from; i < to; i++, numInserted++) {
        if (!(*filter).insertElement(i)) {
            break;
        }
    }
    return numInserted;
}


template<typename fp_type>
void containsIntsInRange(CuckooFilter<size_t, fp_type> *filter, size_t from, size_t to) {
    for (size_t i = from; i < to; i++) {
        assert((*filter).containsElement(i));
    }
}

template<typename fp_type>
float getFPRate(CuckooFilter<size_t, fp_type> *filter, size_t from, size_t to) {
    size_t total_queries = 0;
    size_t false_queries = 0;
    for (size_t i = from; i < to; i++) {
        if ((*filter).containsElement(i)) {
            false_queries++;
        }
        total_queries++;
    }
    return 100.0 * false_queries / total_queries;
}

template<typename fp_type>
void deleteAll(CuckooFilter<size_t, fp_type> *filter, size_t from, size_t to) {
    for (size_t i = from; i < to; i++) {
        (*filter).deleteElement(i);
    }
}


int main(int argc, char **argv) {
    size_t tableSize = 1000;

    //Elements inserted in the filter are from 0 to numOfElements
    size_t numOfElements = tableSize;
//    size_t numOfElements = 1000000;

    int n = 30;

    double totalTime = 0.;
    double insTotalTime = 0.;
    double contTotalTime = 0.;
    double delTotalTime = 0.;

    size_t from = 0;
    size_t to = numOfElements;
    size_t numInserted;
    float numInsertedTot = 0;
    float fpRate;

    for (size_t i = 0; i < n; ++i) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        CuckooFilter<size_t, uint16_t> filter(tableSize, 16, 4);

        std::chrono::steady_clock::time_point insBegin = std::chrono::steady_clock::now();
        numInserted = insertIntsInRange(&filter, from, to);
        numInsertedTot += numInserted;
        std::chrono::steady_clock::time_point insEnd = std::chrono::steady_clock::now();
        insTotalTime += std::chrono::duration_cast<std::chrono::microseconds>(insEnd - insBegin).count();

        std::chrono::steady_clock::time_point containsBegin = std::chrono::steady_clock::now();
        containsIntsInRange(&filter, from, numInserted);
        std::chrono::steady_clock::time_point containsEnd = std::chrono::steady_clock::now();
        contTotalTime += std::chrono::duration_cast<std::chrono::microseconds>(containsEnd - containsBegin).count();

        fpRate = getFPRate(&filter, to, 2 * to);
        double availability = filter.availability();

        std::chrono::steady_clock::time_point delBegin = std::chrono::steady_clock::now();
        deleteAll(&filter, from, numInserted);
        std::chrono::steady_clock::time_point delEnd = std::chrono::steady_clock::now();
        delTotalTime += std::chrono::duration_cast<std::chrono::microseconds>(delEnd - delBegin).count();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

        std::cout << i << ". iter" << std::endl;
        std::cout << "Inserted: " << numInserted << "/" << numOfElements << std::endl;
        std::cout << "false positive rate is "
                  << fpRate << "%\n";
        std::cout << "availability: "
                  << availability << "%\n";

    }


    std::cout << "\nAvg insertion time: " << insTotalTime / n
              << "[µs] (for " << numInsertedTot / ((float) n) << " elements inserted in avg)" << std::endl;
    std::cout << "Avg lookup time: " << contTotalTime / n
              << "[µs] (for " << numInsertedTot / ((float) n) << " elements inserted in avg)" << std::endl;
    std::cout << "Avg deletion time: " << delTotalTime / n
              << "[µs] (for " << numInsertedTot / ((float) n) << " elements inserted in avg)" << std::endl;
    std::cout << "Avg time (all operations): " << totalTime / n
              << "[µs]" << std::endl;

}