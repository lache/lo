#pragma once
int srp_alloc_random_bytes(unsigned char ** b, int len_b);
void srp_unhexify(const char * str, unsigned char ** b, int * len_b);
