#include <stdexcept>
#include <iomanip>
#include <iostream>
#include "CuckooTable.h"


template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::CuckooTable(const size_t table_size, uint32_t fp_mask) {
    this->table_size = table_size;
    this->fp_mask = fp_mask;

    buckets = new Bucket[table_size];
    memset(buckets, 0, bytes_per_bucket * table_size);

    if (entries_per_bucket == 4 && bits_per_fp == 4 && std::is_same<fp_type, uint8_t>::value) {
        bit_manager = new BitManager4<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 8 && std::is_same<fp_type, uint8_t>::value) {
        bit_manager = new BitManager8<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 12 && std::is_same<fp_type, uint16_t>::value) {
        bit_manager = new BitManager12<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 16 && std::is_same<fp_type, uint16_t>::value) {
        bit_manager = new BitManager16<fp_type>();
    } else if (entries_per_bucket == 2 && bits_per_fp == 32 && std::is_same<fp_type, uint32_t>::value) {
        bit_manager = new BitManager32<fp_type>();
    } else {
        throw std::runtime_error("Bits per fingerprint should be in {4, 8, 12, 16, 32}");
    }

}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::~CuckooTable() {
    delete[] buckets;
    delete bit_manager;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
size_t CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::getTableSize() {
    return table_size;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
size_t CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::maxNoOfElements() {
    return entries_per_bucket * table_size;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
inline uint32_t CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::getFingerprint(const size_t i, const size_t j) {
    const uint8_t *bucket = buckets[i].data;
    //bucket += j;
    //uint32_t  fp = *((uint8_t *)bucket);
    uint32_t fp = bit_manager->read(j, bucket);
    return fp & fp_mask;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
size_t CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::fingerprintCount(const size_t i) {
    size_t count = 0;
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) != 0) {
            count++;
        }
    }
    return count;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
void CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::insertFingerprint(const size_t i, const size_t j, const uint32_t fp) {
    const uint8_t *bucket = buckets[i].data;
    uint32_t efp = fp & fp_mask;
    bit_manager->write(j, bucket, efp);
    //((uint8_t *)bucket)[j] = efp;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
inline bool CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::replacingFingerprintInsertion(const size_t i, const uint32_t fp,
                                                                const bool eject, uint32_t &prev_fp) {
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) == 0) {
            insertFingerprint(i, j, fp);
            return true;
        }
    }

    if (eject) {
        size_t next = rand() % entries_per_bucket;
        prev_fp = getFingerprint(i, next);
        insertFingerprint(i, next, fp);
    }
    return false;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
bool CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::containsFingerprint(const size_t i, const uint32_t fp) {
    const uint8_t *bucket = buckets[i].data;
    uint64_t val = *((uint64_t *) bucket);

    return bit_manager->hasvalue(val, fp);
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
bool CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::containsFingerprint(const size_t i1, const size_t i2, const uint32_t fp) {
    const uint8_t *b1 = buckets[i1].data;
    const uint8_t *b2 = buckets[i2].data;

    uint64_t val1 = *((uint64_t *) b1);
    uint64_t val2 = *((uint64_t *) b2);

    return bit_manager->hasvalue(val1, fp) || bit_manager->hasvalue(val2, fp);
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
bool CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::deleteFingerprint(const uint32_t fp, const size_t i) {
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) == fp) {
            insertFingerprint(i, j, 0);
            return true;
        }
    }
    return false;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
size_t CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::getNumOfFreeEntries() {
    size_t free = 0;
    for (size_t i = 0; i < table_size; ++i) {
        for (size_t j = 0; j < entries_per_bucket; ++j) {
            if (getFingerprint(i, j) == 0) {
                free++;
            }
        }
    }
    return free;
}

template<typename fp_type, size_t entries_per_bucket, size_t bits_per_fp>
void CuckooTable<fp_type, entries_per_bucket, bits_per_fp>::printTable() {
    for (int i = 0; i < table_size; ++i) {
        std::cout << i << " | ";
        for (int j = 0; j < entries_per_bucket; ++j) {
            auto bucket = buckets[i].data;
            uint32_t fp = bit_manager->read(j, bucket);
            std::cout << std::setfill('0') << std::setw(8) << std::hex << fp << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec;
}

template
class CuckooTable<uint8_t>;

template
class CuckooTable<uint16_t>;

template
class CuckooTable<uint32_t>;

template
class CuckooTable<uint64_t>;