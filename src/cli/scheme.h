/*
 */
#ifndef LOTTO_CLI_SCHEME_H
#define LOTTO_CLI_SCHEME_H

void lotto_scheme_register_library_path(const char *dir);
int lotto_scheme_eval_file(const char *fname);

#endif
