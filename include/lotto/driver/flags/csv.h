/**
 * @file csv.h
 * @brief Driver declarations for CSV flag parsing helpers.
 */
#ifndef LOTTO_DRIVER_FLAGS_CSV_H
#define LOTTO_DRIVER_FLAGS_CSV_H

typedef int(csv_token_f)(const char *token, void *arg);

int csv_for_each(const char *csv, csv_token_f *fn, void *arg);

#endif
